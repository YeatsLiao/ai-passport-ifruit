"""Extract source-registered complete story phone sequences, retaining unresolved engine cues."""
from pathlib import Path
import json,re,subprocess,sys,wave,hashlib
from tools.extract.resolve_answerphone import gxt,joaat,streams
ROOT=Path(__file__).resolve().parents[2];REF=ROOT/'.local/reference'
GAME=Path('D:/1.Soft/Steam/steamapps/common/Grand Theft Auto V')
VGM=ROOT/'.local/tooling/vgmstream/vgmstream-cli.exe'

def ps(script,*args):
 subprocess.run(['powershell','-ExecutionPolicy','Bypass','-File',str(ROOT/'tools/extract'/script),*map(str,args)],check=True,stdout=subprocess.DEVNULL)
def extract(archive,member,out,logical=None):
 if out.exists():return
 args=['-Archive',archive,'-Member',member,'-Output',out]
 if logical:args+=['-LogicalName',logical]
 ps('gta_extract.ps1',*args)
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run():
 initial=(REF/'initial.ysc.full.c').read_text(encoding='utf-8');comms=(REF/'comms_controller.ysc.full.c').read_text(encoding='utf-8')
 part=comms[comms.index('void func_87('):];part=part[:part.index('\nvoid ',20)]
 mapping={int(i):re.findall(r'TEXT_LABEL_ASSIGN_STRING\([^,]+, "([A-Za-z0-9_]+)"',b) for i,b in re.findall(r'case (-?\d+):(.*?)(?:break;)',part,re.S)}
 selected={'ARFL_CALL1','ARFL_CALL_CT','ARFL_CALL2','CHFL_CALL1','CHFL_CALL2'}
 catalog=[];new_items={};bank_cache={};seen=set()
 bank_archives={}
 for archive in sorted((GAME/'x64/audio/sfx').glob('SS_*.rpf')):
  idx_path=REF/(archive.stem.lower().replace('_','-')+'-index.json')
  if not idx_path.exists():
   raw=subprocess.run(['powershell','-ExecutionPolicy','Bypass','-File',str(ROOT/'tools/extract/gta_extract.ps1'),'-Archive',str(archive)],check=True,stdout=subprocess.PIPE).stdout
   idx_path.write_bytes(raw)
  idx=json.loads(idx_path.read_text(encoding='utf-8-sig'))
  if isinstance(idx,dict):idx=[idx]
  for item in idx:bank_archives[Path(item['path']).stem]=archive

 manifest=json.loads((ROOT/'assets/catalog/manifest.json').read_text(encoding='utf-8'));old_ids={a['id'] for a in manifest['assets']}
 for m in re.finditer(r'func_95\((\d+), (\d+), (\d+), (\d+), (\d+), (-?\d+),',initial):
  args=list(map(int,m.groups()));mask,contact,_,_,_,message=args;names=mapping.get(message,[])
  if len(names)<2:continue
  block=names[0].lower();bank=block[:-3] if block.endswith('aud') else block
  if not bank_cache.get(bank):
   archive=bank_archives.get(bank)
   if not archive:
    if any(l in selected for l in names[1:]):raise ValueError('Bank not in SS_AC: '+bank)
    continue
   textfile=REF/(block+'.gxt2');extract(REF/'american_rel.rpf',block+'.gxt2',textfile)
   awc=REF/(bank+'.awc');extract(archive,bank+'.awc',awc)
   plain=REF/('plain_'+bank+'.awc')
   if not plain.exists():ps('gta_awc.ps1','-InputAwc',awc,'-OutputAwc',plain)
   bank_cache[bank]=(gxt(textfile),streams(plain),plain)
  text,stream_ids,plain=bank_cache[bank]
  for label in names[1:]:
   if '--all' not in sys.argv and label not in selected:continue
   if label in seen:continue
   seen.add(label);lines=[];unresolved=[]
   for n in range(1,101):
    key=f'{label}_{n}';sound=text.get(joaat(key+'a'))
    if not sound:continue
    line={'key':key,'sound_key':key+'a','sound':sound,'subtitle':text.get(joaat(key))}
    if sound.startswith('SFX_PAUSE_'):
     line['pause_ms']=int(sound.rsplit('_',1)[1])
    elif sound.startswith('SFX_OVERLAP_'):
     line['overlap_cue']=sound;line['timing_unresolved']=True
    else:
     wave_name=sound.lower()+'_01';wave_id=joaat(wave_name)&0x1fffffff;stream=stream_ids.get(wave_id)
     if stream:
      aid='gta_story_'+wave_name;dest=ROOT/'assets/voices'/(aid+'.wav')
      if not dest.exists():subprocess.run([str(VGM),'-i','-s',str(stream),'-o',str(dest),str(plain)],check=True,stdout=subprocess.DEVNULL)
      with wave.open(str(dest)) as w:meta=dict(sample_rate=w.getframerate(),channels=w.getnchannels(),sample_width=w.getsampwidth(),duration_seconds=w.getnframes()/w.getframerate())
      item=dict(id=aid,category='voices',path='voices/'+dest.name,sha256=digest(dest),source='GTA V Legacy 1.0.3889.0 / RageLib AWC read-only export / vgmstream r2168',source_path=f'x64/audio/sfx/{bank_archives[bank].name}/{bank}.awc#stream:{stream};wave:{wave_id:08x}',license='Rockstar original; no redistribution authorization recorded',status='decoded',**meta)
      new_items[aid]=item;line.update(asset_id=aid,stream=stream,wave_name=wave_name,wave_hash=f'{wave_id:08x}',duration_ms=round(meta['duration_seconds']*1000,3))
     else:line['unresolved_audio']=True;unresolved.append(sound)
    lines.append(line)
   if not lines:continue
   line_no=initial[:m.start()].count('\n')+1
   funcs=list(re.finditer(r'\nvoid (func_\d+)\(',initial[:m.start()]))
   catalog.append(dict(id=label.lower(),role_mask=mask,roles=[role for bit,role in [(1,'Michael'),(2,'Franklin'),(4,'Trevor')] if mask&bit],native_contact_id=contact,message_id=message,text_block=block.upper(),conversation=label,direction='script-registered phone event',source_registration=f'initial.ysc {funcs[-1].group(1) if funcs else "unknown"}, line {line_no}: {m.group(0)}...',source_mapping='comms_controller.ysc func_87; func_73 message type 0 (phone)',source_gxt=f'x64b.rpf/data/lang/american_rel.rpf/{block}.gxt2',lines=lines,complete_speech=not unresolved,unresolved_audio=unresolved,timing_policy='Original line order and explicit pauses retained. Overlap cues preserved, not reinterpreted. Needs branch-specific story conditions before random activation.'))
 # Preserve other workers' manifest additions by reading again only at merge time.
 latest=json.loads((ROOT/'assets/catalog/manifest.json').read_text(encoding='utf-8'));ids={a['id'] for a in latest['assets']}
 latest['assets'].extend(item for aid,item in new_items.items() if aid not in ids)
 (ROOT/'assets/catalog/manifest.json').write_text(json.dumps(latest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 result=dict(source='Original queued communication registration, text-block mapping, GXT audio labels and AWC hash matches',scenes=catalog)
 (ROOT/'assets/catalog/call-scenes-extra.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 print('Exported',len(catalog),'scenes,',len(new_items),'unique speech clips; complete speech',sum(s['complete_speech'] for s in catalog))
 for s in catalog:print(s['conversation'],s['roles'],s['native_contact_id'],round(sum(l.get('duration_ms',l.get('pause_ms',0)) for l in s['lines'])/1000,2),'seconds',s['unresolved_audio'])
if __name__=='__main__':run()
