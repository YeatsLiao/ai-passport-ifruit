"""Offline native DAT10 dial/remote-ring rendition for the local 3s connection demo."""
from pathlib import Path
import struct, subprocess, array, json, hashlib, wave
from tools.extract.render_phone_synth import programs, OUT, ROOT
from tools.pack import assets as a

def render(key, overrides, samples, name):
    b=bytearray(programs()[key]);o=36+struct.unpack_from('<I',b,24)[0]
    n=struct.unpack_from('<I',b,o)[0];o+=4+n*4
    n=struct.unpack_from('<I',b,o)[0];o+=4
    for i in range(n):
        k=struct.unpack_from('<I',b,o+i*8)[0]
        if k in overrides:struct.pack_into('<f',b,o+i*8+4,overrides[k])
    path=OUT/(name+'.dat');path.write_bytes(b);raw=path.with_suffix('.f32')
    subprocess.run(['dotnet',str(OUT/'bin/Release/net10.0/Render.dll'),str(path),str(raw),str((samples+255)//256)],check=True)
    pcm=array.array('f');pcm.frombytes(raw.read_bytes());return pcm[:samples]

def main():
    nodes=programs();preset=nodes[0xa8b09e12]
    values={struct.unpack_from('<I',preset,9+i*12)[0]:struct.unpack_from('<f',preset,13+i*12)[0] for i in range(preset[8])}
    dial=render(0x713b54cf,values,43200,'native-dial')
    # DAT54 VariableBlock maps exposed remote-ring parameters to native synth variables.
    remote=render(0xa9cb8bfc,{0x0b8b8d69:.1,0x1768b047:.1,0x8fd9f2d4:1.,0x9edb8569:.2,0x24672fd4:.1},48000,'native-remote')
    pcm=dial+array.array('f',[0.])*9600+remote+array.array('f',[0.])*43200
    assert max(map(abs,pcm))>1e-4 and max(map(abs,pcm))<=1.01
    raw=OUT/'dial-sequence.f32';raw.write_bytes(pcm.tobytes())
    import imageio_ffmpeg
    dest=ROOT/'assets/sounds/native_dial.wav'
    subprocess.run([imageio_ffmpeg.get_ffmpeg_exe(),'-y','-v','error','-f','f32le','-ar','48000','-ac','1','-i',str(raw),'-ar','16000','-c:a','pcm_s16le',str(dest)],check=True)
    p=ROOT/'assets/catalog/manifest.json';m=json.loads(p.read_text(encoding='utf-8'))
    item=dict(id='gta_native_dial',category='sounds',path='sounds/native_dial.wav',sha256=a.digest(dest),source='Local GTA V Legacy DAT10 bytecode / CodeWalker offline DSP rendition',source_path='optamp.dat10.rel#713b54cf,a8b09e12,a9cb8bfc; sounds.dat54.rel#Dial_and_Remote_Ring',license='Rockstar original program; no redistribution authorization recorded',status='rendered',**a.wav_info(dest))
    m['assets']=[x for x in m['assets'] if x['id']!=item['id']]+[item];a.validate(m);p.write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    p=ROOT/'assets/catalog/playback.json';m=json.loads(p.read_text(encoding='utf-8'));m['dial']='gta_native_dial';p.write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    (ROOT/'assets/catalog/native-dial.json').write_text(json.dumps(dict(programs=['713b54cf','a9cb8bfc'],preset='a8b09e12',dial_ms=900,native_gap_ms=200,remote_ring_ms=1000,local_wait_ms=3000,limitations='Original programs and parameters rendered offline; remote audible branch selected for demo, native random silence branch not replayed; proprietary DSP/envelope equivalence not asserted.'),indent=2)+'\n',encoding='utf-8')
    print('Prepared original-program dial rendition')
if __name__=='__main__':main()
