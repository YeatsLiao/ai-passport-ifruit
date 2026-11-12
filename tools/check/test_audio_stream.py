from pathlib import Path
import subprocess,imageio_ffmpeg,os
r=Path('.');out=r/'.local/gsm-test';out.mkdir(parents=True,exist_ok=True)
os.environ['PATH']='D:/1.Soft/msys64/mingw64/bin;'+os.environ['PATH']
sources=[str(p) for p in (r/'firmware/components/gsm/src').glob('*.c')]
subprocess.run(['gcc','-O2','-DWAV49','-DSASR','-Ifirmware/components/gsm/include','-Ifirmware/main',*sources,'firmware/main/wav.c','tests/wav_decode.c','-o',str(out/'wav_decode.exe')],check=True)
for file in sorted(Path('.local/audio').glob('*.wav')):
 if file.name.startswith(('call_','ap_')):subprocess.run([str(out/'wav_decode.exe'),str(file),str(out/'ours.raw')],check=True)
file=Path('.local/audio/call_0.wav');subprocess.run([str(out/'wav_decode.exe'),str(file),str(out/'ours.raw')],check=True)
subprocess.run([imageio_ffmpeg.get_ffmpeg_exe(),'-y','-v','error','-i',str(file),'-f','s16le',str(out/'ref.raw')],check=True)
a=(out/'ours.raw').read_bytes();b=(out/'ref.raw').read_bytes();assert a==b[:len(a)] and 0<=len(b)-len(a)<640,(len(a),len(b))
print('PASS:',sum(p.name.startswith(('call_','ap_')) for p in Path('.local/audio').glob('*.wav')),'compressed conversations stream-decode; complete call PCM matches FFmpeg')
