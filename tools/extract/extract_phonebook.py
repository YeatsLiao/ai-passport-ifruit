"""Extract original story-contact metadata and answerphone sequences without inventing unlocks."""
from pathlib import Path
import hashlib,json,re
from PIL import Image
from tools.extract.extract_story_content import args
from tools.extract.resolve_answerphone import gxt,joaat
ROOT=Path(__file__).resolve().parents[2]

SPECIAL={
 12:[('LES_APH2','default story answerphone')],
 13:[('LES_APH2','appContacts CHAR_LESTER_DEATHWISH redirects to CHAR_LESTER / LES_APH2; registered LES_APH1 is not the final runtime branch')],
 14:[('JIM_APH1','!func_8(19)'),('JIM_APH3','func_8(19) && !func_8(21)'),('JIM_APH2','func_8(19) && func_8(21) && !func_8(22)'),('JIM_APH4','remaining story state')],
 15:[('TRA_APH1','!func_8(19)'),('TRA_APH2','func_8(19) && !func_8(20)'),('TRA_APH3','func_8(19) && func_8(20) && !func_8(22)'),('TRA_APH4','remaining story state')],
 17:[('AMA_APH2','func_8(21) && !func_8(22)'),('AMA_APH1','otherwise')],
 19:[('LAM_APH1','!func_8(19)'),('LAM_APH2','func_8(19) && !func_8(20)'),('LAM_APH3','otherwise')],
 20:[('RON_APH1','story mode; Online branch uses ANS_RBusy')],
 36:[('SHR_APH3','game clock hours >17 or <9'),('SHR_APH1','game clock hours >8 and <13'),('SHR_APH2','game clock hours >12 and <18')],
 44:[('DEN_APH3','func_7(126)'),('DEN_APH1','otherwise registered default')],
 49:[('BAR_APH2','func_6(7)'),('BAR_APH1','otherwise')]
}
def main():
 text=gxt(ROOT/'assets/ui/global_american.gxt2')
 get=lambda k:text.get(joaat(k),'')
 registry=(ROOT/'.local/reference/standard_global_reg.ysc.full.c').read_text(encoding='utf-8')
 mapping=json.loads((ROOT/'assets/catalog/answerphone-map.json').read_text(encoding='utf-8'))
 rows={}
 for no,line in enumerate(registry.splitlines(),1):
  match=re.search(r'func_691\((.*)\);',re.sub(r'/\*.*?\*/','',line))
  if not match:continue
  params=args(match[1])
  if len(params)!=16 or not params[0].isdigit():continue
  native=int(params[0]);conv=params[8].strip('"')
  # Main story NPC default answerphones, plus three players and story figures without voicemail.
  if not ((native<=69 and native not in [3,13,22,41] and not conv.startswith('NO_')) or native in [0,1,2,40,46,47,53,66,93,103,131]):continue
  name_key=params[3].strip('"');portrait_label=params[4].strip('"');portrait_key=get(portrait_label) or portrait_label
  portrait_key=portrait_key.lower();paths=[ROOT/'assets/Chars'/(portrait_key+'.png'),ROOT/'assets/portraits'/(portrait_key+'.png')]
  portrait=next((p for p in paths if p.exists()),None)
  portrait_status='present' if portrait else 'missing_original_named_texture'
  if native==12:portrait_status='original_generic_silhouette_confirmed'
  # The default placeholder can be verified by pixels instead of assuming every dark portrait is a placeholder.
  default=ROOT/'assets/Chars/char_default.png'
  if portrait and default.exists():
   with Image.open(portrait) as im,Image.open(default) as other:
    if im.convert('RGBA').tobytes()==other.convert('RGBA').tobytes():portrait_status='exact_default_placeholder'
  variants=SPECIAL.get(native,[(conv,'registered default; native story availability still required')]) if not conv.startswith('NO_') else []
  sequences=[]
  for label,condition in variants:
   lines=mapping['conversations'].get(label,[])
   sequences.append(dict(conversation=label,condition=condition,sequence=lines,
     complete_recorded_speech=bool(lines) and all('asset_id' in i for i in lines if i.get('subtitle')),
     unresolved_sounds=[i['sound'] for i in lines if i.get('unresolved_audio')]))
  rows[native]=dict(native_id=native,name=get(name_key) or name_key,name_key=name_key,sort_key=int(params[2]),
    portrait_key=portrait_key,portrait_path=portrait.relative_to(ROOT).as_posix() if portrait else None,portrait_status=portrait_status,
    number=get(params[14].strip('"')),number_key=params[14].strip('"'),registered_answerphone=conv,
    answerphone_conversation=conv if variants else None,answerphones=sequences,
    answerphone_status='native_no_answerphone' if conv.startswith('NO_') else 'mapped' if sequences and sequences[0]['sequence'] else 'unresolved_conversation',
    source=dict(archive='update/update2.rpf/x64/levels/gta5/script/script_rel.rpf/standard_global_reg.ysc',function='func_684 / func_691',decompiled_line=no),
    role_unlock_status='dynamic native f_24 changes by mission; registration is not proof of simultaneous availability')
 contacts=sorted(rows.values(),key=lambda item:(item['sort_key'],item['native_id']))
 story=json.loads((ROOT/'assets/catalog/story-content.json').read_text(encoding='utf-8'))
 email_script=(ROOT/'.local/reference/email_controller.ysc.full.c').read_text(encoding='utf-8')
 icon_function=email_script[email_script.index('const char* func_5('):].split('\n}',1)[0]
 email_native={}
 for match in re.finditer(r'case (\d+):(?P<body>.*?)(?=\n\s*case |\n\s*default:|\Z)',icon_function,re.S):
  native=re.search(r'Global_2349\[(\d+) /\*29\*/\]',match['body'])
  if native:email_native[int(match[1])]=int(native[1])
 roles={};evidence={}
 by_name={r['name'].casefold():r['native_id'] for r in contacts}
 for role_id,role in enumerate(['Michael','Franklin','Trevor']):
  ids={0,1,2}-{role_id};proof={i:['other playable protagonist; offline archive visibility policy'] for i in ids}
  ids.add(131);proof[131]=['Story mode taxi dispatch; registered for all three protagonists']
  for item in story['roles'][role]['texts']:
   n=item['sender_native_id']
   if n in rows and n!=role_id:ids.add(n);proof.setdefault(n,[]).append(item['source_key'])
  for item in story['roles'][role]['emails']:
   n=email_native.get(item.get('sender_native_id'),by_name.get(item['sender'].casefold()))
   if n in rows and n!=role_id:ids.add(n);proof.setdefault(n,[]).append(item['source_key'])
  roles[role]=[r['native_id'] for r in contacts if r['native_id'] in ids]
  evidence[role]={str(n):sorted(set(keys)) for n,keys in proof.items()}
 output=dict(schema=1,contacts=contacts,roles=roles,role_evidence=evidence,archive_native_ids=[r['native_id'] for r in contacts],
  limitations=['Role lists use observed original correspondence plus player-contact archive policy, not full native story unlock simulation.',
    'Contacts without an original answerphone remain without one; NO_ANSMSG never receives an invented recording.',
    'Original pause and synthesized beep labels are retained in sequence; unresolved beep is explicitly listed, not replaced.',
    'Script conditions retain native function/flag notation rather than guessing unsupported story-state names.'])
 (ROOT/'assets/catalog/phonebook.json').write_text(json.dumps(output,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 print(len(contacts),'story contacts; role lists',{k:len(v) for k,v in roles.items()})
 print('answerphone statuses',{status:sum(r['answerphone_status']==status for r in contacts) for status in set(r['answerphone_status'] for r in contacts)})
if __name__=='__main__':main()
