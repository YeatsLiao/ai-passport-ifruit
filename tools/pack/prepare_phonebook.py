"""Generate phonebook and device sequence plan from verified local catalogs."""
from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[2]
def main():
 book=json.loads((ROOT/'assets/catalog/phonebook.json').read_text(encoding='utf-8'))
 taxi=json.loads((ROOT/'assets/catalog/taxi-calls.json').read_text(encoding='utf-8'))['roles']
 titles=json.loads((ROOT/'assets/catalog/story-call-titles.json').read_text(encoding='utf-8'))
 scenes=json.loads((ROOT/'assets/catalog/call-scenes.json').read_text(encoding='utf-8'))['scenes']+json.loads((ROOT/'assets/catalog/call-scenes-extra.json').read_text(encoding='utf-8'))['scenes']
 outgoing_file=ROOT/'assets/catalog/call-scenes-outgoing.json'
 if outgoing_file.exists():scenes+=json.loads(outgoing_file.read_text(encoding='utf-8'))['scenes']
 branches=ROOT/'assets/catalog/call-scenes-branches.json'
 if branches.exists():scenes+=json.loads(branches.read_text(encoding='utf-8'))['scenes']
 contacts=book['contacts'];roles=['Michael','Franklin','Trevor'];lookup={c['native_id']:i for i,c in enumerate(contacts)};plan={};crows=[]
 def seq(lines):
  out=[]
  for line in lines:
   if line.get('asset_id'):out.append({'asset_id':line['asset_id']})
   elif 'pause_ms' in line:out.append({'pause_ms':line['pause_ms']})
   elif line.get('sound','').startswith('SFX_PAUSE_'):out.append({'pause_ms':int(line['sound'].rsplit('_',1)[1])})
  return out
 for c in contacts:
  conv=c.get('answerphone_conversation');answer=next((a for a in c['answerphones'] if a['conversation']==conv and a['complete_recorded_speech']),None)
  path=''
  if answer:
   name='ap_'+conv.lower();plan[name]=seq(answer['sequence']);path='/assets/'+name+'.wav'
  number=c['number'] if len(c['number'])==10 and c['number'].isascii() and c['number'].isdecimal() else ''
  crows.append((str(c['native_id']),*(json.dumps(v) for v in [c['name'],c['portrait_key'],path]),json.dumps(number)))
 role_rows=[]
 for r in range(3):
  rows=[]
  for contact,row in zip(contacts,crows):
   scene_id=next((i for i,scene in enumerate(scenes) if scene.get('incoming') is False and scene['native_contact_id']==contact['native_id'] and scene['role_mask']&(1<<r)),-1)
   rows.append('{'+','.join((*row[:-1],str(scene_id),row[-1]))+'}')
  role_rows.append('{'+','.join(rows)+'}')
 lines=['#include "phonebook.h"','#include <stddef.h>','static const phone_contact_t contacts[3]['+str(len(contacts))+'] = {'+','.join(role_rows)+'};']
 lists=[]
 for r,role in enumerate(roles):
  ids=set(book['roles'][role]);ids.update(s['native_contact_id'] for s in scenes if s.get('role_mask',1 if s.get('role')=='Michael' else 0)&(1<<r))
  indices=[i for i,c in enumerate(contacts) if c['native_id'] in ids]
  lists.append(indices);lines.append(f'static const unsigned role_{r}[] = {{'+','.join(map(str,indices))+'};')
 lines.append('static const unsigned *const role_ids[3]={role_0,role_1,role_2};')
 lines.append('static const unsigned role_counts[3]={'+','.join(str(len(x)) for x in lists)+'};')
 srows=[]
 for i,s in enumerate(scenes):
  name='call_'+str(i);plan[name]=seq(s['lines']);mask=s.get('role_mask',1);task=1 if i==0 else -1
  incoming=s.get('incoming',True)
  title=titles[s['id']] if incoming else ''
  caller_name=portrait_key=''
  if incoming:
   contact=lookup.get(s['native_contact_id'])
   if contact is None:
    caller_name,portrait_key=s['contact_name'],s['portrait_name']
   else:
    caller_name,portrait_key=contacts[contact]['name'],contacts[contact]['portrait_key']
  srows.append('{'+','.join((json.dumps(s['id']),str(s['native_contact_id']),str(mask),
      'true' if incoming else 'false',str(task),json.dumps('/assets/'+name+'.wav'),
      str(s.get('outcome',0)),json.dumps(title),json.dumps(caller_name),json.dumps(portrait_key)))+'}')
 lines+=['static const call_scene_t scenes[] = {'+','.join(srows)+'};','unsigned phonebook_count(unsigned r){return r<3?role_counts[r]:0;}','const phone_contact_t *phonebook_contact(unsigned r,unsigned i){return i<phonebook_count(r)?&contacts[r][role_ids[r][i]]:NULL;}','int phonebook_find(unsigned r,unsigned id){for(unsigned i=0;i<phonebook_count(r);i++)if(phonebook_contact(r,i)->native_id==id)return (int)i;return -1;}','int call_scene_outgoing(unsigned r,unsigned id,unsigned outcome){for(unsigned i=0;i<sizeof(scenes)/sizeof(scenes[0]);i++)if(!scenes[i].incoming && scenes[i].contact_native_id==id && (scenes[i].role_mask&(1u<<r)) && scenes[i].outcome==outcome)return (int)i;return -1;}', 'unsigned call_scene_count(void){return sizeof(scenes)/sizeof(scenes[0]);}','const call_scene_t *call_scene_get(unsigned i){return i<call_scene_count()?&scenes[i]:NULL;}','int call_scene_default_incoming(unsigned r){for(unsigned i=0;i<call_scene_count();i++)if(scenes[i].incoming && (scenes[i].role_mask&(1u<<r)))return (int)i;return -1;}']
 taxi_paths=[]
 for r,role in enumerate(roles):
  name=f'taxi_{r}'
  plan[name]=seq(taxi[role])
  if len(plan[name])!=3:raise ValueError('Missing original taxi dialogue for '+role)
  taxi_paths.append(json.dumps('/assets/'+name+'.wav'))
 lines.append('static const char *const taxi_paths[3]={'+','.join(taxi_paths)+'};')
 lines.append('const char *phone_taxi_voice_file(unsigned r){return r<3?taxi_paths[r]:NULL;}')
 folder=ROOT/'.local/generated';folder.mkdir(exist_ok=True,parents=True);(folder/'phonebook_data.inc').write_text('\n'.join(lines)+'\n',encoding='utf-8')
 (folder/'phone-audio-plan.json').write_text(json.dumps(plan,indent=2)+'\n')
 print('Phonebook',len(contacts),'roles',list(map(len,lists)),'calls',len(scenes),'answerphones',sum(n.startswith('ap_') for n in plan))
if __name__=='__main__':main()
