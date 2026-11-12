. "$PSScriptRoot/gta_support.ps1"
$root=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$rows=@()
foreach($category in @('icons','wallpapers','portraits')) {
 foreach($file in Get-ChildItem (Join-Path $root ('assets/'+$category)) -Filter '*.ytd') {
  $y=[RageLib.ResourceWrappers.GTA5.PC.Textures.TextureDictionaryFileWrapper_GTA5_pc]::new();$y.Load($file.FullName)
  foreach($t in $y.TextureDictionary.Textures) {
   if($t.Name -notmatch '^[a-zA-Z0-9_]+$'){throw 'Invalid texture name'}
   $output=Join-Path $root ('.local/reference/decoded-textures/'+$category+'_'+$t.Name+'.bin')
   $rows+=[pscustomobject]@{name=$t.Name;width=$t.Width;height=$t.Height;format=$t.Format.ToString();file=$output;source=$file.FullName;pointer=$t.texture.DataPointer;category=$category}
  }
 }
}
New-Item -ItemType Directory -Path (Join-Path $root '.local/reference/decoded-textures') -Force | Out-Null
$rows | ConvertTo-Json -Depth 3 | Set-Content (Join-Path $root '.local/reference/all-textures.json') -Encoding utf8
Write-Output ('Texture candidates: '+$rows.Count)
