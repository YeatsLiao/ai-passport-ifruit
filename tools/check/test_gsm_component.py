from pathlib import Path
import subprocess,shutil,sys,struct
import imageio_ffmpeg,numpy as np
root=Path('.');cc='D:/1.Soft/msys64/mingw64/bin/gcc.exe';ff=imageio_ffmpeg.get_ffmpeg_exe();out=root/'.local/gsm-test';out.mkdir(parents=True,exist_ok=True)
sources=list((root/'firmware/components/gsm/src').glob('*.c'))
subprocess.run([cc,'-std=c11','-O2','-DWAV49','-DSASR','-DNDEBUG','-Ifirmware/components/gsm/include',*[str(p) for p in sources],'tests/gsm_decode_reference.c','-o',str(out/'decode.exe')],check=True)
subprocess.run([ff,'-y','-v','error','-i','assets/voices/gta_fam1_phone_464.wav','-ar','8000','-ac','1','-c:a','gsm_ms',str(out/'sample.wav')],check=True)
b=(out/'sample.wav').read_bytes();o=12;data=None
while o+8<=len(b):
 tag=b[o:o+4];n=struct.unpack_from('<I',b,o+4)[0]
 if tag==b'data':data=b[o+8:o+8+n];break
 o+=8+n+(n&1)
assert data and len(data)%65==0
(out/'data.gsm').write_bytes(data)
subprocess.run([str((out/'decode.exe').resolve()),str(out/'data.gsm'),str(out/'ours.raw')],check=True)
subprocess.run([ff,'-y','-v','error','-i',str(out/'sample.wav'),'-f','s16le',str(out/'reference.raw')],check=True)
a=np.frombuffer((out/'ours.raw').read_bytes(),dtype=np.int16);b=np.frombuffer((out/'reference.raw').read_bytes(),dtype=np.int16)
n=min(len(a),len(b));delta=np.abs(a[:n].astype(np.int32)-b[:n]);print('samples',len(a),len(b),'max_diff',delta.max(),'rms_diff',float(np.sqrt(np.mean(delta.astype(float)**2))))
assert delta.max()<=8
