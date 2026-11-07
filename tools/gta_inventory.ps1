param([string]$Game = 'D:/1.Soft/Steam/steamapps/common/Grand Theft Auto V')
$ErrorActionPreference='Stop'
$toolDir=Join-Path (Split-Path $PSScriptRoot -Parent) 'build/tooling/gtautil'
Get-ChildItem $toolDir -Filter '*.dll' | ForEach-Object {try {[Reflection.Assembly]::LoadFrom($_.FullName) | Out-Null} catch {}}
[Reflection.Assembly]::LoadFrom((Join-Path $toolDir 'ragelib.gta5.dll')) | Out-Null
# Use the official resource tool's bundled format support in memory; never export key material.
$assembly=[Reflection.Assembly]::LoadFrom((Join-Path $toolDir 'GTAUtil.exe'))
$resource=[Resources.ResourceManager]::new('GTAUtil.Resource',$assembly)
[RageLib.GTA5.Cryptography.GTA5Constants]::PC_AES_KEY=$resource.GetObject('gtav_aes_key')
[RageLib.GTA5.Cryptography.GTA5Constants]::PC_NG_KEYS=[RageLib.GTA5.Cryptography.Helpers.CryptoIO]::ReadNgKeys([byte[]]$resource.GetObject('gtav_ng_key'))
[RageLib.GTA5.Cryptography.GTA5Constants]::PC_NG_DECRYPT_TABLES=[RageLib.GTA5.Cryptography.Helpers.CryptoIO]::ReadNgTables([byte[]]$resource.GetObject('gtav_ng_decrypt_tables'))
[RageLib.GTA5.Cryptography.GTA5Constants]::PC_LUT=$resource.GetObject('gtav_hash_lut')
Write-Output 'Reading archive headers with GTAUtil/RageLib format support...' 
$rows=[Collections.Generic.List[object]]::new()
function WalkDirectory($dir,[string]$prefix) {
 foreach($f in $dir.GetFiles()) { $rows.Add([pscustomobject]@{path=($prefix+'/'+$f.Name);size=$f.Size}) }
 foreach($d in $dir.GetDirectories()) { WalkDirectory $d ($prefix+'/'+$d.Name) }
}
foreach($path in Get-ChildItem $Game -Filter '*.rpf') {
 $stream=[IO.File]::Open($path.FullName,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
 try {
  $archive=[RageLib.GTA5.ArchiveWrappers.RageArchiveWrapper7]::Open($stream,$path.Name,$true)
  WalkDirectory $archive.Root $path.Name
  $archive.Dispose()
  Write-Output ('Indexed '+$path.Name)
 } finally { $stream.Dispose() }
}
$output=Join-Path (Split-Path $PSScriptRoot -Parent) 'build/reference/gta-top-index.json'
$rows | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $output -Encoding utf8
$rows | Where-Object {$_.path -match 'scaleform|phone|cell|frontend'} | Format-Table -AutoSize
