"""Extract original story mail/SMS with native sender and recipient metadata; no invented prose."""
from pathlib import Path
import collections,hashlib,html,json,re
from tools.extract.resolve_answerphone import gxt,joaat
ROOT=Path(__file__).resolve().parents[2]
REF=ROOT/'.local/reference'
ARCHIVE='update/update2.rpf/x64/levels/gta5/script/script_rel.rpf'
ROLES=['Michael','Franklin','Trevor']

def clean(s):
    s=re.sub(r'(?i)<br\s*/?>','\n',s.replace('~n~','\n').replace('~nrt~','\n'))
    s=re.sub(r'<[^>]*>','',s)
    # Keep parameter placeholders visible; remove formatting controls only.
    s=re.sub(r'~(?:[bgrysuw]|h)~','',s)
    return re.sub(r'\n{3,}','\n\n',html.unescape(s).replace('\r','')).strip()

def args(s):
    result=[];start=depth=0;quote=False
    for i,c in enumerate(s):
        if c=='"' and (i==0 or s[i-1]!='\\'):quote=not quote
        if quote:continue
        if c=='(':depth+=1
        if c==')':depth-=1
        if c==',' and depth==0:result.append(s[start:i].strip());start=i+1
    result.append(s[start:].strip());return result

def main():
    text_path=ROOT/'assets/ui/global_american.gxt2'
    text=gxt(text_path)
    get=lambda key:text.get(joaat(key),'')
    email=(REF/'email_controller.ysc.full.c').read_text(encoding='utf-8')
    comms=(REF/'comms_controller.ysc.full.c').read_text(encoding='utf-8')
    initial=(REF/'initial.ysc.full.c').read_text(encoding='utf-8')
    registry=(REF/'standard_global_reg.ysc.full.c').read_text(encoding='utf-8')
    roles={r:{'emails':[],'texts':[],'web':[]} for r in ROLES}
    native_names={}
    for line in registry.splitlines():
        m=re.search(r'func_691\((.*)\);',re.sub(r'/\*.*?\*/','',line))
        if not m:continue
        a=args(m[1])
        if a[0].isdigit() and len(a)>4:
            key=a[3].strip('"');native_names[int(a[0])]=get(key) or key
    email_senders={}
    function=email[email.index('char* func_4('):email.index('const char* func_5(')]
    for m in re.finditer(r'case (\d+):\s*return "([^"]+)"',function):
        email_senders[int(m[1])]=get(m[2]) or m[2]
    records={}
    for m in re.finditer(r'Global_46166\[(\d+) /\*12\*/\](?:\.f_(\d+))? = (-?\d+);',email):
        records.setdefault(int(m[1]),{})[int(m[2] or 0)]=int(m[3])
    for n,r in records.items():
        if r.get(3) not in range(3) or 1 not in r:continue
        title_key='EMSTR_'+str(r[0]);body_key='EMSTR_'+str(r[1]);raw=get(body_key)
        if len(raw)<=25 or raw=='Dynamic E-Mail':continue
        role=ROLES[r[3]]
        roles[role]['emails'].append(dict(id=f'email_{n}',sender=email_senders.get(r.get(2),f'Email contact {r.get(2)}'),
            sender_native_id=r.get(2),title=clean(get(title_key)),body=clean(raw),raw_body=raw,source_key=body_key,
            subject_key=title_key,archive=ARCHIVE+'/email_controller.ysc',text_archive='x64b.rpf/data/lang/american_rel.rpf/global.gxt2',
            evidence=f'func_69 Global_46166[{n}]: f_2 sender, f_3 recipient; appEmail native view reads base and f_1',
            stage='Original story archive; unlocked together for offline browsing, not chronological inbox',
            unresolved_variables=bool(re.search(r'~(?:a|[0-9]+)~',raw))))
    sms=comms[comms.index('void func_72('):comms.index('int func_73(')]
    message_keys={int(m[1]):m[2] for m in re.finditer(r'case (-?\d+):\s*TEXT_LABEL_ASSIGN_STRING\(sParam1, "([^"]+)"',sms)}
    seen=set()
    for line_no,line in enumerate(initial.splitlines(),1):
        m=re.search(r'func_105\(([^;]+)\);',line)
        if not m:continue
        try:a=[int(x.strip()) for x in m[1].split(',')]
        except ValueError:continue
        mask,sender,message=a[0],a[1],a[3];key=message_keys.get(message)
        if not key or not get(key) or not 1<=mask<=7:continue
        for role_id,role in enumerate(ROLES):
            if not mask&(1<<role_id) or (role,key,sender) in seen:continue
            seen.add((role,key,sender));raw=get(key)
            roles[role]['texts'].append(dict(id=f'text_{key}',sender=native_names.get(sender,f'Contact {sender}'),
                sender_native_id=sender,title=clean(raw).split('\n')[0][:60],body=clean(raw),raw_body=raw,source_key=key,
                archive=ARCHIVE+'/comms_controller.ysc',text_archive='x64b.rpf/data/lang/american_rel.rpf/global.gxt2',
                evidence=f'initial.ysc func_105 line {line_no}: recipient_mask={mask}, sender={sender}, message_id={message}; comms_controller func_72 maps id to key',
                stage='Original story archive; browse collection, not an assertion these events are simultaneously unlocked',
                unresolved_variables=bool(re.search(r'~(?:a|[0-9]+)~',raw))))
    domains={}
    for bucket in roles.values():
        for item in bucket['emails']:
            for domain in re.findall(r'www\.[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}',item['raw_body']):
                domains.setdefault(domain.lower(),item['source_key'])
    # A verified address directory, not fabricated copies of the sites' page bodies.
    for role in ROLES:
        roles[role]['web']=[dict(id='web_'+re.sub('[^a-z0-9]','_',domain),sender='Eyefind',title=domain,body=domain,
            source_key=key,archive='x64b.rpf/data/lang/american_rel.rpf/global.gxt2',public=True,directory_only=True,
            stage='Public game website address found in original mail; full site page not yet extracted') for domain,key in sorted(domains.items())]
    pages=[]
    for bank in ['NWS_AR2','NWS_AR3','NWS_ASS1','NWS_ASS2','NWS_ASS3','NWS_ASS4']:
        path=REF/(bank.lower()+'.gxt2')
        if not path.exists():continue
        news=gxt(path)
        for n in range(1,16):
            headline_key=f'{bank}_HL{n}';body_key=f'{bank}_S{n}'
            title=news.get(joaat(headline_key));raw=news.get(joaat(body_key))
            if not title or not raw:continue
            byline=clean(raw.split('<br>',1)[0])
            pages.append(dict(id=f'news_{bank.lower()}_{n}',sender=byline,title=clean(title),body=clean(raw),raw_body=raw,
                source_key=body_key,subject_key=headline_key,archive=f'x64b.rpf/data/lang/american_rel.rpf/{bank.lower()}.gxt2',
                evidence='appinternet.ysc func_9136/9137 loads NWS bank; native news selection uses bank_HL{n} and bank_S{n}',
                public=True,directory_only=False,stage='Offline news archive spanning original plot stages; not simultaneously current headlines'))
    for role in ROLES:roles[role]['web']=pages+roles[role]['web']
    output=dict(schema=1,source_sha256=hashlib.sha256(text_path.read_bytes()).hexdigest(),roles=roles,
        limitations=['Story archive spans multiple plot stages; not a chronological ready-to-trigger event pool.',
                    'Dynamic-email placeholder bodies excluded; variable placeholders preserved and flagged.',
                    'Web news pages contain original article text; directory_only entries remain address-only.',
                    'News archive spans multiple plot stages and does not assert all headlines are concurrently current.'])
    (ROOT/'assets/catalog/story-content.json').write_text(json.dumps(output,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({r:{k:len(v) for k,v in bucket.items()} for r,bucket in roles.items()},indent=2))
if __name__=='__main__':main()

