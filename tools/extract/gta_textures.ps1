param([Parameter(Mandatory=$true)][string]$InputYtd,[Parameter(Mandatory=$true)][string]$OutputDirectory)
. "$PSScriptRoot/gta_support.ps1"
$folder=[IO.Path]::GetFullPath($OutputDirectory)
$workspace=[IO.Path]::GetFullPath((Split-Path (Split-Path $PSScriptRoot -Parent) -Parent))+[IO.Path]::DirectorySeparatorChar
if(-not $folder.StartsWith($workspace,[StringComparison]::OrdinalIgnoreCase)){throw 'Output must be within project'}
New-Item -ItemType Directory -Path $folder -Force | Out-Null
$y=[RageLib.ResourceWrappers.GTA5.PC.Textures.TextureDictionaryFileWrapper_GTA5_pc]::new()
$y.Load((Resolve-Path -LiteralPath $InputYtd).Path)
$rows=@()
foreach($t in $y.TextureDictionary.Textures) {
 if($t.Name -notmatch '^[a-zA-Z0-9_]+$'){throw 'Unsupported texture name'}
 $dest=Join-Path $folder ($t.Name+'.bin')
 
 $rows+= [pscustomobject]@{name=$t.Name;width=$t.Width;height=$t.Height;format=$t.Format.ToString();file=$dest;source=(Resolve-Path -LiteralPath $InputYtd).Path;pointer=$t.texture.DataPointer}
}
$rows | ConvertTo-Json -Depth 3
