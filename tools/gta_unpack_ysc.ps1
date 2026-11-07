param([Parameter(Mandatory=$true)][string]$InputFile,[Parameter(Mandatory=$true)][string]$OutputFile)
. "$PSScriptRoot/gta_support.ps1"
$inputPath=(Resolve-Path -LiteralPath $InputFile).Path
$target=[IO.Path]::GetFullPath($OutputFile)
$workspace=[IO.Path]::GetFullPath((Split-Path $PSScriptRoot -Parent))+[IO.Path]::DirectorySeparatorChar
if(-not $target.StartsWith($workspace,[StringComparison]::OrdinalIgnoreCase)){throw 'Output must be within project'}
if(Test-Path -LiteralPath $target){throw 'Refusing overwrite'}
$all=[IO.File]::ReadAllBytes($inputPath)
if([Text.Encoding]::ASCII.GetString($all,0,4) -ne 'RSC7'){throw 'Expected RSC7 script'}
$payload=[byte[]]$all[16..($all.Length-1)]
$plain=[RageLib.GTA5.Cryptography.GTA5Crypto]::DecryptNG($payload,[IO.Path]::GetFileName($inputPath),[uint32]$all.Length)
$ms=[IO.MemoryStream]::new($plain,$false);$def=[IO.Compression.DeflateStream]::new($ms,[IO.Compression.CompressionMode]::Decompress);$out=[IO.MemoryStream]::new()
$def.CopyTo($out);[IO.File]::WriteAllBytes($target,$out.ToArray());$out.Dispose();$def.Dispose();$ms.Dispose()
Get-Item -LiteralPath $target | Select-Object Name,Length
