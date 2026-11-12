param([Parameter(Mandatory=$true)][string]$Archive,[string]$Member,[string]$Output,[string]$LogicalName)
. "$PSScriptRoot/gta_support.ps1"
$source=(Resolve-Path -LiteralPath $Archive).Path
$stream=[IO.File]::Open($source,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read)
try {
 if(-not $LogicalName){$LogicalName=[IO.Path]::GetFileName($source)}
 $container=[RageLib.GTA5.ArchiveWrappers.RageArchiveWrapper7]::Open($stream,$LogicalName,$true)
 if($Member) {
  if(-not $Output) {throw 'Output required'}
  $target=[IO.Path]::GetFullPath($Output)
  $workspace=[IO.Path]::GetFullPath((Split-Path (Split-Path $PSScriptRoot -Parent) -Parent))+[IO.Path]::DirectorySeparatorChar
  if(-not $target.StartsWith($workspace,[StringComparison]::OrdinalIgnoreCase)) {throw 'Output must be within project'}
  if(Test-Path -LiteralPath $target) {throw 'Refusing overwrite'}
  $parts=$Member.Replace('\','/').Split('/'); $dir=$container.Root
  for($i=0;$i -lt $parts.Length-1;$i++) { $dir=$dir.GetDirectory($parts[$i]); if(-not $dir){throw 'Directory not found'} }
  $file=$dir.GetFile($parts[-1]); if(-not $file){throw 'Member not found'}
  if(($file -is [RageLib.Archives.IArchiveBinaryFile]) -and ($file.IsEncrypted -or $file.IsCompressed)) {
   $ms=[IO.MemoryStream]::new(); $file.Export($ms); [byte[]]$bytes=$ms.ToArray();$ms.Dispose()
   if($file.IsEncrypted) {
    $stream.Position=12; $reader=[IO.BinaryReader]::new($stream,[Text.Encoding]::UTF8,$true); $encryption=$reader.ReadUInt32();$reader.Dispose()
    if($encryption -eq 0x0FFFFFF9) {$bytes=[RageLib.GTA5.Cryptography.GTA5Crypto]::DecryptAES($bytes)}
    else {$bytes=[RageLib.GTA5.Cryptography.GTA5Crypto]::DecryptNG($bytes,$file.Name,[uint32]$file.UncompressedSize)}
   }
   if($file.IsCompressed) {
    $ms=[IO.MemoryStream]::new($bytes,$false);$deflate=[IO.Compression.DeflateStream]::new($ms,[IO.Compression.CompressionMode]::Decompress)
    $outputStream=[IO.MemoryStream]::new();$deflate.CopyTo($outputStream);$bytes=$outputStream.ToArray();$outputStream.Dispose();$deflate.Dispose();$ms.Dispose()
   }
   if($bytes.Length -ne $file.UncompressedSize){throw 'Exported binary size mismatch'}
   [IO.File]::WriteAllBytes($target,$bytes)
  } else {$file.Export($target)}
  Get-Item -LiteralPath $target | Select-Object Name,Length
 } else {
  function Walk($dir,[string]$prefix) {
   foreach($f in $dir.GetFiles()) { [pscustomobject]@{path=($prefix+$f.Name);size=$f.Size} }
   foreach($d in $dir.GetDirectories()) {Walk $d ($prefix+$d.Name+'/')}
  }
  Walk $container.Root '' | ConvertTo-Json -Depth 3
 }
 $container.Dispose()
} finally {$stream.Dispose()}
