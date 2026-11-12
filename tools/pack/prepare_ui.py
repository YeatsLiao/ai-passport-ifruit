"""Convert selected original game PNGs to flash-resident LVGL RGB565A8 descriptors."""
from pathlib import Path
from PIL import Image,ImageChops,ImageOps,ImageDraw
import json,struct,hashlib
ROOT=Path(__file__).resolve().parents[2]; OUT=ROOT/'.local/generated';OUT.mkdir(parents=True,exist_ok=True)
images={};portrait_sources={}
settings=json.loads((ROOT/'assets/catalog/phone-settings.json').read_text(encoding='utf-8'))
backgrounds=settings['categories'][0]['options']
for i,option in enumerate(backgrounds):
 images[f'wallpaper_{i}']=Image.open(ROOT/'assets/wallpapers'/(option['texture']+'.png')).convert('RGBA').resize((240,223),Image.Resampling.LANCZOS).crop((0,0,240,216))
native=json.loads((ROOT/'assets/catalog/native-icon-crops.json').read_text(encoding='utf-8'))['roles']
for role,crops in native.items():
 for name,crop in crops.items():
  if name=='trackify':continue
  glyph=Image.open(ROOT/'assets'/crop['atlas']).convert('RGBA').crop(crop['bounds'])
  if crop.get('exclude_polygons'):
   alpha = glyph.getchannel('A')
   draw = ImageDraw.Draw(alpha)
   for polygon in crop['exclude_polygons']:
    draw.polygon([tuple(point) for point in polygon], fill=0)
   glyph.putalpha(alpha)
  if role=='ifruit' and name!='story_calls':images['icon_'+name]=glyph.resize((48,48),Image.Resampling.LANCZOS)
  else:
   side=48 if role=='ifruit' else 40
   glyph.thumbnail((side,side),Image.Resampling.LANCZOS)
   canvas=Image.new('RGBA',(side,side))
   canvas.alpha_composite(glyph,((side-glyph.width)//2,(side-glyph.height)//2))
   images[('icon_' if role=='ifruit' else role+'_icon_')+name]=canvas
images['badger_default']=Image.open(ROOT/'assets/wallpapers/phone_wallpaper_badgerdefault.png').convert('RGBA').resize((120,112),Image.Resampling.LANCZOS).crop((0,0,120,108))
images['egg_poster']=Image.open(ROOT/'assets/icons/gta6_easteregg_user.png').convert('RGBA').resize((160,205),Image.Resampling.LANCZOS)
images['facade_glass']=Image.open(ROOT/'assets/icons/facade_facade_glass.png').convert('RGBA').resize((120,60),Image.Resampling.LANCZOS)
notifications=json.loads((ROOT/'assets/catalog/ui-notifications.json').read_text(encoding='utf-8'))
for role in ('ifruit','badger'):
 info=notifications[role];images['notice_'+role]=Image.open(ROOT/'assets'/info['atlas']).convert('RGBA').crop(info['crop']).resize(info['output'],Image.Resampling.LANCZOS)
visuals=json.loads((ROOT/'assets/catalog/native-visuals.json').read_text(encoding='utf-8'));masks=set()
for name,info in visuals['glyphs'].items():
 im=Image.open(ROOT/'assets'/info.get('path',info.get('atlas'))).convert('RGBA')
 if 'crop' in info:im=im.crop(info['crop']);im.thumbnail((18,18),Image.Resampling.LANCZOS)
 canvas=Image.new('RGBA',(18,18));canvas.alpha_composite(im,((18-im.width)//2,(18-im.height)//2));images['state_'+name]=canvas;masks.add('state_'+name)
for role,info in visuals['roles'].items():
 for name,item in info['status'].items():
  im=Image.open(ROOT/'assets'/item['path']).convert('RGBA');alpha=ImageChops.multiply(im.convert('L'),im.getchannel('A'));im.putalpha(alpha)
  images['status_'+role+'_'+name]=im;masks.add('status_'+role+'_'+name)
for role in ('ifruit','badger','facade'):
 source=ROOT/'assets/icons'/('soft_key_bg_gradient_ifruit.png' if role!='badger' else 'badger_soft_key_bg_gradient_badger.png')
 images['footer_'+role]=Image.new('RGBA',(4,37),(25,29,30,255))
# Native protagonist loading art, paired with its original background. Device framing only.
for n,who in enumerate(['michael','franklin','trevor']):
 bg=ImageOps.fit(Image.open(ROOT/f'assets/icons/snapmatic_{who}_background.png').convert('RGBA'),(240,216),method=Image.Resampling.LANCZOS)
 fg=Image.open(ROOT/f'assets/icons/snapmatic_{who}_foreground.png').convert('RGBA');fg=fg.crop(fg.getbbox());fg.thumbnail((240,216),Image.Resampling.LANCZOS)
 bg.alpha_composite(fg,((240-fg.width)//2,216-fg.height));images['snap_'+str(n)]=bg
images['snap_3']=ImageOps.fit(Image.open(ROOT/'assets/icons/snapmatic_michael_background.png').convert('RGBA'),(240,216),method=Image.Resampling.LANCZOS)
book=json.loads((ROOT/'assets/catalog/phonebook.json').read_text(encoding='utf-8'))
portraits={contact['portrait_key']:ROOT/contact['portrait_path'] for contact in book['contacts']}
extra_calls=json.loads((ROOT/'assets/catalog/call-scenes-extra.json').read_text(encoding='utf-8'))['scenes']
for scene in extra_calls:
 name=scene['portrait_name']
 portraits.setdefault(name,ROOT/'assets/Chars'/(name+'.png'))
for name,p in portraits.items():
 if not p.exists():continue
 images[name]=Image.open(p).convert('RGBA')
 portrait_sources[name]={'source_path':p.relative_to(ROOT).as_posix(),'source_sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
fallback=ROOT/'assets/Chars/char_default.png'
if 'char_default' not in images:
 images['char_default']=Image.open(fallback).convert('RGBA').resize((64,64),Image.Resampling.LANCZOS)
 portrait_sources['char_default']={'source_path':fallback.relative_to(ROOT).as_posix(),'source_sha256':hashlib.sha256(fallback.read_bytes()).hexdigest()}
buttons=Image.open(ROOT/'assets/icons/buttons_icons_spritesheet_256.png').convert('RGBA')
for name,x,y in [('select',0,0),('back',2,3),('hangup',2,2),('keypad',1,2),('add',3,0)]:
 glyph=buttons.crop((x*64,y*64,x*64+64,y*64+64));glyph=glyph.crop(glyph.getbbox())
 glyph.thumbnail((23,23),Image.Resampling.LANCZOS)
 canvas=Image.new('RGBA',(23,23));canvas.alpha_composite(glyph,((23-glyph.width)//2,(23-glyph.height)//2));images['soft_'+name]=canvas
header=['#pragma once','#include "lvgl.h"'];body=['#include "ui_assets.h"'];index=[]
for name,image in images.items():
 # Original CONTENT offset; preserve alpha and leave external glass/status untouched.
 if name.startswith(('badger_icon_','facade_icon_','footer_badger','footer_facade')) or name=='badger_default':
  image=image.point([max(0,v-25) for v in range(256)]*3+list(range(256)))
 w,h=image.size;pixels=list(image.getdata());raw=bytearray()
 for r,g,b,a in (pixels if name not in masks else []):raw.extend(struct.pack('<H',((r>>3)<<11)|((g>>2)<<5)|(b>>3)))
 opaque=name.startswith(('wallpaper_','char_')) or name.startswith('snap_') or name in ('badger_default','egg_poster')
 if not opaque:raw.extend(p[3] for p in pixels)
 body.append('static const uint8_t data_'+name+'[] = {'+','.join(map(str,raw))+'};')
 fmt='LV_COLOR_FORMAT_A8' if name in masks else 'LV_COLOR_FORMAT_RGB565' if opaque else 'LV_COLOR_FORMAT_RGB565A8'
 stride=w if name in masks else w*2
 body.append(f'const lv_image_dsc_t ui_{name} = {{.header={{.magic=LV_IMAGE_HEADER_MAGIC,.cf={fmt},.w={w},.h={h},.stride={stride}}},.data_size={len(raw)},.data=data_{name}}};')
 header.append('extern const lv_image_dsc_t ui_'+name+';');index.append(dict(name=name,width=w,height=h,bytes=len(raw),**portrait_sources.get(name,{})))
header.append(f'extern const lv_image_dsc_t *const ui_wallpapers[{len(backgrounds)}];')
body.append(f'const lv_image_dsc_t *const ui_wallpapers[{len(backgrounds)}] = {{'+','.join(f'&ui_wallpaper_{i}' for i in range(len(backgrounds)))+'};')
header.append('const lv_image_dsc_t *ui_portrait_lookup(const char *key);')
body.append('#include <string.h>')
body.append('const lv_image_dsc_t *ui_portrait_lookup(const char *key){')
for name in portrait_sources:body.append('if(key && !strcmp(key,"'+name+'"))return &ui_'+name+';')
body.append('return NULL;}')
(OUT/'ui_assets.h').write_text('\n'.join(header)+'\n',encoding='utf-8');(OUT/'ui_assets.c').write_text('\n'.join(body)+'\n',encoding='utf-8');(OUT/'ui-assets.json').write_text(json.dumps(index,indent=2)+'\n')
print('Prepared',len(images),'original-image descriptors')

from tools.pack.prepare_fonts import main as prepare_fonts
prepare_fonts()
