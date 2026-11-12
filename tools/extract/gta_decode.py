"""Decode exported base-mip texture bytes; does not alter game files or original YTDs."""
import argparse,json,struct,zlib
from pathlib import Path
from PIL import Image
p=argparse.ArgumentParser();p.add_argument('index',type=Path);args=p.parse_args()
items=json.loads(args.index.read_text(encoding='utf-8-sig'))
if isinstance(items,dict):items=[items]
for t in items:
    path=Path(t['file']);w,h=t['width'],t['height'];fmt=t['format']
    packed=Path(t['source']).read_bytes()
    if packed[:4]!=b'RSC7':raise ValueError('Not RSC7')
    flags=struct.unpack_from('<I',packed,8)[0]
    counts=((flags>>27)&1)+(((flags>>26)&1)<<1)+(((flags>>25)&1)<<2)+(((flags>>24)&1)<<3)+(((flags>>17)&127)<<4)+(((flags>>11)&63)<<5)+(((flags>>7)&15)<<6)+(((flags>>5)&3)<<7)+(((flags>>4)&1)<<8)
    system_size=(512<<(flags&15))*counts
    inflated=zlib.decompress(packed[16:],-15)
    size=w*h*4 if fmt=='D3DFMT_A8R8G8B8' else ((w+3)//4)*((h+3)//4)*(8 if fmt=='D3DFMT_DXT1' else 16)
    offset=system_size+int(t['pointer'])-0x60000000
    if offset<system_size or offset+size>len(inflated):raise ValueError('Texture pointer outside graphics data')
    raw=inflated[offset:offset+size];path.write_bytes(raw)
    if fmt=='D3DFMT_A8R8G8B8': image=Image.frombytes('RGBA',(w,h),raw,'raw','BGRA')
    elif fmt in ['D3DFMT_DXT1','D3DFMT_DXT3','D3DFMT_DXT5']:
        header=struct.pack('<7I',124,0x81007,h,w,len(raw),0,1)+bytes(44)
        header+=struct.pack('<II4s5I',32,4,fmt[-4:].encode(),0,0,0,0,0)+struct.pack('<5I',0x1000,0,0,0,0)
        dds=path.with_suffix('.dds');dds.write_bytes(b'DDS '+header+raw);image=Image.open(dds)
    else: raise ValueError(fmt)
    image.save(path.with_suffix('.png'));print(t['name'],image.size,'unique bytes',len(set(raw)))
