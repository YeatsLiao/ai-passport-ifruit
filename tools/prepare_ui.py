"""Convert selected original game PNGs to flash-resident LVGL RGB565A8 descriptors."""
from pathlib import Path
from PIL import Image
import json,struct
ROOT=Path(__file__).resolve().parents[1]; OUT=ROOT/'build/generated';OUT.mkdir(parents=True,exist_ok=True)
images={}
settings=json.loads((ROOT/'assets/phone-settings.json').read_text(encoding='utf-8'))
backgrounds=settings['categories'][0]['options']
for i,option in enumerate(backgrounds):
 images[f'wallpaper_{i}']=Image.open(ROOT/'assets/wallpapers'/(option['texture']+'.png')).convert('RGBA').resize((240,223),Image.Resampling.LANCZOS)
atlas=Image.open(ROOT/'assets/icons/icons_spritesheet_ifruit.png').convert('RGBA')
# Atlas has 5 columns/rows; 50px artwork cells separated by 1px padding.
for name,x,y in [('texts',1,0),('contacts',4,0),('email',3,0),('internet',0,1),('camera',0,0),('save',0,3),('settings',3,1),('checklist',4,1),('calendar',2,0),('trackify',2,2)]:
 images['icon_'+name]=atlas.crop((x*51,y*51,x*51+50,y*51+50)).resize((48,48),Image.Resampling.LANCZOS)
for person in ['franklin','trevor','lester','amanda','jimmy','dave']:
 name='char_'+person;p=ROOT/'assets/portraits'/(name+'.png')
 if not p.exists() and person=='dave':p=ROOT/'assets/portraits/char_dave_norton.png'
 if not p.exists() and person=='franklin':p=ROOT/'assets/portraits/char_franklin.png'
 images['portrait_'+person]=Image.open(p).convert('RGBA')
buttons=Image.open(ROOT/'assets/icons/buttons_icons_spritesheet_256.png').convert('RGBA')
for name,x,y in [('select',0,0),('back',2,3),('hangup',2,2),('keypad',1,2)]:
 images['soft_'+name]=buttons.crop((x*64,y*64,x*64+64,y*64+64)).resize((23,23),Image.Resampling.LANCZOS)
header=['#pragma once','#include "lvgl.h"'];body=['#include "ui_assets.h"'];index=[]
for name,image in images.items():
 w,h=image.size;pixels=list(image.getdata());raw=bytearray()
 for r,g,b,a in pixels:raw.extend(struct.pack('<H',((r>>3)<<11)|((g>>2)<<5)|(b>>3)))
 opaque=name.startswith('wallpaper_')
 if not opaque:raw.extend(p[3] for p in pixels)
 body.append('static const uint8_t data_'+name+'[] = {'+','.join(map(str,raw))+'};')
 fmt='LV_COLOR_FORMAT_RGB565' if opaque else 'LV_COLOR_FORMAT_RGB565A8'
 body.append(f'const lv_image_dsc_t ui_{name} = {{.header={{.magic=LV_IMAGE_HEADER_MAGIC,.cf={fmt},.w={w},.h={h},.stride={w*2}}},.data_size={len(raw)},.data=data_{name}}};')
 header.append('extern const lv_image_dsc_t ui_'+name+';');index.append(dict(name=name,width=w,height=h,bytes=len(raw)))
header.append(f'extern const lv_image_dsc_t *const ui_wallpapers[{len(backgrounds)}];')
body.append(f'const lv_image_dsc_t *const ui_wallpapers[{len(backgrounds)}] = {{'+','.join(f'&ui_wallpaper_{i}' for i in range(len(backgrounds)))+'};')
(OUT/'ui_assets.h').write_text('\n'.join(header)+'\n',encoding='utf-8');(OUT/'ui_assets.c').write_text('\n'.join(body)+'\n',encoding='utf-8');(OUT/'ui-assets.json').write_text(json.dumps(index,indent=2)+'\n')
print('Prepared',len(images),'original-image descriptors')
