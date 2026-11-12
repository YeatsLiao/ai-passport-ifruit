"""Stage prepared device inputs outside disposable .local/. Originals stay in assets/."""
from pathlib import Path
import hashlib,json,shutil
ROOT=Path(__file__).resolve().parents[2]
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def copy(src,dst):
    if not src.is_file():raise FileNotFoundError(src)
    dst.parent.mkdir(parents=True,exist_ok=True)
    if not dst.exists() or digest(src)!=digest(dst):shutil.copyfile(src,dst)
def main():
    out=ROOT/'firmware/resources'
    for name in ['ui_assets.c','ui_assets.h','ui_fonts.c','ui_fonts.h','phone_content.c','phonebook_data.inc','story_data.inc']:
        copy(ROOT/'.local/generated'/name,out/name)
    index=ROOT/'.local/audio-index.json'
    if index.exists():
        items=json.loads(index.read_text(encoding='utf-8'))
        for item in items:
            src=ROOT/'.local/audio'/item['file']
            if digest(src)!=item['sha256']:raise ValueError('Audio hash mismatch: '+item['file'])
            copy(src,out/'audio'/item['file'])
        copy(index,out/'audio-index.json')
    items=json.loads((out/'audio-index.json').read_text(encoding='utf-8'))
    expected={i['file'] for i in items}
    if {p.name for p in (out/'audio').iterdir()}!=expected:raise ValueError('Stale/missing staged audio; archive it before staging')
    for item in items:
        if digest(out/'audio'/item['file'])!=item['sha256']:raise ValueError('Staged audio differs')
    files={p.relative_to(out).as_posix():digest(p) for p in sorted(out.rglob('*')) if p.is_file() and p.name!='manifest.json'}
    (out/'manifest.json').write_text(json.dumps(dict(schema=1,files=files),indent=2)+'\n',encoding='utf-8')
    print('Staged firmware inputs:',len(files),'files; originals unchanged')
if __name__=='__main__':main()
