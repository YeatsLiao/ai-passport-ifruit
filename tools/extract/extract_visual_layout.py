"""Extract native GUI rectangles, state glyph crops and status vectors.

Reads cached FFDec XML/SVG exports. Original game files remain read-only;
decoded assets are catalogued in shallow icons/ui directories.
"""
from pathlib import Path
import xml.etree.ElementTree as ET
import json,re,io,hashlib,subprocess,base64
from PIL import Image
import cairosvg
ROOT=Path(__file__).resolve().parents[2]
NAMES=['ifruit','badger','facade']

def main():
    result={'canvas':[256,344],'roles':{},'glyphs':{}}
    manifest_path=ROOT/'assets/catalog/manifest.json';manifest=json.loads(manifest_path.read_text(encoding='utf-8'))
    def catalog(path,category,source,ident):
        item=dict(id='native_visual_'+ident,category=category,path=path.relative_to(ROOT/'assets').as_posix(),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),source='Local GTA V Legacy GFX / FFDec XML and vector export',source_path=source,license='Rockstar original; no redistribution authorization recorded',status='decoded')
        manifest['assets']=[x for x in manifest['assets'] if x['id']!=item['id']];manifest['assets'].append(item)
    for role in NAMES:
        root=ET.parse(ROOT/f'.local/reference/{role}.xml').getroot();tags=list(root.find('tags'))
        defs={int(t.get('spriteId') or t.get('shapeId') or t.get('characterID') or -1):t for t in tags if t.get('type') in ['DefineSpriteTag','DefineShapeTag','DefineShape2Tag','DefineShape3Tag','DefineShape4Tag','DefineEditTextTag']}
        exports={}
        for t in root.findall('.//item[@type="ExportAssetsTag"]'):exports.update(zip([x.text for x in t.find('names')],[int(x.text) for x in t.find('tags')]))
        textures={int(t.get('imageID')):t.get('exportName').lower() for t in tags if t.get('type')=='DefineExternalImage2'}
        home=defs[exports['homeMenu' if role=='facade' else 'HOMEMENU']]
        cells=[]
        for t in home.findall('subTags/item'):
            if re.fullmatch('c[123]_[123]',t.get('name','')):
                m=t.find('matrix');cells.append([float(m.get('translateX','0'))/20,float(m.get('translateY','0'))/20,float(m.get('scaleX','1')),float(m.get('scaleY','1'))])
        result['roles'][role]={'cells':cells,'cell_size':[64,64],'glyph_origin':[32,32],'source':f'cellphone_{role}.gfx#HOMEMENU'}
        # Resolve the display object active at each named icon frame, including
        # frames which reuse the preceding object's character ID.
        wanted={'UNREAD_EMAIL':'email_unread','READ_EMAIL':'email_read','UNREAD_SMS':'sms_unread','READ_SMS':'sms_read'}
        resolved={}
        for timeline in tags:
            labels=set(n.get('name') for n in timeline.findall('subTags/item[@type="FrameLabelTag"]'))
            if timeline.get('type')!='DefineSpriteTag' or len(labels)>3 or not labels.intersection(wanted):continue
            display={};label=''
            for t in timeline.find('subTags'):
                typ=t.get('type')
                if typ=='FrameLabelTag':label=t.get('name')
                elif typ in ['PlaceObject2Tag','PlaceObject3Tag'] and t.get('characterId'):display[int(t.get('depth'))]=int(t.get('characterId'))
                elif typ=='RemoveObject2Tag':display.pop(int(t.get('depth')),None)
                elif typ=='ShowFrameTag' and label in wanted:resolved[wanted[label]]=display.get(1)
        for name,sid in resolved.items():
            if sid is None:
                path=ROOT/f'assets/icons/native_{role}_{name}.png';Image.new('RGBA',(18,18)).save(path);catalog(path,'icons',f'cellphone_{role}.gfx#blank:{name}',role+'_'+name)
                result['glyphs'][role+'_'+name]={'path':path.relative_to(ROOT/'assets').as_posix(),'blank':True};continue
            shape=defs[sid]
            while shape.get('type')=='DefineSpriteTag':
                child=next(t for t in shape.findall('subTags/item') if t.get('characterId'));shape=defs[int(child.get('characterId'))]
            fill=shape.find('.//item[@type="FILLSTYLE"]');matrix=fill.find('bitmapMatrix');bounds=shape.find('shapeBounds')
            if matrix is None:
                out=ROOT/f'.local/reference/visual-shapes-{role}';gfx=ROOT/f'.local/reference/{"decoded_ifruit" if role=="ifruit" else "cellphone_"+role}.gfx'
                subprocess.run(['java','-jar',str(ROOT/'.local/tooling/ffdec/ffdec.jar'),'-selectid',str(shape.get('shapeId')),'-format','shape:svg','-export','shape',str(out),str(gfx)],check=True,stdout=subprocess.DEVNULL)
                svg=next(out.rglob(shape.get('shapeId')+'.svg'))
                im=Image.open(io.BytesIO(cairosvg.svg2png(url=str(svg),scale=4))).convert('RGBA');im.thumbnail((18,18),Image.Resampling.LANCZOS);canvas=Image.new('RGBA',(18,18));canvas.alpha_composite(im,((18-im.width)//2,(18-im.height)//2));path=ROOT/f'assets/icons/native_{role}_{name}.png';canvas.save(path);catalog(path,'icons',f'cellphone_{role}.gfx#shape:{sid}',role+'_'+name);result['glyphs'][role+'_'+name]={'path':path.relative_to(ROOT/'assets').as_posix(),'shape':sid};continue
            crop=[round((float(bounds.get(a))-float(matrix.get(b)))/float(matrix.get(c))) for a,b,c in [('Xmin','translateX','scaleX'),('Ymin','translateY','scaleY'),('Xmax','translateX','scaleX'),('Ymax','translateY','scaleY')]]
            atlas=textures[int(fill.get('bitmapId'))];prefix='' if role=='ifruit' else role+'_'
            result['glyphs'][role+'_'+name]={'atlas':f'icons/{prefix}{atlas}.png','crop':crop,'shape':shape.get('shapeId'),'output':[18,18]}
        ids={'ifruit':{'signal':207,'carrier':211,'sleep':213,'battery':202},'badger':{'signal':158,'carrier':150,'sleep':160,'battery':152,'notifications':133},'facade':{'signal':134,'carrier':138,'sleep':140,'battery':127}}[role]
        result['roles'][role]['status']={}
        for name,sid in ids.items():
            svg=ROOT/f'.local/reference/visual-svg-{role}/sprites/DefineSprite_{sid}/1.svg'
            if not svg.exists():svg=next((ROOT/f'.local/reference/visual-svg-{role}').glob(f'DefineSprite_{sid}*/1.svg'))
            raw=svg.read_bytes();s=ET.fromstring(raw)
            for node in s.iter():
                bitmap=node.get('{https://www.free-decompiler.com/flash}fill-bitmapId')
                if bitmap is None:continue
                pattern_name=re.search(r'url\(#([^)]*)\)',node.get('style',''))
                if pattern_name is None:continue
                pattern=next(n for n in s.iter() if n.get('id')==pattern_name.group(1))
                texture=ROOT/'assets/icons'/((role+'_' if role!='ifruit' else '')+textures[int(bitmap)]+'.png')
                for image in pattern.iter('{http://www.w3.org/2000/svg}image'):image.set('{http://www.w3.org/1999/xlink}href','data:image/png;base64,'+base64.b64encode(texture.read_bytes()).decode())
            raw=ET.tostring(s,encoding='utf-8')
            # The exported SVG translates its native bounding-box minimum to 0.
            matrix=s.find('{http://www.w3.org/2000/svg}g').get('transform');values=[float(x) for x in re.findall(r'[-+]?\d*\.?\d+',matrix)]
            im=Image.open(io.BytesIO(cairosvg.svg2png(bytestring=raw,scale=4))).convert('RGBA')
            w=max(1,round(float(s.get('width').removesuffix('px'))*240/256));h=max(1,round(float(s.get('height').removesuffix('px'))*320/344));im=im.resize((w,h),Image.Resampling.LANCZOS)
            path=ROOT/f'assets/icons/native_{role}_{name}.png';im.save(path)
            catalog(path,'icons',f'cellphone_{role}.gfx#sprite:{sid}/frame:1',role+'_'+name)
            source=ROOT/f'assets/ui/native_{role}_{name}.svg';source.write_bytes(raw);catalog(source,'ui',f'cellphone_{role}.gfx#sprite:{sid}/frame:1',role+'_'+name+'_svg')
            result['roles'][role]['status'][name]={'path':path.relative_to(ROOT/'assets').as_posix(),'bounds_offset':[-values[4],-values[5]],'size':[w,h],'sprite':sid}
    (ROOT/'assets/catalog/native-visuals.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    manifest_path.write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print('Extracted native home geometry, 12 state glyphs and 13 status graphics')
if __name__=='__main__':main()
