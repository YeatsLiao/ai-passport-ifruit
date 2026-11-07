param([string]$IdfRoot = "D:/1.Soft/Espressif", [string]$Port = "", [switch]$PackAudio)
$ErrorActionPreference = "Stop"
& python (Join-Path $PSScriptRoot "prepare_content.py")
if ($LASTEXITCODE -ne 0) { throw "Content preparation failed" }
& python (Join-Path $PSScriptRoot "prepare_ui.py")
if ($LASTEXITCODE -ne 0) { throw "UI asset preparation failed; Python with Pillow is required" }
$env:IDF_PATH = Join-Path $IdfRoot "frameworks/esp-idf-v5.5.5"
$env:IDF_PYTHON_ENV_PATH = Join-Path $IdfRoot "python_env/idf5.5_py3.11_env"
$env:IDF_TOOLS_PATH = $IdfRoot
$env:PROCESSOR_ARCHITECTURE = "AMD64"
$env:IDF_CCACHE_ENABLE = "0"
$rom = Get-ChildItem (Join-Path $IdfRoot "tools/esp-rom-elfs") -Directory | Select-Object -Last 1
if ($rom) { $env:ESP_ROM_ELF_DIR = $rom.FullName }
$toolDirs = @("python_env/idf5.5_py3.11_env/Scripts", "tools/riscv32-esp-elf/esp-14.2.0_20260121/riscv32-esp-elf/bin", "tools/cmake/3.30.5/bin", "tools/ninja/1.12.1")
$env:PATH = (($toolDirs | ForEach-Object { Join-Path $IdfRoot $_ }) -join ";") + ";" + $env:PATH
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    $buildArgs = @("build")
    if ($PackAudio) { $buildArgs = @("-DIFRUIT_PACK_AUDIO=ON", "build") }
    else { $buildArgs = @("-DIFRUIT_PACK_AUDIO=OFF", "build") }
    & python "$env:IDF_PATH/tools/idf.py" @buildArgs
    if ($LASTEXITCODE -ne 0) { throw "Firmware build failed" }
    if ($Port) {
        & python "$env:IDF_PATH/tools/idf.py" -p $Port flash monitor
        if ($LASTEXITCODE -ne 0) { throw "Flash/monitor failed" }
    }
} finally { Pop-Location }
