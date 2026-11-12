param([string]$IdfRoot = "D:/1.Soft/Espressif", [string]$Port = "", [switch]$PackAudio, [string]$BuildDir = "build", [switch]$Merge)
$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$firmware = Join-Path $projectRoot "firmware"
if (![IO.Path]::IsPathRooted($BuildDir)) { $BuildDir = Join-Path $firmware $BuildDir }
Push-Location $projectRoot
try {
    foreach ($module in "prepare_content", "prepare_ui", "prepare_firmware") {
        & python -B -m "tools.pack.$module"
        if ($LASTEXITCODE -ne 0) { throw "Resource preparation failed: $module" }
    }
} finally { Pop-Location }
$env:IDF_PATH = Join-Path $IdfRoot "frameworks/esp-idf-v5.5.5"
$env:IDF_PYTHON_ENV_PATH = Join-Path $IdfRoot "python_env/idf5.5_py3.11_env"
$env:IDF_TOOLS_PATH = $IdfRoot
$env:PROCESSOR_ARCHITECTURE = "AMD64"
$env:IDF_CCACHE_ENABLE = "0"
$rom = Get-ChildItem (Join-Path $IdfRoot "tools/esp-rom-elfs") -Directory | Select-Object -Last 1
if ($rom) { $env:ESP_ROM_ELF_DIR = $rom.FullName }
$toolDirs = @("python_env/idf5.5_py3.11_env/Scripts", "tools/riscv32-esp-elf/esp-14.2.0_20260121/riscv32-esp-elf/bin", "tools/cmake/3.30.5/bin", "tools/ninja/1.12.1")
$env:PATH = (($toolDirs | ForEach-Object { Join-Path $IdfRoot $_ }) -join ";") + ";" + $env:PATH
Push-Location $firmware
try {
    $buildArgs = @("-DIFRUIT_PACK_AUDIO=ON", "build")
    if ($PackAudio) { $buildArgs = @("-DIFRUIT_PACK_AUDIO=ON", "build") }

    & python "$env:IDF_PATH/tools/idf.py" -B $BuildDir @buildArgs
    if ($LASTEXITCODE -ne 0) { throw "Firmware build failed" }
    if ($Merge) {
        & python "$env:IDF_PATH/tools/idf.py" -B $BuildDir merge-bin
        if ($LASTEXITCODE -ne 0) { throw "Merge failed" }
        Copy-Item -LiteralPath (Join-Path $BuildDir "merged-binary.bin") -Destination (Join-Path $BuildDir "ai-passport-ifruit-full.bin")
    }
    if ($Port) {
        & python "$env:IDF_PATH/tools/idf.py" -B $BuildDir -p $Port flash monitor
        if ($LASTEXITCODE -ne 0) { throw "Flash/monitor failed" }
    }
} finally { Pop-Location }
