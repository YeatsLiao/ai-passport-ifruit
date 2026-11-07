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
Write-Verbose 'Reading archive headers with GTAUtil/RageLib format support...' 
