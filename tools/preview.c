#include "lvgl.h"
#include "ui.h"
#include <stdio.h>
#include <stdint.h>
static uint16_t buffer[240*20],pixels[240*320];
static void flush(lv_display_t *d,const lv_area_t *a,uint8_t *data) {
 uint16_t *p=(uint16_t *)data;
 for(int y=a->y1;y<=a->y2;y++)for(int x=a->x1;x<=a->x2;x++)pixels[y*240+x]=*p++;
 lv_display_flush_ready(d);
}
static void save(const char *dir,const char *name,phone_t *p) {
 phone_ui_render(p);for(int n=0;n<5;n++){lv_tick_inc(20);lv_timer_handler();}lv_refr_now(NULL);
 char path[1024];snprintf(path,sizeof(path),"%s/%s.ppm",dir,name);FILE *f=fopen(path,"wb");if(!f)return;
 fprintf(f,"P6\n240 320\n255\n");
 for(int n=0;n<240*320;n++){uint16_t c=pixels[n];uint8_t rgb[]={((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);
}
int main(int argc,char **argv) {
 if(argc!=2)return 1;lv_init();lv_display_t *d=lv_display_create(240,320);lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(d,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(d,flush);
 phone_ui_init();phone_t p;phone_init(&p);save(argv[1],"home",&p);
 p.page=PHONE_CONTACTS;p.selected_contact=1;save(argv[1],"contacts",&p);
 phone_incoming(&p,1);save(argv[1],"incoming",&p);phone_key(&p,KEY_OK);p.call_seconds=18;save(argv[1],"call",&p);
 p.page=PHONE_MESSAGES;save(argv[1],"messages",&p);
 phone_init(&p);p.page=PHONE_SETTINGS;save(argv[1],"settings",&p);
 phone_key(&p,KEY_OK);save(argv[1],"background_options",&p);
 phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);phone_key(&p,KEY_BACK);
 p.selected_setting=SET_THEME;phone_key(&p,KEY_OK);p.selected_option=5;phone_key(&p,KEY_OK);save(argv[1],"theme_options",&p);
 phone_key(&p,KEY_BACK);phone_key(&p,KEY_BACK);save(argv[1],"custom_home",&p);
 p.page=PHONE_CONTACTS;p.selected_contact=3;phone_key(&p,KEY_OK);save(argv[1],"dialing",&p);
 phone_advance(&p,3000);save(argv[1],"answerphone",&p);
 phone_audio_finished(&p);save(argv[1],"returned_contacts",&p);
 p.selected_contact=0;phone_key(&p,KEY_OK);phone_advance(&p,3000);save(argv[1],"busy",&p);
 phone_init(&p);phone_key(&p,KEY_ROLE);save(argv[1],"character_select",&p);phone_key(&p,KEY_OK);
 phone_advance(&p,45000);p.selected_app=2;phone_key(&p,KEY_OK);save(argv[1],"checklist",&p);
 phone_key(&p,KEY_OK);save(argv[1],"open_missions",&p);phone_key(&p,KEY_OK);save(argv[1],"mission_detail",&p);
 phone_key(&p,KEY_OK);phone_key(&p,KEY_BACK);phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);save(argv[1],"progress",&p);
 lv_deinit();return 0;
}
