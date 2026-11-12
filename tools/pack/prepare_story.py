"""Compile source-traced preset content to Flash tables; no runtime heap or JSON parser."""
from pathlib import Path
import json,hashlib
ROOT=Path(__file__).resolve().parents[2]
def main():
    data=json.loads((ROOT/'assets/catalog/story-content.json').read_text(encoding='utf-8'))
    roles=['Michael','Franklin','Trevor'];kinds=['emails','texts','web'];out=[];slices=[];report={}
    for r,role in enumerate(roles):
        report[role]={};row=[]
        for k,kind in enumerate(kinds):
            items=[item for item in data['roles'][role].get(kind,[]) if not item.get('unresolved_variables')]
            if any(len(item['body'].encode('utf-8'))>4096 for item in items):raise ValueError('Article requires reader pagination')
            if kind!='web':items=items[:64]
            symbol=f'story_{r}_{k}';values=[]
            for item in items:
                if not item.get('body') or not item.get('source_key'):raise ValueError('Missing preset body/source')
                display_body=item['body']
                if kind=='web':display_body=item.get('title','')+'\n\n'+display_body
                fields=[item.get('sender',''),item.get('title',''),display_body,item['source_key']]
                if any('\0' in str(field) for field in fields):raise ValueError('NUL in story content')
                values.append('{'+','.join(json.dumps(str(f),ensure_ascii=False).replace('?',r'\?') for f in fields)+'}')
            out.append(f'static const story_item_t {symbol}[] = {{'+(','.join(values) if values else '{0}')+'};')
            row.append('{'+symbol+','+str(len(items))+'}');report[role][kind]=len(items)
        slices.append('{'+','.join(row)+'}')
    out.append('static const story_slice_t story_slices[3][3] = {'+','.join(slices)+'};')
    out.append('static const uint32_t story_content_revision='+str(int.from_bytes(hashlib.sha256(json.dumps(data,sort_keys=True,ensure_ascii=False).encode()).digest()[:4],'little'))+'u;')
    fallback=json.loads((ROOT/'assets/catalog/call-scenes.json').read_text(encoding='utf-8'))['scenes'][0]['followup_sms']
    out.append('static const story_item_t story_missed_jimmy = {'+','.join(json.dumps(value,ensure_ascii=False).replace('?',r'\?') for value in [fallback['sender'],fallback['sender'],fallback['text'],fallback['key']])+'};')
    folder=ROOT/'.local/generated';folder.mkdir(parents=True,exist_ok=True)
    (folder/'story_data.inc').write_text('\n'.join(out)+'\n',encoding='utf-8')
    (folder/'story-index.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('Prepared story presets:',report)
if __name__=='__main__':main()
