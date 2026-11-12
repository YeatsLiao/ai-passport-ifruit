"""Resolve all phone home glyphs from native GFX bitmap matrices, not a guessed grid."""
from pathlib import Path
import json,xml.etree.ElementTree as E
ROOT=Path(__file__).resolve().parents[2]
labels={'email':'EMAIL','texts':'TEXT_MESSAGE','checklist':'SIDE_TASKS','save':'SAVE','contacts':'CALL','settings':'SETTINGS_2','camera':'CAMERA','internet':'EYEFIND','trackify':'TRACKIFY','story_calls':'MISSED_CALL'}
result={}
for role in ['ifruit','badger','facade']:
 r=E.parse(ROOT/f'.local/reference/{role}.xml').getroot()
 tags={int(t.get('shapeId') or t.get('spriteId') or -1):t for t in r.find('tags') if t.get('type','').startswith('Define')}
 atlases={int(t.get('imageID')):t.get('exportName').lower() for t in r.find('tags') if t.get('type')=='DefineExternalImage2'}
 menu=next(s for s in r.findall('.//item[@type="DefineSpriteTag"]') if {'CAMERA','SAVE','TRACKIFY'}<=set(t.get('name') for t in s.find('subTags') if t.get('type')=='FrameLabelTag'))
 crops={};label=''
 for t in menu.find('subTags'):
  if t.get('type')=='FrameLabelTag':label=t.get('name')
  if t.get('type')!='PlaceObject2Tag' or t.get('depth')!='1' or label not in labels.values():continue
  child=tags[int(t.get('characterId'))]
  while child.get('type')=='DefineSpriteTag':child=tags[int(child.find('subTags/item[@type="PlaceObject2Tag"]').get('characterId'))]
  bounds=child.find('shapeBounds');m=child.find('.//bitmapMatrix')
  crop=[round((float(bounds.get(axis))-float(m.get(trans)))/float(m.get(scale))) for axis,trans,scale in [('Xmin','translateX','scaleX'),('Ymin','translateY','scaleY'),('Xmax','translateX','scaleX'),('Ymax','translateY','scaleY')]]
  bitmap=child.find('.//item[@type="FILLSTYLE"]').get('bitmapId');prefix='' if role=='ifruit' else role+'_'
  crops[next(k for k,v in labels.items() if v==label)]={'bounds':crop,'atlas':'icons/'+prefix+atlases[int(bitmap)]+'.png','shape':child.get('shapeId'),'frame':label}
 if role == 'badger':
  # The native rectangular bounds include pixels from adjacent atlas entries.
  crops['contacts']['exclude_polygons'] = [[[0, 0], [2, 0], [2, 42], [0, 42]]]
  crops['settings']['exclude_polygons'] = [[[46, 41], [46, 46], [39, 46], [40, 45], [42, 44], [44, 42]]]
 assert len(crops)==len(labels)
 result[role]=crops
(ROOT/'assets/catalog/native-icon-crops.json').write_text(json.dumps(dict(source='Native GFX shapes and bitmap matrices; Check List uses SIDE_TASKS home glyph, not the CHECKLIST delete-state glyph',roles=result),indent=2)+'\n',encoding='utf-8')
print(f'Resolved {len(result)*len(labels)} native glyph rectangles')
