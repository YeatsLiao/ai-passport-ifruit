"""Rasterize the local game's exported $Font2; retain original source separately.

4bpp LVGL glyphs use FreeType metrics and native TTF pair adjustments.
Only the current Latin UI/content character set is included; LVGL's existing
Montserrat fonts remain the symbol fallback. No substitute is labeled original.
"""
from pathlib import Path
import json,hashlib
from PIL import Image,ImageDraw,ImageFont
from fontTools.ttLib import TTFont
ROOT=Path(__file__).resolve().parents[2]

def main():
    source=ROOT/'assets/fonts/gta_chalet_london.ttf'
    cmap=TTFont(source).getBestCmap()
    content=json.loads((ROOT/'assets/catalog/story-content.json').read_text(encoding='utf-8'))
    requested=set(range(32,127))|{ord(c) for c in json.dumps(content,ensure_ascii=False) if 127<ord(c)<0x3000}
    if requested-set(cmap):raise ValueError('Original font lacks content characters: '+str(sorted(requested-set(cmap))))
    points=sorted(requested&set(cmap))
    out=ROOT/'.local/generated';out.mkdir(parents=True,exist_ok=True)
    header=['#pragma once','#include "lvgl.h"'];code=['#include "ui_fonts.h"'];sizes=[]
    all_points=points
    for size in [14,16,18,20,23,28]:
        points=all_points if size in (16,18,20,23) else list(range(48,58))
        font=ImageFont.truetype(str(source),size)
        bitmap=bytearray();glyphs=['{0}'];n=f'game_{size}'
        for point in points:
            char=chr(point);x0,y0,x1,y1=font.getbbox(char,anchor='ls');w=x1-x0;h=y1-y0
            image=Image.new('L',(max(1,w),max(1,h)))
            ImageDraw.Draw(image).text((-x0,-y0),char,font=font,fill=255,anchor='ls')
            offset=len(bitmap);pixels=list(image.getdata()) if w and h else []
            for i in range(0,len(pixels),2):bitmap.append(((pixels[i]+8)//17<<4)|((pixels[i+1]+8)//17 if i+1<len(pixels) else 0))
            glyphs.append(f'{{.bitmap_index={offset},.adv_w={round(font.getlength(char)*16)},.box_w={w},.box_h={h},.ofs_x={x0},.ofs_y={-y1}}}')
        code.append(f'static const uint8_t {n}_bitmap[]={{'+','.join(map(str,bitmap))+'};')
        code.append(f'static const lv_font_fmt_txt_glyph_dsc_t {n}_glyphs[]={{'+','.join(glyphs)+'};')
        code.append(f'static const uint16_t {n}_unicode[]={{'+','.join(str(p-points[0]) for p in points)+'};')
        code.append(f'static const lv_font_fmt_txt_cmap_t {n}_cmap[]='+'{{'+f'.range_start={points[0]},.range_length={points[-1]-points[0]+1},.glyph_id_start=1,.unicode_list={n}_unicode,.list_length={len(points)},.type=LV_FONT_FMT_TXT_CMAP_SPARSE_TINY'+'}};')
        pairs=[]
        for i,left in enumerate(points,1):
            for j,right in enumerate(points,1):
                delta=round((font.getlength(chr(left)+chr(right))-font.getlength(chr(left))-font.getlength(chr(right)))*16)
                if delta:pairs.append((i,j,delta))
        # LVGL compares left glyph first, then right glyph (native fmt_txt comparator).
        pairs.sort(key=lambda p:(p[0],p[1]))
        kern='NULL'
        if pairs:
            code.append(f'static const uint16_t {n}_pairs[]={{'+','.join(str(v) for a,b,_ in pairs for v in [a,b])+'};')
            code.append(f'static const int8_t {n}_kern_values[]={{'+','.join(str(v) for _,_,v in pairs)+'};')
            code.append(f'static const lv_font_fmt_txt_kern_pair_t {n}_kern='+'{'+f'.glyph_ids={n}_pairs,.values={n}_kern_values,.pair_cnt={len(pairs)},.glyph_ids_size=1'+'};');kern='&'+n+'_kern'
        code.append(f'static const lv_font_fmt_txt_dsc_t {n}_dsc='+'{'+f'.glyph_bitmap={n}_bitmap,.glyph_dsc={n}_glyphs,.cmaps={n}_cmap,.kern_dsc={kern},.kern_scale=16,.cmap_num=1,.bpp=4,.kern_classes=0,.bitmap_format=0'+'};')
        ascent,descent=font.getmetrics()
        code.append(f'const lv_font_t ui_game_font_{size}='+'{'+f'.get_glyph_dsc=lv_font_get_glyph_dsc_fmt_txt,.get_glyph_bitmap=lv_font_get_bitmap_fmt_txt,.line_height={size+2},.base_line={descent},.dsc=&{n}_dsc,.fallback=&lv_font_montserrat_{14 if size<=18 else 20}'+'};')
        header.append(f'extern const lv_font_t ui_game_font_{size};');sizes.append(dict(size=size,glyphs=len(points),bitmap_bytes=len(bitmap),kern_pairs=len(pairs)))
    (out/'ui_fonts.h').write_text('\n'.join(header)+'\n',encoding='utf-8')
    (out/'ui_fonts.c').write_text('\n'.join(code)+'\n',encoding='utf-8')
    (out/'ui-fonts.json').write_text(json.dumps(dict(source='fonts/gta_chalet_london.ttf',sha256=hashlib.sha256(source.read_bytes()).hexdigest(),sizes=sizes,missing=[hex(p) for p in sorted(requested-set(cmap))]),indent=2)+'\n',encoding='utf-8')
    print('Prepared native Chalet:',sizes)
if __name__=='__main__':main()
