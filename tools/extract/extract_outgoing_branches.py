"""Export verified native declined/recent-phone Hang Out branches without inventing dialogue."""
from pathlib import Path
import json,subprocess,wave
from tools.extract.extract_story_calls import extract,ps,REF,ROOT,GAME,VGM,digest
from tools.extract.resolve_answerphone import gxt,joaat,streams
pairs=['MF','MT','MJ','MA','FM','FT','FL','FJ','TM','TF','TL','TJ']
native={'M':0,'F':1,'T':2,'L':19,'J':14,'A':17}
m=json.loads((ROOT/'assets/catalog/manifest.json').read_text(encoding='utf-8'));ids={x['id'] for x in m['assets']};scenes=[]
for pair in pairs:
 bank='fp'+pair.lower();prefix='Fp'+pair;text=gxt(REF/(bank+'au.gxt2'))
 for branch,part in [(1,'N1_ok'),(2,'N1_ph')]:
  lines=[]
  for suffix in ['G1','G2',part,'N2']:
   key=prefix+'_'+suffix;sound=text.get(joaat(key+'a'));assert sound,key
   audio_bank=sound.split('_')[0].lower();plain=REF/('plain_'+audio_bank+'.awc')
   if not plain.exists():
    awc=REF/(audio_bank+'.awc');extract(GAME/'x64/audio/sfx/SS_FF.rpf',audio_bank+'.awc',awc);ps('gta_awc.ps1','-InputAwc',awc,'-OutputAwc',plain)
   wavename=sound.lower()+'_01';stream=streams(plain)[joaat(wavename)&0x1fffffff];aid='gta_outgoing_'+wavename;dest=ROOT/'assets/voices'/(aid+'.wav')
   if not dest.exists():subprocess.run([str(VGM),'-i','-s',str(stream),'-o',str(dest),str(plain)],check=True,stdout=subprocess.DEVNULL)
   with wave.open(str(dest)) as w:meta=dict(sample_rate=w.getframerate(),channels=w.getnchannels(),sample_width=w.getsampwidth(),duration_seconds=w.getnframes()/w.getframerate())
   if aid not in ids:m['assets'].append(dict(id=aid,category='voices',path='voices/'+dest.name,sha256=digest(dest),source='GTA V Legacy / native friends_controller Hang Out branch / vgmstream PCM',source_path=f'x64/audio/sfx/SS_FF.rpf/{audio_bank}.awc#stream:{stream}',license='Rockstar original; no redistribution authorization recorded',status='decoded',**meta));ids.add(aid)
   lines.append(dict(key=key,asset_id=aid,sound=sound,subtitles=[text[joaat(key+'_01')]],duration_seconds=meta['duration_seconds']))
  scenes.append(dict(id='out_'+bank+'_'+part.lower(),role_mask=1<<native[pair[0]],native_contact_id=native[pair[1]],incoming=False,outcome=branch,conversation='Declined invitation' if branch==1 else 'Recently called refusal',lines=lines,source='friends_controller func_443: G1/G2/N1_ok or N1_ph/N2; func_433 refusal selection',conditions='Native spoken branch; local availability and recent-call timing are simulated.'))
(ROOT/'assets/catalog/manifest.json').write_text(json.dumps(m,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(ROOT/'assets/catalog/call-scenes-branches.json').write_text(json.dumps(dict(scenes=scenes),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Exported',len(scenes),'complete native outgoing branches')
