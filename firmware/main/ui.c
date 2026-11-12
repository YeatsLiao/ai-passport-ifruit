#include "ui.h"
#include "lvgl.h"
#include "ui_assets.h"
#include "ui_fonts.h"
#include <stdio.h>
#include <string.h>
static lv_obj_t *header,*body,*wallpaper,*home,*selection,*contact_list,*rows[5],*names[5],*faces[5];
static lv_obj_t *call_face,*call_name,*call_state,*soft[3];
static lv_obj_t *header_bar,*footer_bar,*reader_panel;
static lv_obj_t *reader_sender,*reader_subject,*glass,*clock_text,*signal_bars,*scroll_thumb;
static unsigned unread_count(const phone_t *p,story_kind_t kind);
static bool reader_active;static unsigned reader_previous_scroll;
static bool rendered;static phone_page_t previous_page;static unsigned previous_role;
static unsigned list_first[PHONE_STORY_CALLS+1][3],last_selection,last_count;
enum { HOME_TOP=60, HOME_WIDTH=240, HOME_HEIGHT=216, HOME_ROW_STEP=67 };
static int home_icon_x(unsigned role,unsigned column){return HOME_WIDTH/2+((int)column-1)*(role==2?73:69);}
static int home_icon_y(unsigned row){return HOME_HEIGHT/2+((int)row-1)*HOME_ROW_STEP;}
static unsigned view_rows=5;static int view_top=60,view_height=223;
static struct {unsigned role,day,profile;bool home,landscape;unsigned email,texts;} chrome;
static int row_subx[5];static char row_subtitle[5][180];static const lv_font_t *row_subfont[5];static lv_color_t row_subcolor[5];
static unsigned home_badge_count[3],home_role;
static char badge_text[3][4];static lv_color_t badge_color[3];

static void scrollbar_alpha(void *object,int32_t value) {lv_obj_set_style_bg_opa(object,(lv_opa_t)value,0);}
static void scrollbar(unsigned selected,unsigned count,bool show,bool white) {
    if(count<=view_rows){lv_anim_delete(scroll_thumb,scrollbar_alpha);lv_obj_set_style_bg_opa(scroll_thumb,0,0);return;}
    int height=view_height*(int)view_rows/(int)count;if(height<8)height=8;
    lv_obj_set_pos(scroll_thumb,lv_display_get_horizontal_resolution(NULL)-5,view_top+(view_height-height)*(int)selected/(int)(count-1));lv_obj_set_size(scroll_thumb,3,height);
    lv_obj_set_style_bg_color(scroll_thumb,white?lv_color_white():lv_color_black(),0);
    if(show){
        lv_anim_delete(scroll_thumb,scrollbar_alpha);
        lv_anim_t a;lv_anim_init(&a);lv_anim_set_var(&a,scroll_thumb);lv_anim_set_exec_cb(&a,scrollbar_alpha);
        lv_anim_set_values(&a,white?191:128,0);lv_anim_set_delay(&a,300);lv_anim_set_duration(&a,2500);lv_anim_start(&a);
        scrollbar_alpha(scroll_thumb,white?191:128);
    }
}
static unsigned window_start(const phone_t *p,unsigned selected,unsigned count) {
    unsigned *first=&list_first[p->page][p->world.role];
    if(count<=view_rows)*first=0;
    else {if(*first>count-view_rows)*first=count-view_rows;if(selected<*first)*first=selected;else if(selected>=*first+view_rows)*first=selected-view_rows+1;}
    bool moved=rendered && previous_page==p->page && previous_role==p->world.role && (selected!=last_selection || count!=last_count);
    scrollbar(selected,count,moved,p->world.role==2);
    last_selection=selected;last_count=count;
    return *first;
}
static void badge_draw(lv_event_t *event) {
 lv_area_t origin;lv_obj_get_coords(lv_event_get_target_obj(event),&origin);lv_layer_t *layer=lv_event_get_layer(event);
 for(unsigned i=0;i<3;i++)if(home_badge_count[i]){
  int cx=home_icon_x(home_role,i),cy=home_icon_y(0);
  if(home_role==2){
   /* Facade uses a large right-aligned number, never a badge circle. */
   lv_area_t at={origin.x1+cx-4,origin.y1+cy-29,origin.x1+cx+29,origin.y1+cy-2};
   lv_draw_label_dsc_t label;lv_draw_label_dsc_init(&label);label.text=badge_text[i];label.font=&ui_game_font_20;label.color=badge_color[i];label.align=LV_TEXT_ALIGN_RIGHT;lv_draw_label(layer,&label,&at);continue;
  }
  int x=cx+10,y=cy-21;const lv_image_dsc_t *art=&ui_notice_badger;
  if(home_role==0){
   x=cx-35;y=cy-33;art=&ui_notice_ifruit;
  }
  lv_area_t at={origin.x1+x,origin.y1+y,origin.x1+x+(int)art->header.w-1,origin.y1+y+(int)art->header.h-1};
  lv_draw_image_dsc_t image;lv_draw_image_dsc_init(&image);image.src=art;lv_draw_image(layer,&image,&at);
  /* Badger's per-app indicator is only the original asterisk. */
  if(home_role==0){
   at.x2=at.x1+25;at.y1+=3;at.y2=at.y1+25;
   lv_draw_label_dsc_t label;lv_draw_label_dsc_init(&label);label.text=badge_text[i];label.font=&ui_game_font_16;label.color=lv_color_white();label.align=LV_TEXT_ALIGN_CENTER;lv_draw_label(layer,&label,&at);
  }
 }
}
static const lv_image_dsc_t *state_glyph(unsigned role,bool email,bool unread) {
 static const lv_image_dsc_t *const table[3][4]={
  {&ui_state_ifruit_sms_unread,&ui_state_ifruit_sms_read,&ui_state_ifruit_email_unread,&ui_state_ifruit_email_read},
  {&ui_state_badger_sms_unread,&ui_state_badger_sms_read,&ui_state_badger_email_unread,&ui_state_badger_email_read},
  {&ui_state_facade_sms_unread,&ui_state_facade_sms_read,&ui_state_facade_email_unread,&ui_state_facade_email_read}};
 return table[role][(email?2:0)+(unread?0:1)];
}
static void fit_line(char *out,unsigned capacity,const char *text,int width,const lv_font_t *font) {
 snprintf(out,capacity,"%s",text);unsigned n=(unsigned)strlen(out);bool shortened=false;
 while(n){lv_point_t size;lv_text_get_size(&size,out,font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_EXPAND);if(size.x<=width-12)break;n--;while(n && ((unsigned char)out[n]&0xc0)==0x80)n--;out[n]=0;shortened=true;}
 if(shortened && n+3<capacity)strcat(out,"...");
}
static void subtitle_draw(lv_event_t *event) {
 unsigned i=(unsigned)(uintptr_t)lv_event_get_user_data(event);if(!row_subtitle[i][0])return;
 lv_area_t at;lv_obj_get_coords(lv_event_get_target_obj(event),&at);at.x1+=row_subx[i];at.x2-=7;at.y1+=row_subfont[i]==&ui_game_font_18?25:24;
 lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);d.text=row_subtitle[i];d.font=row_subfont[i];d.color=row_subcolor[i];lv_draw_label(lv_event_get_layer(event),&d,&at);
}
static void header_draw(lv_event_t *event) {
 lv_obj_t *o=lv_event_get_target_obj(event);lv_area_t at;lv_obj_get_coords(o,&at);
 lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=lv_obj_get_style_bg_color(o,0);d.bg_opa=255;
 d.bg_color=lv_color_mix(chrome.role==0?lv_color_hex(0x79b7ff):lv_color_white(),d.bg_color,chrome.role==0?85:35);at.y2=at.y1+11;lv_draw_rect(lv_event_get_layer(event),&d,&at);
}
static bool dial_controls, story_call_controls, numberpad_active;
static unsigned keypad_selected;
static char keypad_number[PHONE_NUMBER_CAPACITY+1];
static lv_color_t keypad_focus;
static const lv_image_dsc_t *const snap_images[SNAP_SCENES]={&ui_snap_0,&ui_snap_1,&ui_snap_2,&ui_snap_3};
static const char *const snap_names[SNAP_SCENES]={"Michael","Franklin","Trevor","Los Santos"};
static bool viewfinder;
static void centered_text(lv_layer_t *layer,lv_area_t area,const char *text,const lv_font_t *font,lv_color_t color) {
 lv_draw_label_dsc_t label;
 lv_draw_label_dsc_init(&label);
 label.font=font;
 label.color=color;
 label.text=text;
 label.text_local=1;
 label.align=LV_TEXT_ALIGN_CENTER;
 lv_draw_label(layer,&label,&area);
}
static void keypad_draw(lv_event_t *event) {
 if(!numberpad_active)return;
 lv_area_t base;
 lv_obj_get_coords(lv_event_get_target_obj(event),&base);
 lv_layer_t *layer=lv_event_get_layer(event);
 bool dark=chrome.role==2;
 lv_color_t ink=dark?lv_color_white():lv_color_black();
 centered_text(layer,(lv_area_t){base.x1+6,base.y1+5,base.x2-6,base.y1+33},
    *keypad_number?keypad_number:"Enter a number",&ui_game_font_20,ink);
 const char *const keys[]={"1","2","3","4","5","6","7","8","9","Erase","0","Call"};
 lv_draw_rect_dsc_t tile;
 lv_draw_rect_dsc_init(&tile);
 tile.bg_opa=255;
 for(unsigned i=0;i<sizeof(keys)/sizeof(keys[0]);i++){
  int x=base.x1+5+77*(int)(i%3),y=base.y1+37+43*(int)(i/3);
  lv_area_t area={x,y,x+74,y+40};
  bool selected=i==keypad_selected;
  tile.bg_color=selected?keypad_focus:lv_color_hex(dark?0x292929:0xe6e6e6);
  lv_draw_rect(layer,&tile,&area);
  area.y1+=8;
  centered_text(layer,area,keys[i],&ui_game_font_20,selected?lv_color_white():ink);
 }
}
static void camera_draw(lv_event_t *event){
 if(!viewfinder)return;
 lv_area_t at;lv_obj_get_coords(lv_event_get_target_obj(event),&at);lv_layer_t *layer=lv_event_get_layer(event);
 lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_opa=0;d.border_width=1;d.border_color=lv_color_white();d.border_opa=180;lv_area_t frame={at.x1+12,at.y1+12,at.x2-12,at.y2-12};lv_draw_rect(layer,&d,&frame);
 d.bg_opa=255;d.bg_color=lv_color_white();d.border_width=0;lv_area_t h={at.x1+111,at.y1+107,at.x1+129,at.y1+107},v={at.x1+120,at.y1+98,at.x1+120,at.y1+116};lv_draw_rect(layer,&d,&h);lv_draw_rect(layer,&d,&v);
}
static void footer_draw(lv_event_t *event) {
 lv_area_t at;lv_obj_get_coords(lv_event_get_target_obj(event),&at);int w=lv_area_get_width(&at);
 lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=lv_color_hex(0x737373);d.bg_opa=130;
 if(dial_controls || story_call_controls){
  lv_layer_t *layer=lv_event_get_layer(event);
  centered_text(layer,(lv_area_t){at.x1,at.y1+3,at.x2,at.y1+22},story_call_controls?"UP/DOWN: Choose   OK: Ring":"UP/DOWN: Move   OK: Select",&ui_game_font_16,lv_color_white());
  centered_text(layer,(lv_area_t){at.x1,at.y1+24,at.x2,at.y2},"Hold OK: Back",&ui_game_font_16,lv_color_hex(0xbcbcbc));
  return;
 }
 for(int i=1;i<3;i++){lv_area_t line={at.x1+w*i/3,at.y1+5,at.x1+w*i/3,at.y2-5};lv_draw_rect(lv_event_get_layer(event),&d,&line);}
}
static void chrome_image(lv_layer_t *layer,const lv_image_dsc_t *art,int x,int y,lv_color_t color) {
 lv_draw_image_dsc_t d;lv_draw_image_dsc_init(&d);d.src=art;d.recolor=color;d.recolor_opa=255;
 lv_area_t at={x,y,x+(int)art->header.w-1,y+(int)art->header.h-1};lv_draw_image(layer,&d,&at);
}
static void signal_draw(lv_event_t *event) {
 static const lv_image_dsc_t *const art[3][4]={
  {&ui_status_ifruit_signal,&ui_status_ifruit_carrier,&ui_status_ifruit_sleep,&ui_status_ifruit_battery},
  {&ui_status_badger_signal,&ui_status_badger_carrier,&ui_status_badger_sleep,&ui_status_badger_battery},
  {&ui_status_facade_signal,&ui_status_facade_carrier,&ui_status_facade_sleep,&ui_status_facade_battery}};
 static const char *days[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
 unsigned r=chrome.role;int w=lv_display_get_horizontal_resolution(NULL);lv_layer_t *layer=lv_event_get_layer(event);lv_color_t white=lv_color_hex(r?0xe6e6e6:0xffffff);
 int sx=r==1?w-29:4,cx=r==1?w-48:r==2?36:27;
 chrome_image(layer,art[r][0],sx,5,white);chrome_image(layer,art[r][1],cx,5,white);
 if(chrome.profile==1)chrome_image(layer,art[r][2],r==1?w-68:r==2?143:w-49,6,white);
 if(r!=1 || chrome.home)chrome_image(layer,art[r][3],r==1?4:r==2?w-77:w-29,r==1?4:6,white);
 if(r!=1 || chrome.home){lv_area_t day={r==1?3:r==2?55:49,r==1?17:1,r==1?40:r==2?88:81,r==1?38:23};lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);d.font=&ui_game_font_16;d.text=days[chrome.day%7];d.color=white;lv_draw_label(layer,&d,&day);}
 if(r==1 && chrome.home){
  chrome_image(layer,&ui_status_badger_notifications,66,33,white);
  char count[4];lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);d.font=&ui_game_font_18;d.align=LV_TEXT_ALIGN_RIGHT;d.color=white;
  snprintf(count,sizeof(count),"%u",chrome.email>99?99:chrome.email);d.text=count;d.text_local=1;lv_area_t at={66,35,96,58};lv_draw_label(layer,&d,&at);
  snprintf(count,sizeof(count),"%u",chrome.texts>99?99:chrome.texts);at.x1=114;at.x2=144;lv_draw_label(layer,&d,&at);
 }
}
static const lv_image_dsc_t *icons[9]={&ui_icon_email,&ui_icon_texts,&ui_icon_checklist,&ui_icon_save,&ui_icon_contacts,&ui_icon_settings,&ui_icon_camera,&ui_icon_internet,&ui_icon_story_calls};

static const lv_image_dsc_t *skin_icons[2][9]={
 {&ui_badger_icon_email,&ui_badger_icon_texts,&ui_badger_icon_checklist,&ui_badger_icon_save,&ui_badger_icon_contacts,&ui_badger_icon_settings,&ui_badger_icon_camera,&ui_badger_icon_internet,&ui_badger_icon_story_calls},
 {&ui_facade_icon_email,&ui_facade_icon_texts,&ui_facade_icon_checklist,&ui_facade_icon_save,&ui_facade_icon_contacts,&ui_facade_icon_settings,&ui_facade_icon_camera,&ui_facade_icon_internet,&ui_facade_icon_story_calls}};
static lv_obj_t *box(lv_obj_t *parent,int x,int y,int w,int h,uint32_t colour) {
    lv_obj_t *o=lv_obj_create(parent);lv_obj_remove_style_all(o);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
    lv_obj_set_style_bg_color(o,lv_color_hex(colour),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;
}
static lv_obj_t *text(lv_obj_t *parent,int x,int y,int w,const char *value) {
    lv_obj_t *o=lv_label_create(parent);lv_obj_set_pos(o,x,y);lv_obj_set_width(o,w);lv_label_set_text(o,value);return o;
}
static lv_obj_t *image(lv_obj_t *parent,int x,int y,const lv_image_dsc_t *source) {
    lv_obj_t *o=lv_image_create(parent);lv_image_set_src(o,source);lv_obj_set_pos(o,x,y);return o;
}
static void visible(lv_obj_t *o,bool show) { if(show)lv_obj_remove_flag(o,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN); }
void phone_ui_init(void) {
    lv_obj_t *s=lv_screen_active();lv_obj_set_style_bg_color(s,lv_color_black(),0);lv_obj_set_style_text_color(s,lv_color_white(),0);
    lv_obj_set_style_text_font(s,&ui_game_font_18,0);lv_obj_remove_flag(s,LV_OBJ_FLAG_SCROLLABLE);
    signal_bars=box(s,0,0,240,80,0);lv_obj_set_style_bg_opa(signal_bars,0,0);lv_obj_add_event_cb(signal_bars,signal_draw,LV_EVENT_DRAW_MAIN,NULL);
    clock_text=text(s,78,6,85,"09:41");lv_obj_set_style_text_align(clock_text,LV_TEXT_ALIGN_CENTER,0);

    header_bar=box(s,0,30,240,30,0x5a97e8);header=text(header_bar,3,7,234,"");lv_obj_add_event_cb(header_bar,header_draw,LV_EVENT_DRAW_MAIN_END,NULL);lv_obj_set_style_text_align(header,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_font(header,&ui_game_font_23,0);lv_obj_set_height(header,27);lv_label_set_long_mode(header,LV_LABEL_LONG_DOT);
    wallpaper=image(s,0,60,ui_wallpapers[0]);lv_obj_add_event_cb(wallpaper,camera_draw,LV_EVENT_DRAW_POST,NULL);
    home=box(s,0,HOME_TOP,HOME_WIDTH,HOME_HEIGHT,0);lv_obj_set_style_bg_opa(home,LV_OPA_TRANSP,0);
    selection=box(home,0,0,68,65,0x98b6e6);lv_obj_set_style_bg_opa(selection,100,0);
    for(int i=0;i<9;i++) image(home,16+(i%3)*80,10+(i/3)*70,icons[i]);
    lv_obj_add_event_cb(home,badge_draw,LV_EVENT_DRAW_POST,NULL);

    contact_list=box(s,0,60,240,223,0xd2d2d2);
    for(int i=0;i<5;i++) {
        rows[i]=box(contact_list,0,i*44,240,44,0xebebeb);lv_obj_add_event_cb(rows[i],subtitle_draw,LV_EVENT_DRAW_POST,(void *)(uintptr_t)i);
        faces[i]=image(rows[i],3,2,ui_portrait_lookup("char_default"));lv_image_set_pivot(faces[i],0,0);lv_image_set_scale(faces[i],160);
        names[i]=text(rows[i],46,13,190,"");lv_obj_set_height(names[i],20);lv_label_set_long_mode(names[i],LV_LABEL_LONG_DOT);lv_obj_set_style_text_color(names[i],lv_color_black(),0);
    }
    reader_panel=box(s,0,60,240,223,0xffffff);lv_obj_add_event_cb(reader_panel,keypad_draw,LV_EVENT_DRAW_MAIN_END,NULL);
    reader_subject=text(reader_panel,7,24,224,"");lv_obj_set_style_text_color(reader_subject,lv_color_black(),0);
    reader_sender=text(reader_panel,8,8,224,"");lv_obj_set_style_text_font(reader_sender,&ui_game_font_20,0);lv_obj_set_style_text_color(reader_sender,lv_color_black(),0);
    body=text(reader_panel,8,8,224,"");lv_obj_set_style_text_color(body,lv_color_black(),0);
    call_face=image(s,9,72,ui_portrait_lookup("char_default"));lv_image_set_pivot(call_face,0,0);call_name=text(s,82,73,152,"");call_state=text(s,82,104,152,"");

    footer_bar=box(s,0,283,240,37,0);
    lv_obj_add_event_cb(footer_bar,footer_draw,LV_EVENT_DRAW_MAIN_END,NULL);
    soft[0]=image(footer_bar,27,7,&ui_soft_select);soft[1]=image(footer_bar,108,7,&ui_soft_keypad);soft[2]=image(footer_bar,188,7,&ui_soft_back);
    scroll_thumb=box(s,235,60,3,20,0);lv_obj_set_style_bg_opa(scroll_thumb,0,0);
    glass=image(s,0,0,&ui_facade_glass);lv_image_set_pivot(glass,0,0);lv_image_set_scale(glass,512);lv_obj_set_style_opa(glass,102,0);visible(glass,false);
}
/* Native Badger/Facade CONTENT has a -25 RGB offset, clamped per channel. */
static lv_color_t content_color(unsigned role,uint32_t rgb) {
 if(role){unsigned r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255;rgb=((r>25?r-25:0)<<16)|((g>25?g-25:0)<<8)|(b>25?b-25:0);}
 return lv_color_hex(rgb);
}
static void render_page(phone_t *p) {
    viewfinder=p->page==PHONE_VIEWFINDER;
    numberpad_active=p->page==PHONE_NUMBERPAD;
    dial_controls=numberpad_active || p->page==PHONE_NUMBER_LIST;
    story_call_controls=p->page==PHONE_STORY_CALLS;
    bool black_phone=(p->black_phone_roles&(1u<<p->world.role))!=0;
    for(unsigned i=0;i<5;i++){row_subtitle[i][0]=0;row_subx[i]=27;}
    visible(reader_sender,p->page==PHONE_READER && p->reader_kind!=STORY_WEB);
    visible(reader_subject,p->page==PHONE_READER && p->reader_kind==STORY_EMAIL);
    lv_obj_set_y(body,p->page==PHONE_READER && p->reader_kind!=STORY_WEB?42:8);
    bool ishome=p->page==PHONE_HOME,iscontacts=p->page==PHONE_CONTACTS;
    if(p->page!=PHONE_READER)reader_active=false;
    bool issettings=p->page==PHONE_SETTINGS || p->page==PHONE_SETTING_OPTIONS;
    bool isstory=p->page==PHONE_EMAIL || p->page==PHONE_MESSAGES || p->page==PHONE_INTERNET;
    bool background_preview=p->page==PHONE_SETTING_OPTIONS && p->selected_setting==SET_BACKGROUND;
    bool isphoto=p->page==PHONE_PHOTO || p->page==PHONE_VIEWFINDER || background_preview,isegg=p->page==PHONE_EASTER_EGG;
    bool islist=story_call_controls || p->page==PHONE_NUMBER_LIST || p->page==PHONE_CALL_HISTORY || p->page==PHONE_CONTACT_ACTIONS || isstory || p->page==PHONE_CAMERA || p->page==PHONE_GALLERY || p->page==PHONE_ROLE || p->page==PHONE_CHECKLIST || p->page==PHONE_OPEN_MISSIONS || p->page==PHONE_PROGRESS;
    bool iscall=p->page==PHONE_CALL || p->page==PHONE_INCOMING || p->page==PHONE_DIALING || p->page==PHONE_BUSY;
    unsigned role=p->world.role;
    bool facade=role==2;
    uint32_t native_palette[3];
    unsigned theme=p->settings[SET_THEME];
    if(role==1 && theme==0)theme=1;
    if(role==2 && theme==0)theme=2;
    for(unsigned i=0;i<3;i++)native_palette[i]=theme_colors[theme][i];
    if(role==1){static const uint32_t badger[7][2]={{0x058df3,0x4c8fc0},{0x208427,0x39a033},{0xf30505,0xdf3232},{0xe6712b,0xd5792f},{0x7d7d7d,0x646464},{0x560c7b,0x834e9e},{0xdb3892,0xd05fa1}};native_palette[0]=badger[theme][0];native_palette[1]=badger[theme][1];native_palette[2]=0x32363b;}
    if(facade){static const uint32_t colors[7]={0x508dde,0x41ae08,0x9a1111,0xe6712b,0x6e6e6e,0x4c0271,0xbc4b8d};native_palette[0]=native_palette[1]=colors[theme];native_palette[2]=0;}
    if(black_phone){native_palette[0]=0x282828;native_palette[1]=0x646464;}
    const uint32_t *palette=native_palette;
    visible(glass,facade);
    bool light_story=isstory || p->page==PHONE_READER;
    lv_obj_set_style_bg_color(reader_panel,iscall&&facade?content_color(p->world.role,palette[1]):content_color(p->world.role,0xffffff),0);
    lv_obj_set_style_text_color(body,facade&&!light_story?content_color(p->world.role,0xffffff):lv_color_black(),0);
    if(facade&&!light_story&&!iscall)lv_obj_set_style_bg_color(reader_panel,lv_color_black(),0);
    lv_obj_set_style_text_color(reader_sender,lv_color_black(),0);
    lv_obj_set_style_bg_color(contact_list,facade&&!light_story?lv_color_black():content_color(p->world.role,0xd2d2d2),0);

    lv_obj_set_style_bg_color(header_bar,content_color(p->world.role,facade?(light_story?0xebebeb:0):(role==0&&theme==0&&!black_phone?0x385e82:palette[0])),0);
    lv_obj_set_style_text_color(header,facade&&light_story?lv_color_black():content_color(p->world.role,0xffffff),0);
    visible(header_bar,!iscall && !(role==1&&ishome));
    lv_obj_set_style_text_color(call_name,facade?content_color(p->world.role,0xffffff):lv_color_black(),0);
    lv_obj_set_style_text_color(call_state,facade?content_color(p->world.role,0xffffff):lv_color_black(),0);
    visible(body,!iscall && !numberpad_active);

    lv_obj_set_style_bg_color(footer_bar,content_color(role,0x191d1e),0);

    lv_obj_set_style_bg_color(selection,content_color(p->world.role,palette[1]),0);lv_obj_set_style_bg_opa(selection,role==1?191:100,0);visible(selection,!facade);
    bool badger_default=role==1 && p->page!=PHONE_PHOTO && !viewfinder && (background_preview?p->selected_option:p->settings[SET_BACKGROUND])==0;
    lv_image_set_src(wallpaper,(p->page==PHONE_PHOTO || viewfinder)?snap_images[p->selected_photo]:isegg?&ui_egg_poster:badger_default?&ui_badger_default:ui_wallpapers[background_preview?p->selected_option:isphoto?p->selected_photo:p->settings[SET_BACKGROUND]]);
    lv_image_set_pivot(wallpaper,0,0);lv_image_set_scale(wallpaper,badger_default&&!isegg?512:256);lv_obj_set_pos(wallpaper,isegg?40:0,isegg?65:60);
    lv_obj_set_style_image_recolor(wallpaper,lv_color_black(),0);
    lv_obj_set_style_image_recolor_opa(wallpaper,black_phone&&ishome?140:0,0);
    visible(wallpaper,isegg || isphoto || (ishome&&!facade));visible(home,ishome);visible(contact_list,!background_preview&&(iscontacts||issettings||islist));visible(reader_panel,iscall || (!isegg&&!ishome&&!iscontacts&&!issettings&&!islist&&!isphoto));
    visible(call_face,iscall || (p->page==PHONE_READER&&p->reader_kind==STORY_TEXT));visible(call_name,iscall);visible(call_state,iscall);
    visible(soft[1],iscontacts);visible(soft[2],true);
    lv_image_set_src(soft[2],p->page==PHONE_INCOMING?&ui_soft_hangup:&ui_soft_back);
    lv_image_set_src(soft[0],ishome?&ui_soft_add:iscall?&ui_soft_hangup:&ui_soft_select);
    visible(soft[0],p->page!=PHONE_READER && p->page!=PHONE_PROGRESS && !(p->page==PHONE_CALL_HISTORY&&!phone_comm_history_count(p)) && !(p->page==PHONE_GALLERY&&!phone_photo_count(p)) && !isegg);
    lv_obj_set_style_image_recolor_opa(soft[0],255,0);lv_obj_set_style_image_recolor(soft[0],content_color(role,iscall&&p->page!=PHONE_INCOMING?0xd71920:0x65c86f),0);
    lv_obj_set_style_image_recolor_opa(soft[1],255,0);lv_obj_set_style_image_recolor(soft[1],content_color(role,0x548ec8),0);
    lv_obj_set_style_image_recolor_opa(soft[2],255,0);lv_obj_set_style_image_recolor(soft[2],content_color(role,0xd71920),0);


    if(dial_controls || story_call_controls)for(unsigned i=0;i<3;i++)visible(soft[i],false);
    if(numberpad_active){
        lv_label_set_text(header,"Dial Pad");
        memcpy(keypad_number,p->dial_number,sizeof(keypad_number));
        keypad_selected=p->selected_digit;
        keypad_focus=content_color(role,palette[1]);
        lv_obj_scroll_to_y(reader_panel,0,LV_ANIM_OFF);
        lv_obj_invalidate(reader_panel);
        return;
    }
    if(background_preview){lv_label_set_text_fmt(header,"Wallpaper %u/%u",p->selected_option+1,setting_counts[SET_BACKGROUND]);return;}
    if(isegg){lv_label_set_text(header,"VICE CITY / VI");return;}
    if(ishome) {
        lv_label_set_text(header,p->selected_app==8?phone_apps[8]:"Home");
        lv_obj_set_size(selection,60,60);
        lv_obj_set_pos(selection,home_icon_x(role,p->selected_app%3)-30,home_icon_y(p->selected_app/3)-30);
        int icon_w=facade?64:role==1?60:48,icon_h=role?60:48;
        for(unsigned i=0;i<9;i++) {
            lv_obj_t *icon=lv_obj_get_child(home,(int)i+1);
            lv_image_set_src(icon,role?skin_icons[role-1][i]:icons[i]);
            lv_image_set_inner_align(icon,LV_IMAGE_ALIGN_CENTER);
            lv_obj_set_size(icon,icon_w,icon_h);
            lv_obj_set_pos(icon,home_icon_x(role,i%3)-icon_w/2,home_icon_y(i/3)-icon_h/2);
            lv_image_set_scale(icon,256);lv_obj_set_style_image_opa(icon,255,0);
            lv_obj_set_style_bg_opa(icon,facade?LV_OPA_COVER:LV_OPA_TRANSP,0);
            lv_obj_set_style_bg_color(icon,i==p->selected_app?content_color(p->world.role,0xffffff):content_color(p->world.role,palette[1]),0);
            lv_obj_set_style_image_recolor_opa(icon,facade&&i!=6?LV_OPA_COVER:LV_OPA_TRANSP,0);
            lv_obj_set_style_image_recolor(icon,i==p->selected_app?content_color(p->world.role,palette[1]):content_color(p->world.role,0xffffff),0);
            bool notice=i<3 && (i==2?world_count(&p->world,false)>0:unread_count(p,i==0?STORY_EMAIL:STORY_TEXT)>0);
            lv_image_set_offset_x(icon,facade&&notice?-4:0);lv_image_set_offset_y(icon,facade&&notice?7:0);
        }
    } else if(iscontacts) {
        lv_label_set_text(header,"Contacts");unsigned first=window_start(p,p->selected_contact,phonebook_count(p->world.role));
        for(unsigned i=0;i<5;i++) {
            unsigned n=first+i;visible(rows[i],n<phonebook_count(p->world.role));if(n>=phonebook_count(p->world.role))continue;
            bool chosen=n==p->selected_contact;
            lv_obj_set_style_bg_color(rows[i],content_color(p->world.role,chosen?palette[1]:facade&&!light_story?0:0xebebeb),0);
            visible(faces[i],true);lv_obj_set_pos(faces[i],3,2);lv_obj_set_style_image_recolor_opa(faces[i],0,0);lv_obj_set_style_text_font(names[i],&ui_game_font_18,0);lv_obj_set_y(names[i],11);lv_obj_set_height(names[i],20);lv_obj_set_x(names[i],46);lv_obj_set_width(names[i],190);
            lv_obj_set_style_text_color(names[i],chosen||(facade&&!light_story)?content_color(p->world.role,0xffffff):lv_color_black(),0);
            const phone_contact_t *contact=phonebook_contact(p->world.role,n);lv_label_set_text(names[i],contact?contact->name:"Unknown");lv_image_set_src(faces[i],ui_portrait_lookup(contact?contact->portrait_key:"char_default"));lv_image_set_scale(faces[i],160);lv_obj_set_style_image_opa(faces[i],255,0);
        }
    } else if(issettings) {
        bool options=p->page==PHONE_SETTING_OPTIONS;
        unsigned selected=options?p->selected_option:p->selected_setting;
        unsigned count=options?setting_counts[p->selected_setting]:SET_COUNT+2;
        unsigned first=window_start(p,selected,count);
        lv_label_set_text(header,options?setting_options[p->selected_setting][selected]:"Settings");
        for(unsigned i=0;i<5;i++) {
            unsigned n=first+i;visible(rows[i],n<count);if(n>=count)continue;
            visible(faces[i],false);lv_obj_set_style_text_font(names[i],&ui_game_font_18,0);lv_obj_set_y(names[i],11);lv_obj_set_height(names[i],20);lv_obj_set_x(names[i],6);lv_obj_set_width(names[i],228);
            bool chosen=n==selected;
            lv_obj_set_style_bg_color(rows[i],content_color(p->world.role,chosen?palette[1]:facade&&!light_story?0:0xebebeb),0);
            lv_obj_set_style_text_color(names[i],chosen||(facade&&!light_story)?content_color(p->world.role,0xffffff):lv_color_black(),0);
            const char *name=options?setting_options[p->selected_setting][n]:n==SET_COUNT?"Character":n==SET_COUNT+1?"Call history":setting_names[n];
            if(options && n==p->settings[p->selected_setting])lv_label_set_text_fmt(names[i],"%s %s",LV_SYMBOL_OK,name);
            else lv_label_set_text(names[i],name);
            if(options && p->selected_setting==SET_THEME && n==0)lv_label_set_text(names[i],"Character default");
        }
    } else if(islist) {
        unsigned count=0,selected=0;
        const char *title="";
        if(p->page==PHONE_CONTACT_ACTIONS){count=2;selected=p->contact_action;title=phone_current_contact(p)->name;}
        else if(story_call_controls){count=phone_story_call_count(role);selected=p->selected_story_call;title=phone_apps[8];}
        else if(p->page==PHONE_NUMBER_LIST){count=phone_number_list_count(p);selected=p->selected_number;title="Dial Pad";}
        else if(p->page==PHONE_CALL_HISTORY){count=phone_comm_history_count(p);selected=p->selected_history;title="Recent calls";}
        else if(isstory){unsigned kind=p->page==PHONE_EMAIL?STORY_EMAIL:p->page==PHONE_MESSAGES?STORY_TEXT:STORY_WEB;count=phone_story_count(p,(story_kind_t)kind);selected=p->selected_story[kind];title=kind==STORY_EMAIL?"Email":kind==STORY_TEXT?"Texts":"Internet";}
        else if(p->page==PHONE_CAMERA){count=2;selected=p->selected_list;title="Snapmatic";}
        else if(p->page==PHONE_GALLERY){count=phone_photo_count(p);selected=p->selected_list;title="Photos";}
        else if(p->page==PHONE_ROLE){count=3;selected=p->selected_role;title="Character";}
        else if(p->page==PHONE_CHECKLIST){count=2;selected=p->selected_list;title="Check List";}
        else if(p->page==PHONE_OPEN_MISSIONS){count=world_count(&p->world,false);selected=p->selected_task;title=checklist_labels[0];}
        else {count=5;title=checklist_labels[1];}
        lv_label_set_text(header,title);
        unsigned first=window_start(p,selected,count);
        for(unsigned i=0;i<5;i++) {
            unsigned row=first+i;
            bool empty=count==0;
            visible(rows[i],i<view_rows && (row<count || (empty&&i==0)));if(i>=view_rows || (row>=count&&!empty))continue;
            visible(faces[i],false);lv_obj_set_style_text_font(names[i],&ui_game_font_18,0);lv_obj_set_y(names[i],11);lv_obj_set_height(names[i],20);lv_obj_set_x(names[i],6);lv_obj_set_width(names[i],228);
            bool chosen=row==selected && !empty && p->page!=PHONE_PROGRESS;
            lv_obj_set_style_bg_color(rows[i],content_color(p->world.role,chosen?palette[1]:facade&&!light_story?0:0xebebeb),0);
            lv_obj_set_style_text_color(names[i],chosen||(facade&&!light_story)?content_color(p->world.role,0xffffff):lv_color_black(),0);
            if(p->page==PHONE_CONTACT_ACTIONS)lv_label_set_text(names[i],row?"Hang Out":"Call");
            else if(story_call_controls){
                const call_scene_t *scene=call_scene_get((unsigned)phone_story_call_index(role,row));
                if(!scene){lv_label_set_text(names[i],"No story calls");continue;}
                lv_label_set_text(names[i],scene->caller_name);
                lv_obj_set_pos(names[i],46,1);lv_obj_set_width(names[i],188);
                visible(faces[i],true);
                const lv_image_dsc_t *portrait=ui_portrait_lookup(scene->portrait_key);
                lv_image_set_src(faces[i],portrait?portrait:ui_portrait_lookup("char_default"));
                lv_image_set_scale(faces[i],160);lv_obj_set_pos(faces[i],3,2);
                lv_obj_set_style_image_opa(faces[i],255,0);lv_obj_set_style_image_recolor_opa(faces[i],0,0);
                row_subx[i]=46;row_subfont[i]=&ui_game_font_16;
                row_subcolor[i]=chosen||facade?lv_color_white():lv_color_hex(0x565656);
                fit_line(row_subtitle[i],sizeof(row_subtitle[i]),scene->title,188,row_subfont[i]);
            }
            else if(p->page==PHONE_NUMBER_LIST){
                int index=phone_number_list_contact(p,row);
                const phone_contact_t *contact=index<0?NULL:phonebook_contact(role,(unsigned)index);
                lv_label_set_text(names[i],contact?contact->name:"Enter a number");
                if(contact){
                    lv_obj_set_y(names[i],1);
                    snprintf(row_subtitle[i],sizeof(row_subtitle[i]),"%.3s-%.3s-%s",contact->number,contact->number+3,contact->number+6);
                    row_subx[i]=6;
                    row_subfont[i]=&ui_game_font_16;
                    row_subcolor[i]=chosen||(facade&&!light_story)?lv_color_white():lv_color_hex(0x565656);
                }
            }
            else if(p->page==PHONE_CALL_HISTORY){
                const comm_log_t *log=phone_comm_history(p,row);if(!log){lv_label_set_text(names[i],"No calls yet");continue;}
                int ci=phonebook_find(role,log->contact);const phone_contact_t *c=ci<0?NULL:phonebook_contact(role,(unsigned)ci);
                lv_label_set_text(names[i],c?c->name:"Unknown");lv_obj_set_pos(names[i],46,1);lv_obj_set_width(names[i],188);
                visible(faces[i],true);lv_image_set_src(faces[i],ui_portrait_lookup(c?c->portrait_key:"char_default"));lv_image_set_scale(faces[i],160);lv_obj_set_pos(faces[i],3,2);lv_obj_set_style_image_recolor_opa(faces[i],0,0);
                static const char *status[]={"Received","Dialled","Missed","Rejected"};
                snprintf(row_subtitle[i],sizeof(row_subtitle[i]),"%02u:%02u %s %us",log->minute/60,log->minute%60,status[log->status],log->seconds);row_subx[i]=46;row_subfont[i]=&ui_game_font_16;row_subcolor[i]=chosen?content_color(role,0xffffff):content_color(role,0x606060);
            }
            else if(isstory){
                unsigned kind=p->page==PHONE_EMAIL?STORY_EMAIL:p->page==PHONE_MESSAGES?STORY_TEXT:STORY_WEB;
                const story_item_t *item=phone_story_item(p,(story_kind_t)kind,row);
                if(item && kind<STORY_WEB){
                    bool unread=!phone_story_is_read(p,(story_kind_t)kind,row);
                    visible(faces[i],true);lv_image_set_src(faces[i],state_glyph(role,kind==STORY_EMAIL,unread));lv_image_set_scale(faces[i],256);lv_obj_set_pos(faces[i],4,kind==STORY_EMAIL?14:13);lv_obj_set_style_image_opa(faces[i],255,0);
                    lv_obj_set_style_image_recolor_opa(faces[i],255,0);lv_obj_set_style_image_recolor(faces[i],chosen||facade?content_color(p->world.role,0xffffff):content_color(p->world.role,palette[1]),0);
                    lv_obj_set_x(names[i],27);lv_obj_set_y(names[i],1);lv_obj_set_width(names[i],lv_obj_get_width(rows[i])-35);lv_obj_set_height(names[i],24);
                    lv_obj_set_style_text_font(names[i],((kind==STORY_TEXT&&role==0)||(kind==STORY_EMAIL&&role==2))?&ui_game_font_20:&ui_game_font_18,0);lv_label_set_text(names[i],item->sender);
                    row_subfont[i]=kind==STORY_EMAIL?&ui_game_font_18:&ui_game_font_16;row_subcolor[i]=chosen?content_color(p->world.role,0xffffff):content_color(p->world.role,0x606060);fit_line(row_subtitle[i],sizeof(row_subtitle[i]),item->title,lv_obj_get_width(rows[i])-35,row_subfont[i]);
                }else lv_label_set_text(names[i],item?item->title:"No content installed");
            }
            else if(p->page==PHONE_CAMERA)lv_label_set_text(names[i],row?"View photos":"Take photo (demo)");
            else if(p->page==PHONE_GALLERY)lv_label_set_text(names[i],empty?"No saved photos":snap_names[phone_photo_at(p,row)]);
            else if(p->page==PHONE_ROLE)lv_label_set_text(names[i],world_roles[row]);
            else if(p->page==PHONE_CHECKLIST)lv_label_set_text(names[i],checklist_labels[row]);
            else if(p->page==PHONE_OPEN_MISSIONS){int task=world_task(&p->world,row);lv_label_set_text(names[i],task<0?"No open missions":mission_names[task]);}
            else lv_label_set_text_fmt(names[i],"%s: %u",checklist_labels[i+2],i==0?world_count(&p->world,true):0);
        }
    } else if(isphoto){if(viewfinder)lv_label_set_text_fmt(header,"%s / demo",snap_names[p->selected_photo]);else lv_label_set_text(header,p->photo_from_gallery?snap_names[p->selected_photo]:"Save photo?");
    } else if(iscall) {
        const phone_contact_t *contact=phone_current_contact(p);
        const call_scene_t *replay=p->story_call && p->return_page==PHONE_STORY_CALLS?call_scene_get((unsigned)p->scene_id):NULL;
        const char *name=replay?replay->caller_name:contact?contact->name:p->dial_number;
        const lv_image_dsc_t *portrait=ui_portrait_lookup(replay?replay->portrait_key:contact?contact->portrait_key:"char_default");
        lv_label_set_text(header,*name?name:"Unknown");lv_label_set_text(call_name,name);
        lv_image_set_src(call_face,portrait?portrait:ui_portrait_lookup("char_default"));
        if(p->page==PHONE_INCOMING)lv_label_set_text(call_state,"INCOMING CALL");
        else if(p->page==PHONE_DIALING)lv_label_set_text(call_state,"DIALING...");
        else if(p->page==PHONE_BUSY)lv_label_set_text(call_state,"BUSY");
        else lv_label_set_text_fmt(call_state,"%02lu:%02lu",(unsigned long)p->call_seconds/60,(unsigned long)p->call_seconds%60);
    } else {
        if(p->page==PHONE_PURCHASE_CONFIRM || p->page==PHONE_TRANSACTION_RESULT){lv_obj_scroll_to_y(reader_panel,0,LV_ANIM_OFF);lv_label_set_text(header,"Offline checkout");if(p->page==PHONE_PURCHASE_CONFIRM)lv_label_set_text_fmt(body,"Donate $%u?\n\nVirtual balance: $%lu\n\nOK: confirm\nHold OK: cancel\n\nLocal simulation only",p->purchase_item?5000:500,(unsigned long)p->sim.balance[p->world.role]);else lv_label_set_text(body,p->save_status>0?"Transaction recorded.\nSee Account for details.":"Not processed.\nInsufficient funds or\nalready recorded.");return;}
        if(p->page==PHONE_READER){
            lv_label_set_text(header,p->reader_kind==STORY_TEXT?"Texts":p->reader_kind==STORY_EMAIL?"Email":"Internet");
            const story_item_t *item=phone_story_item(p,p->reader_kind,p->selected_story[p->reader_kind]);
            if(p->reader_kind==STORY_EMAIL){
                lv_label_set_text_static(reader_sender,item?item->sender:"");lv_label_set_text_static(reader_subject,item?item->title:"");lv_obj_set_pos(reader_sender,7,1);lv_obj_set_pos(reader_subject,7,24);lv_obj_update_layout(reader_subject);
            }else if(p->reader_kind==STORY_TEXT){
                lv_label_set_text_static(reader_sender,item?item->sender:"");lv_obj_set_pos(reader_sender,60,4);
                const lv_image_dsc_t *portrait=ui_portrait_lookup("char_default");
                for(unsigned r=0;item&&r<3;r++)for(unsigned i=0;i<phonebook_count(r);i++){const phone_contact_t *contact=phonebook_contact(r,i);if(!strcmp(contact->name,item->sender)){portrait=ui_portrait_lookup(contact->portrait_key);break;}}
                lv_image_set_src(call_face,portrait);lv_image_set_scale(call_face,208);lv_obj_set_pos(call_face,5,4);
            }
            lv_label_set_text_static(body,item?item->body:"");
            int body_y=p->reader_kind==STORY_WEB?8:p->reader_kind==STORY_TEXT?64:56;
            if(p->reader_kind==STORY_EMAIL && body_y<lv_obj_get_y(reader_subject)+lv_obj_get_height(reader_subject)+8)body_y=lv_obj_get_y(reader_subject)+lv_obj_get_height(reader_subject)+8;
            lv_obj_set_y(body,body_y);lv_obj_update_layout(reader_panel);
            int limit=lv_obj_get_scroll_y(reader_panel)+lv_obj_get_scroll_bottom(reader_panel);if(limit<0)limit=0;
            unsigned max_step=(unsigned)(limit+19)/20;p->reader_limit=max_step;if(p->reader_scroll>max_step)p->reader_scroll=max_step;
            int target=(int)p->reader_scroll*20;if(target>limit)target=limit;
            bool moved=reader_active && p->reader_scroll!=reader_previous_scroll;
            lv_obj_scroll_to_y(reader_panel,target,LV_ANIM_OFF);
            int viewport=lv_obj_get_height(reader_panel),height=viewport*viewport/(viewport+limit);
            if(height<8)height=8;
            lv_obj_set_pos(scroll_thumb,lv_display_get_horizontal_resolution(NULL)-5,lv_obj_get_y(reader_panel)+(limit?(viewport-height)*target/limit:0));lv_obj_set_size(scroll_thumb,3,height);
            if(!limit)lv_obj_set_style_bg_opa(scroll_thumb,0,0);
            else if(moved){lv_anim_delete(scroll_thumb,scrollbar_alpha);lv_anim_t a;lv_anim_init(&a);lv_anim_set_var(&a,scroll_thumb);lv_anim_set_exec_cb(&a,scrollbar_alpha);lv_anim_set_values(&a,128,0);lv_anim_set_delay(&a,300);lv_anim_set_duration(&a,2500);lv_anim_start(&a);scrollbar_alpha(scroll_thumb,128);}
            reader_active=true;reader_previous_scroll=p->reader_scroll;return;
        }
        lv_obj_scroll_to_y(reader_panel,0,LV_ANIM_OFF);
        if(p->page==PHONE_DIAL_RESULT){
            bool taxi=phone_current_contact(p)!=NULL;
            lv_label_set_text(header,taxi?"Downtown Cab Co.":"Black Cellphones");
            lv_label_set_text(body,taxi?"Cab dispatched.\nA text will follow.\n\n[Local dispatch]":black_phone?"Black theme enabled.":"Black theme cleared.");
            return;
        }
        if(p->page==PHONE_SAVE || p->page==PHONE_SAVE_RESULT){lv_label_set_text(header,"Quick Save");lv_label_set_text(body,p->save_status>0?"Progress saved":p->save_status<0?"Save failed. Please retry.":"Saving...");return;}
        lv_label_set_text(header,p->page==PHONE_TASK_DETAIL?"Mission":p->page==PHONE_MESSAGES?"Texts":phone_apps[p->selected_app]);
        if(p->page==PHONE_TASK_DETAIL){int task=world_task(&p->world,p->selected_task);lv_label_set_text_fmt(body,"%s\n\nLocal replay\n\nOK: complete simulation\nHold OK: back",task>=0?mission_names[task]:"");}
        else if(p->page==PHONE_MESSAGES && world_count(&p->world,false)){int task=world_task(&p->world,0);lv_label_set_text_fmt(body,"%s\n\n%s\n\nLocal replay available\nin Check List",world_roles[p->world.role],mission_names[task]);}
        else lv_label_set_text(body,p->page==PHONE_MESSAGES?"No messages":"No content installed");
    }
}

static unsigned unread_count(const phone_t *p,story_kind_t kind){
 unsigned count=phone_story_count(p,kind),unread=0;
 for(unsigned i=0;i<count;i++)if(!phone_story_is_read(p,kind,i))unread++;
 return unread;
}
void phone_ui_render(phone_t *p) {
 bool changed=!rendered || previous_page!=p->page || previous_role!=p->world.role;
 bool landscape=false; /* Fixed device: every application stays portrait. */
 lv_display_t *display=lv_display_get_default();
 lv_display_rotation_t rotation=landscape?LV_DISPLAY_ROTATION_90:LV_DISPLAY_ROTATION_0;
 if(lv_display_get_rotation(display)!=rotation)lv_display_set_rotation(display,rotation);
 int w=lv_display_get_horizontal_resolution(display),h=lv_display_get_vertical_resolution(display);
 bool full=p->page==PHONE_INCOMING || p->page==PHONE_DIALING || p->page==PHONE_CALL || p->page==PHONE_BUSY;
 lv_obj_set_pos(header_bar,0,24);lv_obj_set_size(header_bar,w,36);lv_obj_set_width(header,w-6);lv_obj_set_height(header,30);
 chrome.role=p->world.role;chrome.home=p->page==PHONE_HOME;chrome.landscape=landscape;chrome.day=p->clock_day;chrome.profile=p->settings[SET_PROFILE];chrome.email=unread_count(p,STORY_EMAIL);chrome.texts=unread_count(p,STORY_TEXT);lv_obj_set_size(signal_bars,w,80);lv_obj_set_pos(signal_bars,0,0);lv_obj_invalidate(signal_bars);
 lv_obj_set_style_text_font(clock_text,chrome.role==1&&chrome.home?&ui_game_font_28:&ui_game_font_18,0);
 if(chrome.role==1) {lv_obj_set_width(clock_text,chrome.home?120:68);lv_obj_set_pos(clock_text,chrome.home?w/2-60:3,1);}
 else if(chrome.role==2){lv_obj_set_width(clock_text,46);lv_obj_set_pos(clock_text,w-47,1);}
 else{lv_obj_set_width(clock_text,75);lv_obj_set_pos(clock_text,w/2-37,1);}
 lv_label_set_text_fmt(clock_text,"%02lu:%02lu",(unsigned long)p->clock_minutes/60,(unsigned long)p->clock_minutes%60);
 lv_obj_set_pos(footer_bar,0,h-44);lv_obj_set_size(footer_bar,w,44);
 for(unsigned i=0;i<3;i++){lv_obj_set_x(soft[i],w*(i==0?3:i==1?1:5)/6-13);lv_obj_set_y(soft[i],9);lv_image_set_pivot(soft[i],0,0);lv_image_set_scale(soft[i],290);}
 lv_obj_set_pos(reader_panel,0,full?24:60);lv_obj_set_size(reader_panel,w,h-(full?68:104));
 view_rows=5;view_top=60;view_height=h-104;lv_obj_set_size(contact_list,w,view_height);for(unsigned i=0;i<5;i++){lv_obj_set_pos(rows[i],0,(int)i*view_height/(int)view_rows);lv_obj_set_size(rows[i],w,view_height/(int)view_rows);}
 lv_obj_set_width(reader_sender,w-(p->reader_kind==STORY_TEXT?67:14));lv_obj_set_width(reader_subject,w-14);lv_obj_set_width(body,w-14);
 lv_image_set_rotation(glass,landscape?900:0);lv_obj_set_pos(glass,landscape?w:0,0);
 bool badger=p->world.role==1;
 lv_obj_set_pos(call_name,badger?8:6,badger?28:35);lv_obj_set_width(call_name,w-12);
 lv_image_set_scale(call_face,240);lv_obj_set_pos(call_face,badger?13:6,badger?53:63);lv_obj_set_pos(call_state,badger?82:77,badger?57:63);
 if(changed){lv_anim_delete(scroll_thumb,scrollbar_alpha);lv_obj_set_style_bg_opa(scroll_thumb,0,0);}
 lv_obj_t *face_parent=p->page==PHONE_READER&&p->reader_kind==STORY_TEXT?reader_panel:lv_screen_active();if(lv_obj_get_parent(call_face)!=face_parent)lv_obj_set_parent(call_face,face_parent);
 render_page(p);
 if(changed && (dial_controls || story_call_controls || previous_page==PHONE_NUMBER_LIST || previous_page==PHONE_NUMBERPAD || previous_page==PHONE_STORY_CALLS))lv_obj_invalidate(footer_bar);
 if(p->page==PHONE_HOME){
  unsigned counts[3]={unread_count(p,STORY_EMAIL),unread_count(p,STORY_TEXT),world_count(&p->world,false)};
  for(unsigned i=0;i<3;i++){
   home_badge_count[i]=counts[i];snprintf(badge_text[i],sizeof(badge_text[i]),"%u",counts[i]>99?99:counts[i]);
   badge_color[i]=p->world.role==2?lv_obj_get_style_image_recolor(lv_obj_get_child(home,(int)i+1),0):lv_color_white();
  }
  home_role=p->world.role;lv_obj_invalidate(home);

 }
 previous_page=p->page;previous_role=p->world.role;rendered=true;
}
