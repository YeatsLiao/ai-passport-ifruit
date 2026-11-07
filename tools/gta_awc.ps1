param([Parameter(Mandatory=$true)][string]$InputAwc,[Parameter(Mandatory=$true)][string]$OutputAwc)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$target=[IO.Path]::GetFullPath($OutputAwc)
if(-not $target.StartsWith(([IO.Path]::GetFullPath($root)+[IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase)){throw 'Output must be within project'}
if(Test-Path -LiteralPath $target){throw 'Refusing overwrite'}
# Rockstar AWC variant documented by CodeWalker AwcFile.Decrypt_RSXXTEA.
Add-Type -TypeDefinition @"
using System;
public static class LocalAwcDecoder {
 public static void Decode(byte[] data,byte[] keyBytes) {
  if(data.Length%4!=0 || keyBytes.Length!=16)throw new ArgumentException();
  uint[] key=new uint[4];Buffer.BlockCopy(keyBytes,0,key,0,16);
  uint[] blocks=new uint[data.Length/4];Buffer.BlockCopy(data,0,blocks,0,data.Length);
  int n=blocks.Length;uint b=blocks[0],sum=unchecked(0x9E3779B9u*(uint)(6+52/n));
  unchecked { do {
   for(int p=n-1;p>=0;p--){uint a=blocks[(p>0?p:n)-1];b=blocks[p]-=((a>>5^b<<2)+(b>>3^a<<4))^((sum^b)+(key[p&3^(sum>>2&3)]^a^0x7B3A207F));}
   sum-=0x9E3779B9u;
  }while(sum!=0); }
  Buffer.BlockCopy(blocks,0,data,0,data.Length);
 }
}
"@
$data=[IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $InputAwc).Path)
$key=[IO.File]::ReadAllBytes((Join-Path $root 'build/reference/.codewalker_awckey'))
$wasWholeEncrypted=[Text.Encoding]::ASCII.GetString($data,0,4) -ne 'ADAT'
if($wasWholeEncrypted){[LocalAwcDecoder]::Decode($data,$key)}
if([Text.Encoding]::ASCII.GetString($data,0,4) -ne 'ADAT'){throw 'AWC decryption failed header validation'}
$flags=[BitConverter]::ToUInt16($data,6)
if(($flags -band 8) -and -not $wasWholeEncrypted) {
 if($flags -band 4){throw 'Multichannel encrypted AWC not implemented'}
 $count=[BitConverter]::ToInt32($data,8);$infoStart=16;if($flags -band 1){$infoStart+=$count*2}
 $chunks=0;for($i=0;$i -lt $count;$i++){$chunks+=([BitConverter]::ToUInt32($data,$infoStart+$i*4) -shr 29)}
 $chunkStart=$infoStart+$count*4
 for($i=0;$i -lt $chunks;$i++) {
  $info=[BitConverter]::ToUInt64($data,$chunkStart+$i*8);$type=$info -shr 56
  if($type -ne 0x55){continue}
  $offset=[int]($info -band 0xfffffff);$length=[int](($info -shr 28) -band 0xfffffff)
  if($offset+$length -gt $data.Length){throw 'Invalid audio chunk bound'}
  if($length -eq 0){continue}
  $chunk=[byte[]]::new($length);[Buffer]::BlockCopy($data,$offset,$chunk,0,$length)
  [LocalAwcDecoder]::Decode($chunk,$key);[Buffer]::BlockCopy($chunk,0,$data,$offset,$length)
 }
}
# Whole-file encryption already includes sample bytes; do not decrypt those a second time.
$data[6]=[byte]($data[6] -band 0xf7)
[IO.File]::WriteAllBytes($target,$data)
Write-Output 'Decoded AWC envelope; no key material exported'
