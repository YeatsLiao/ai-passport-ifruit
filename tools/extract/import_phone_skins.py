"""Catalog native badger/facade textures and resolve icon crops from GFX shapes."""
from pathlib import Path
import json, hashlib, shutil, xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[2]
def main():
    manifest_path=ROOT/'assets/catalog/manifest.json'
    manifest=json.loads(manifest_path.read_text(encoding='utf-8'));manifest['assets']=[x for x in manifest['assets'] if not x['id'].startswith('gta_skin_')]
    metadata={}
    labels={'email':'EMAIL','texts':'TEXT_MESSAGE','checklist':'CHECKLIST','save':'SAVE',
            'contacts':'CALL','settings':'SETTINGS_2','camera':'CAMERA','internet':'EYEFIND','trackify':'TRACKIFY'}
    for skin in ['badger','facade']:
        files=[(ROOT/f'.local/reference/cellphone_{skin}.{ext}',f'ui/cellphone_{skin}.{ext}') for ext in ['gfx','ytd']]
        files += [(p,f'icons/{skin}_{p.name}') for p in (ROOT/f'.local/reference/{skin}-textures').glob('*.png')]
        script=ROOT/f'.local/reference/{skin}-export/scripts/__Packages/com/rockstargames/gtav/cellphone/{skin}/CELLPHONE_{skin.upper()}.as'
        files.append((script,f'ui/CELLPHONE_{skin.upper()}.as'))
        home=ROOT/f'.local/reference/{skin}-export/scripts/__Packages/com/rockstargames/gtav/cellphone/apps/HOMEMENU_{skin.upper()}.as'
        files.append((home,f'ui/HOMEMENU_{skin.upper()}.as'))
        for src,dest in files:
            target=ROOT/'assets'/dest;target.parent.mkdir(exist_ok=True,parents=True)
            shutil.copyfile(src,target)
            item={'id':'gta_skin_'+skin+'_'+src.stem.lower()+'_'+src.suffix[1:],'category':dest.split('/')[0],'path':dest,
                  'sha256':hashlib.sha256(src.read_bytes()).hexdigest(),
                  'source':'Local GTA V Legacy / update scaleform_generic.rpf / RageLib and FFDec read-only extraction',
                  'source_path':f'update/update.rpf/x64/data/cdimages/scaleform_generic.rpf/cellphone_{skin}.ytd' if src.suffix=='.png' else f'cellphone_{skin}.gfx' if src.suffix=='.as' else src.name,
                  'license':'Rockstar original; no redistribution authorization recorded','status':'decoded' if src.suffix=='.png' else 'extracted'}
            manifest['assets']=[x for x in manifest['assets'] if x['id']!=item['id']];manifest['assets'].append(item)
        xml=ET.parse(ROOT/f'.local/reference/{skin}.xml').getroot()
        tags={int(t.get('shapeId') or t.get('spriteId') or -1):t for t in xml.find('tags') if t.get('type','').startswith('Define')}
        atlases={int(t.get('imageID')):t.get('exportName').lower() for t in xml.find('tags') if t.get('type')=='DefineExternalImage2'}
        menu=next(s for s in xml.findall('.//item[@type="DefineSpriteTag"]') if {'CAMERA','SAVE','TRACKIFY'}<=set(t.get('name') for t in s.find('subTags') if t.get('type')=='FrameLabelTag'))
        crops={};label=''
        for t in menu.find('subTags'):
            if t.get('type')=='FrameLabelTag':label=t.get('name')
            if t.get('type')!='PlaceObject2Tag' or t.get('depth')!='1' or label not in labels.values():continue
            child=tags[int(t.get('characterId'))]
            while child.get('type')=='DefineSpriteTag':child=tags[int(child.find('subTags/item[@type="PlaceObject2Tag"]').get('characterId'))]
            bounds=child.find('shapeBounds');matrix=child.find('.//bitmapMatrix')
            assert matrix is not None and bounds is not None
            crop=[round((float(bounds.get(axis))-float(matrix.get(trans)))/float(matrix.get(scale))) for axis,trans,scale in [('Xmin','translateX','scaleX'),('Ymin','translateY','scaleY'),('Xmax','translateX','scaleX'),('Ymax','translateY','scaleY')]]
            bitmap=child.find('.//item[@type="FILLSTYLE"]').get('bitmapId')
            crops[next(k for k,v in labels.items() if v==label)]={'bounds':crop,'atlas':f'icons/{skin}_{atlases[int(bitmap)]}.png'}
        assert len(crops)==9,crops
        metadata[skin]={'atlas':f'icons/{skin}_icons_spritesheet_{skin}.png','icon_crops':crops,
                        'source':'Native GFX frame labels, shape bounds and bitmap matrices; no guessed atlas grid'}
    (ROOT/'assets/catalog/phone-skins.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
    manifest_path.write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print('Imported original phone skins and 18 verified icon rectangles')
if __name__=='__main__':main()
