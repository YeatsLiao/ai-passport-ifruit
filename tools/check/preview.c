#include "lvgl.h"
#include "ui.h"
#include <stdio.h>
#include <stdint.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
static uint16_t buffer[240*20],pixels[240*320];
static void flush(lv_display_t *d,const lv_area_t *a,uint8_t *data) {
 uint16_t *p=(uint16_t *)data;
 for(int y=a->y1;y<=a->y2;y++)for(int x=a->x1;x<=a->x2;x++)pixels[y*lv_display_get_horizontal_resolution(d)+x]=*p++;
 lv_display_flush_ready(d);
}
static void save_step(const char *dir,const char *name,phone_t *p,unsigned ms) {
 phone_ui_render(p);assert(lv_display_get_rotation(NULL)==LV_DISPLAY_ROTATION_0 && lv_display_get_horizontal_resolution(NULL)==240 && lv_display_get_vertical_resolution(NULL)==320);for(unsigned n=0;n<ms;n+=20){lv_tick_inc(20);lv_timer_handler();}lv_refr_now(NULL);
 char path[1024];snprintf(path,sizeof(path),"%s/%s.ppm",dir,name);FILE *f=fopen(path,"wb");if(!f)return;
 fprintf(f,"P6\n%d %d\n255\n",lv_display_get_horizontal_resolution(NULL),lv_display_get_vertical_resolution(NULL));
 for(int n=0;n<240*320;n++){uint16_t c=pixels[n];uint8_t rgb[]={((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);
}
static void save(const char *dir,const char *name,phone_t *p){save_step(dir,name,p,400);}
int main(int argc,char **argv) {
 if(argc!=2)return 1;lv_init();lv_display_t *d=lv_display_create(240,320);lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(d,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(d,flush);
 phone_ui_init();phone_t p;phone_init(&p);save(argv[1],"home",&p);
 p.page=PHONE_CONTACTS;p.selected_contact=(unsigned)phonebook_find(p.world.role,14);save(argv[1],"contacts",&p);
 phone_incoming_scene(&p,call_scene_default_incoming(p.world.role));save(argv[1],"incoming",&p);phone_key(&p,KEY_OK);p.call_seconds=18;save(argv[1],"call",&p);
 p.page=PHONE_MESSAGES;save(argv[1],"messages",&p);
 phone_init(&p);p.page=PHONE_SETTINGS;p.selected_setting=SET_COUNT;save(argv[1],"settings_character",&p);phone_key(&p,KEY_OK);save(argv[1],"settings_character_roles",&p);phone_key(&p,KEY_BACK);p.page=PHONE_SETTINGS;p.selected_setting=0;save(argv[1],"settings",&p);
 phone_key(&p,KEY_OK);save(argv[1],"background_options",&p);
 phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);phone_key(&p,KEY_BACK);
 p.selected_setting=SET_THEME;phone_key(&p,KEY_OK);p.selected_option=5;phone_key(&p,KEY_OK);save(argv[1],"theme_options",&p);
 phone_key(&p,KEY_BACK);phone_key(&p,KEY_BACK);save(argv[1],"custom_home",&p);
 p.page=PHONE_CONTACTS;p.selected_contact=(unsigned)phonebook_find(p.world.role,17);phone_key(&p,KEY_OK);if(p.page==PHONE_CONTACT_ACTIONS)phone_key(&p,KEY_OK);save(argv[1],"dialing",&p);
 phone_advance(&p,3000);save(argv[1],"answerphone",&p);
 phone_audio_finished(&p);save(argv[1],"returned_contacts",&p);
 for(unsigned c=0;c<phonebook_count(p.world.role);c++){const phone_contact_t *contact=phonebook_contact(p.world.role,c);if(contact->outgoing_scene<0 && (!contact->answerphone_file || !*contact->answerphone_file)){p.selected_contact=c;phone_key(&p,KEY_OK);phone_advance(&p,3000);save(argv[1],"busy",&p);break;}}
 phone_init(&p);phone_key(&p,KEY_BACK);save(argv[1],"character_select",&p);phone_key(&p,KEY_OK);
 phone_advance(&p,45000);p.selected_app=2;phone_key(&p,KEY_OK);save(argv[1],"checklist",&p);
 phone_key(&p,KEY_OK);save(argv[1],"open_missions",&p);phone_key(&p,KEY_OK);save(argv[1],"mission_detail",&p);
 phone_key(&p,KEY_OK);phone_key(&p,KEY_BACK);phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);save(argv[1],"progress",&p);
 phone_init(&p);p.selected_app=0;phone_key(&p,KEY_OK);save(argv[1],"email_list",&p);phone_key(&p,KEY_OK);save(argv[1],"email_reader",&p);
 phone_key(&p,KEY_BACK);phone_key(&p,KEY_BACK);p.selected_app=7;phone_key(&p,KEY_OK);save(argv[1],"web_list",&p);phone_key(&p,KEY_OK);save(argv[1],"web_reader",&p);
 phone_init(&p);p.selected_app=6;phone_key(&p,KEY_OK);save(argv[1],"camera_menu",&p);phone_key(&p,KEY_OK);
 for(unsigned i=0;i<SNAP_SCENES;i++){char name[64];p.selected_photo=i;snprintf(name,sizeof(name),"snap_scene_%u",i);save(argv[1],name,&p);}
 p.selected_photo=0;phone_key(&p,KEY_OK);save(argv[1],"snap_save",&p);phone_key(&p,KEY_OK);save(argv[1],"snap_gallery",&p);
 phone_save_result(&p,true);save(argv[1],"save_result",&p);

 phone_init(&p);p.page=PHONE_CONTACTS;p.selected_contact=(unsigned)phonebook_find(0,1);phone_key(&p,KEY_OK);save(argv[1],"contact_actions",&p);phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);phone_advance(&p,3000);save(argv[1],"hangout_call",&p);phone_audio_finished(&p);
 p.page=PHONE_INTERNET;p.selected_story[STORY_WEB]=0;phone_key(&p,KEY_OK);save(argv[1],"account",&p);phone_key(&p,KEY_BACK);p.selected_story[STORY_WEB]=1;phone_key(&p,KEY_OK);save(argv[1],"checkout",&p);phone_key(&p,KEY_OK);save(argv[1],"transaction",&p);
 phone_init(&p);p.page=PHONE_INTERNET;p.selected_story[STORY_WEB]=phone_story_count(&p,STORY_WEB)-1;save(argv[1],"vice_city_ad",&p);phone_key(&p,KEY_OK);save(argv[1],"vice_city_poster",&p);phone_key(&p,KEY_BACK);p.page=PHONE_MESSAGES;p.selected_story[STORY_TEXT]=phone_story_count(&p,STORY_TEXT)-1;phone_key(&p,KEY_OK);save(argv[1],"vice_city_message",&p);
 for(unsigned role=0;role<3;role++){
  phone_init(&p);p.world.role=role;char title[64];snprintf(title,sizeof(title),"skin_%u_home",role);save(argv[1],title,&p);
  p.page=PHONE_CONTACTS;snprintf(title,sizeof(title),"skin_%u_contacts",role);save(argv[1],title,&p);
  p.page=PHONE_SETTINGS;p.selected_setting=SET_RINGTONE;phone_key(&p,KEY_OK);p.selected_option=13;snprintf(title,sizeof(title),"skin_%u_ringtones",role);save(argv[1],title,&p);
  phone_key(&p,KEY_BACK);phone_key(&p,KEY_BACK);
  phone_incoming_scene(&p,call_scene_default_incoming(role));snprintf(title,sizeof(title),"skin_%u_incoming",role);save(argv[1],title,&p);
 }
 for(unsigned role=0;role<3;role++)for(unsigned kind=0;kind<3;kind++){

  p.world.role=role;p.reader_kind=(story_kind_t)kind;p.page=PHONE_READER;
  for(unsigned n=0;n<phone_story_count(&p,(story_kind_t)kind);n++){
   p.selected_story[kind]=n;p.reader_scroll=0;phone_ui_render(&p);lv_tick_inc(20);lv_timer_handler();lv_refr_now(NULL);
  }
 }
 phone_init(&p);p.page=PHONE_CONTACTS;save(argv[1],"native_contacts_start",&p);p.selected_contact=6;save_step(argv[1],"native_scroll_bar",&p,0);p.selected_contact=5;save_step(argv[1],"native_scroll_up_window",&p,0);
 p.page=PHONE_HOME;save_step(argv[1],"native_home_000",&p,0);assert(lv_anim_count_running()==0);save_step(argv[1],"native_home_125",&p,120);assert(lv_anim_count_running()==0);save_step(argv[1],"native_home_250",&p,140);assert(lv_anim_count_running()==0);
 p.page=PHONE_CONTACTS;phone_ui_render(&p);p.page=PHONE_HOME;
 for(unsigned frame=0;frame<14;frame++){char name[64];snprintf(name,sizeof(name),"native_motion_%02u",frame);save_step(argv[1],name,&p,frame?20:0);}
 p.page=PHONE_EMAIL;p.selected_story[STORY_EMAIL]=0;phone_key(&p,KEY_OK);save(argv[1],"native_email_portrait",&p);phone_key(&p,KEY_DOWN);phone_ui_render(&p);phone_key(&p,KEY_BACK);save(argv[1],"native_email_return",&p);
 for(unsigned role=0;role<3;role++){phone_init(&p);p.world.role=role;p.page=PHONE_MESSAGES;char name[64];snprintf(name,sizeof(name),"native_%u_texts",role);save(argv[1],name,&p);phone_key(&p,KEY_OK);snprintf(name,sizeof(name),"native_%u_text_reader",role);save(argv[1],name,&p);if(role==2){for(unsigned n=0;n<5;n++)phone_key(&p,KEY_DOWN);save(argv[1],"native_sms_scrolled",&p);}}
 for(unsigned role=0;role<3;role++){
  phone_init(&p);p.world.role=role;p.selected_app=0;char name[64];snprintf(name,sizeof(name),"notice_%u_selected",role);save(argv[1],name,&p);
  p.read_bits[role][0]=UINT64_MAX;p.read_bits[role][1]=UINT64_MAX;snprintf(name,sizeof(name),"notice_%u_clear",role);save(argv[1],name,&p);
  p.read_bits[role][0]&=~UINT64_C(1);p.read_bits[role][1]&=~UINT64_C(1);snprintf(name,sizeof(name),"notice_%u_single",role);save(argv[1],name,&p);
 }
 for(unsigned n=0;n<120;n++){
  phone_init(&p);p.world.role=n%3;p.page=PHONE_CONTACTS;p.selected_contact=n%phonebook_count(p.world.role);phone_ui_render(&p);lv_tick_inc(20);lv_timer_handler();
  p.page=PHONE_HOME;phone_ui_render(&p);lv_tick_inc(40);lv_timer_handler();
  p.page=PHONE_READER;p.reader_kind=STORY_EMAIL;p.selected_story[0]=n%story_count(p.world.role,STORY_EMAIL);phone_ui_render(&p);lv_tick_inc(20);lv_timer_handler();
  phone_incoming_scene(&p,call_scene_default_incoming(p.world.role));phone_ui_render(&p);phone_key(&p,KEY_BACK);phone_ui_render(&p);
 }
 phone_init(&p);phone_incoming_scene(&p,call_scene_default_incoming(0));phone_key(&p,KEY_OK);p.call_seconds=12;phone_audio_finished(&p);phone_advance(&p,4000);
 p.page=PHONE_MESSAGES;save(argv[1],"comms_new_message",&p);phone_key(&p,KEY_OK);save(argv[1],"comms_followup",&p);phone_key(&p,KEY_BACK);phone_key(&p,KEY_BACK);
 phone_incoming_scene(&p,call_scene_default_incoming(0));phone_key(&p,KEY_BACK);p.page=PHONE_CALL_HISTORY;save(argv[1],"comms_history",&p);phone_key(&p,KEY_OK);save(argv[1],"comms_callback",&p);
 p.page=PHONE_SETTINGS;p.selected_setting=SET_COUNT+1;save(argv[1],"comms_settings",&p);
 for(unsigned role=0;role<3;role++){
  char name[64];
  phone_init(&p);p.world.role=role;p.selected_app=8;
  snprintf(name,sizeof(name),"story_calls_%u_home",role);save(argv[1],name,&p);
  phone_key(&p,KEY_OK);snprintf(name,sizeof(name),"story_calls_%u_list",role);save(argv[1],name,&p);
  phone_key(&p,KEY_OK);snprintf(name,sizeof(name),"story_calls_%u_incoming",role);save(argv[1],name,&p);
  phone_key(&p,KEY_OK);snprintf(name,sizeof(name),"story_calls_%u_connected",role);save(argv[1],name,&p);
  phone_audio_finished(&p);phone_key(&p,KEY_BACK);
  p.selected_app=4;phone_key(&p,KEY_OK);phone_key(&p,KEY_DIAL_PAD);
  snprintf(name,sizeof(name),"dialpad_%u_numbers",role);save(argv[1],name,&p);
  phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);
  snprintf(name,sizeof(name),"dialpad_%u_keypad",role);save(argv[1],name,&p);
  phone_key(&p,KEY_BACK);phone_key(&p,KEY_UP);phone_key(&p,KEY_OK);
  snprintf(name,sizeof(name),"dialpad_%u_dialing",role);save(argv[1],name,&p);
  phone_advance(&p,3000);if(p.page==PHONE_CALL)phone_audio_finished(&p);
  snprintf(name,sizeof(name),"dialpad_%u_dispatch",role);save(argv[1],name,&p);
  phone_key(&p,KEY_OK);phone_advance(&p,4000);p.page=PHONE_MESSAGES;
  snprintf(name,sizeof(name),"dialpad_%u_text",role);save(argv[1],name,&p);
 }
 p.page=PHONE_HOME;phone_ui_render(&p);lv_tick_inc(300);lv_timer_handler();
 lv_mem_monitor_t monitor;lv_mem_monitor(&monitor);assert(monitor.free_size>=4096 && monitor.free_biggest_size>=2560);
 printf("PASS: instant home entry (no zoom animation), 120 interrupted transitions, portrait-only readers, bounded scroll and 32KB pool\n");printf("LVGL fixed-pool free=%lu bytes, used=%u%%, fragment=%u%%\n",(unsigned long)monitor.free_size,monitor.used_pct,monitor.frag_pct);
 lv_deinit();return 0;
}
