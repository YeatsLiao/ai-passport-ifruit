"""Read-only focused DAT54 sound-set mapping, following CodeWalker RelFile.cs layout."""
from pathlib import Path
import struct,json
ROOT=Path(__file__).resolve().parents[1]
def joaat(s):
 h=0
 for b in s.lower().encode():
  h=(h+b)&0xffffffff;h=(h+(h<<10))&0xffffffff;h^=h>>6
 h=(h+(h<<3))&0xffffffff;h^=h>>11;return(h+(h<<15))&0xffffffff
sizes=[4,2,2,2,2,2,2,2,2,2,4,4,2,2,2,4,2,2,2,2,4,2,1,1,1,4,4,2,2,2,2,2]
def payload(b):
 flags=struct.unpack_from('<I',b,1)[0];o=5
 for bit,size in enumerate(sizes):
  if ((flags>>(8*(bit//8)))&255)!=0xaa and flags&(1<<bit):o+=size
 if o>len(b):raise ValueError('Truncated sound header')
 return b[o:]
def load(p):
 b=p.read_bytes();typ,n=struct.unpack_from('<II',b);assert typ==54
 data=b[8:8+n];o=8+n;name_len,name_count=struct.unpack_from('<II',b,o);o+=8+name_count*4
 for _ in range(name_count):o=b.index(0,o)+1
 count=struct.unpack_from('<I',b,o)[0];o+=4;nodes={}
 for i in range(count):
  key,off,length=struct.unpack_from('<III',b,o+i*12);chunk=data[off:off+length]
  if len(chunk)!=length:raise ValueError('Invalid index bound')
  nodes[key]={'type':chunk[0],'data':payload(chunk),'source':p.name,'offset':off}
 return nodes
nodes={};
for p in [ROOT/'build/reference/sounds.dat54.rel',ROOT/'build/reference/sp_sounds.dat54.rel']:nodes.update(load(p))
def trace(key,depth=0):
 if depth>12 or key not in nodes:return {'hash':f'{key:08x}','unresolved':True}
 n=nodes[key];b=n['data'];result={'hash':f'{key:08x}','type':n['type'],'source':n['source'],'offset':n['offset']}
 if n['type']==12:
  container,wave=struct.unpack_from('<II',b);result.update(container=f'{container:08x}',wave=f'{wave:08x}',slot=b[8])
 elif n['type']==1:result['child']=trace(struct.unpack_from('<I',b,6)[0],depth+1)
 elif n['type']==6:result['child']=trace(struct.unpack_from('<I',b,0)[0],depth+1)
 elif n['type']==2:
  # CodeWalker Dat54EnvelopeSound: fixed 0x4c-byte payload, child at 0x38.
  if len(b)<0x4c:raise ValueError('Truncated envelope')
  result.update(hold_ms=struct.unpack_from('<i',b,10)[0],child=trace(struct.unpack_from('<I',b,0x38)[0],depth+1))
 elif n['type']==18:
  if len(b)<40:raise ValueError('Truncated modular synth')
  synth,preset,limit,mode,tracks=struct.unpack_from('<IIfii',b)
  result.update(synth_sound=f'{synth:08x}',synth_preset=f'{preset:08x}',time_limit=limit,track_count=tracks,requires_game_synth=True)
 elif n['type'] in [7,8,13,17]:
  # Streaming has a duration field before its child list; leave unsupported unknown.
  if n['type']!=8:result['children']=[trace(struct.unpack_from('<I',b,1+i*4)[0],depth+1) for i in range(b[0])]
 else:result['payload_hex']=b.hex()
 return result
result={}
for set_name in ['Phone_SoundSet_Michael','Phone_SoundSet_Default','Phone_SoundSet_Trevor','Phone_SoundSet_Franklin']:
 key=joaat(set_name);node=nodes.get(key)
 if not node:result[set_name]={'unresolved':True,'hash':f'{key:08x}'};continue
 b=node['data'];assert node['type']==32;count=struct.unpack_from('<I',b)[0];mapping=dict(struct.unpack_from('<II',b,4+i*8) for i in range(count))
 result[set_name]={}
 for event in ['Ringtone','Menu_Navigate','Menu_Accept','Menu_Back','Text_Arrive_Tone','Dial_and_Remote_Ring','Remote_Ring','Hang_Up']:
  h=joaat(event)
  if h in mapping:result[set_name][event]=trace(mapping[h])
result['Generic_Ringtones']={f'PHONE_GENERIC_RING_{i:02}':trace(joaat(f'PHONE_GENERIC_RING_{i:02}')) for i in range(1,4)}
result['Answerphone_Beeps']={f'SFX_Phone_Generic_Answerphone_Beep_{i:02}':trace(joaat(f'Phone_Generic_Answerphone_Beep_{i:02}')) for i in range(1,6)}
(ROOT/'build/reference/phone-soundmap.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2)[:8500])
