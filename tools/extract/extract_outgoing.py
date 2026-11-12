"""Extract original player-dialled Hang Out invitations from friends_controller."""
from pathlib import Path
import json,subprocess,wave,sys
from tools.extract.extract_story_calls import extract,ps,REF,ROOT,GAME,VGM,digest
from tools.extract.resolve_answerphone import gxt,joaat,streams
pairs=['MF','MT','MJ','MA','FM','FT','FL','FJ','TM','TF','TL','TJ']
native={'M':0,'F':1,'T':2,'L':19,'J':14,'A':17};roles={'M':'Michael','F':'Franklin','T':'Trevor'}
manifest=json.loads((ROOT/'assets/catalog/manifest.json').read_text(encoding='utf-8'));ids={x['id'] for x in manifest['assets']};scenes=[]
for pair in pairs:
 bank='fp'+pair.lower();prefix='Fp'+pair;textfile=REF/(bank+'au.gxt2');awc=REF/(bank+'.awc');plain=REF/('plain_'+bank+'.awc')
 extract(REF/'american_rel.rpf',bank+'au.gxt2',textfile)
 extract(GAME/'x64/audio/sfx/SS_FF.rpf',bank+'.awc',awc)
 if not plain.exists():ps('gta_awc.ps1','-InputAwc',awc,'-OutputAwc',plain)
 text=gxt(textfile);stream_ids=streams(plain);lines=[]
 for part in ['G1','G2','Y1_ok','Y2_ok']:
  key=prefix+'_'+part;sound=text.get(joaat(key+'a'))
  if not sound:raise ValueError('Missing native segment '+key)
  audio_bank=sound.split('_')[0].lower()
  if audio_bank!=bank:
   audio_awc=REF/(audio_bank+'.awc');audio_plain=REF/('plain_'+audio_bank+'.awc')
   extract(GAME/'x64/audio/sfx/SS_FF.rpf',audio_bank+'.awc',audio_awc)
   if not audio_plain.exists():ps('gta_awc.ps1','-InputAwc',audio_awc,'-OutputAwc',audio_plain)
   ids_for_sound=streams(audio_plain)
  else:audio_plain=plain;ids_for_sound=stream_ids
  wave_name=sound.lower()+'_01';hash=joaat(wave_name)&0x1fffffff;stream=ids_for_sound[hash];aid='gta_outgoing_'+wave_name;dest=ROOT/'assets/voices'/(aid+'.wav')
  if not dest.exists():subprocess.run([str(VGM),'-i','-s',str(stream),'-o',str(dest),str(audio_plain)],check=True,stdout=subprocess.DEVNULL)
  with wave.open(str(dest)) as w:meta=dict(sample_rate=w.getframerate(),channels=w.getnchannels(),sample_width=w.getsampwidth(),duration_seconds=w.getnframes()/w.getframerate())
  if aid not in ids:manifest['assets'].append(dict(id=aid,category='voices',path='voices/'+dest.name,sha256=digest(dest),source='GTA V Legacy 1.0.3889.0 / RageLib read-only export / vgmstream r2168',source_path=f'x64/audio/sfx/SS_FF.rpf/{audio_bank}.awc#stream:{stream}',license='Rockstar original; no redistribution authorization recorded',status='decoded',**meta));ids.add(aid)
  subs=[text[joaat(key+'_01')]]
  lines.append(dict(key=key,asset_id=aid,sound=sound,subtitles=subs,duration_seconds=meta['duration_seconds']))
 scenes.append(dict(id='out_'+bank,role_mask=1<<native[pair[0]],native_contact_id=native[pair[1]],direction='outgoing',incoming=False,role=roles[pair[0]],conversation='Hang Out accepted',lines=lines,source='appcontacts CELL_209 Hang Out; friends_controller func_422 -> func_432; func_89 Fp+caller+callee, func_443 G1/G2/Y1_ok/Y2_ok; func_444 accepted default 6/13',conditions='Native positive invitation branch with both friends available and normal greeting; local snapshot selects that verified branch. Not all original rejection/activity branches.'))
(ROOT/'assets/catalog/manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(ROOT/'assets/catalog/call-scenes-outgoing.json').write_text(json.dumps({'scenes':scenes},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Extracted',len(scenes),'outgoing conversations')
