#include "phone.h"
#include <assert.h>
#include <stdio.h>
static void options(phone_t *p,unsigned setting) {
 p->page=PHONE_SETTINGS;p->selected_setting=setting;
 assert(phone_key(p,KEY_OK)==FX_ACCEPT);assert(p->page==PHONE_SETTING_OPTIONS);
}
int main(void) {
 world_t w;world_init(&w);assert(world_valid(&w));
 assert(!world_step(&w,UINT32_MAX,false) && w.wait_ms==45000);
 assert(world_step(&w,45000,true));int task=world_task(&w,0);assert(task>=0 && task<2);
 assert(!world_complete(&w,2));assert(world_complete(&w,(unsigned)task));assert(!world_complete(&w,(unsigned)task));
 assert(world_step(&w,UINT32_MAX,true));assert(world_task(&w,0)!=task);
 w.role=1;assert(world_count(&w,false)==0 && world_count(&w,true)==0);
 assert(world_step(&w,UINT32_MAX,true));assert(world_task(&w,0)>=2 && world_task(&w,0)<4);
 w.active[1]|=1;assert(!world_valid(&w));
 phone_t event_phone;phone_init(&event_phone);event_phone.page=PHONE_INCOMING;
 phone_advance(&event_phone,UINT32_MAX);assert(!world_count(&event_phone.world,false));
 event_phone.page=PHONE_HOME;event_phone.settings[SET_PROFILE]=1;
 phone_advance(&event_phone,UINT32_MAX);assert(!world_count(&event_phone.world,false));
 event_phone.settings[SET_PROFILE]=0;assert(phone_advance(&event_phone,45000)==FX_TEXT);
 event_phone.selected_app=2;phone_key(&event_phone,KEY_OK);phone_key(&event_phone,KEY_OK);
 assert(event_phone.page==PHONE_OPEN_MISSIONS);phone_key(&event_phone,KEY_OK);
 assert(event_phone.page==PHONE_TASK_DETAIL);phone_key(&event_phone,KEY_OK);
 assert(world_count(&event_phone.world,true)==1 && !world_count(&event_phone.world,false));
 phone_key(&event_phone,KEY_BACK);phone_key(&event_phone,KEY_BACK);phone_key(&event_phone,KEY_ROLE);
 phone_key(&event_phone,KEY_DOWN);phone_key(&event_phone,KEY_OK);assert(event_phone.world.role==1);
 phone_t restored;phone_init(&restored);
 uint32_t prefs[SET_COUNT]={14,1,3,6,0};assert(phone_restore_settings(&restored,prefs));
 assert(restored.settings[SET_BACKGROUND]==14 && restored.settings[SET_THEME]==6);
 prefs[SET_THEME]=7;prefs[SET_BACKGROUND]=0;
 assert(!phone_restore_settings(&restored,prefs));assert(restored.settings[SET_BACKGROUND]==14);
 prefs[SET_THEME]=UINT32_MAX;assert(!phone_restore_settings(&restored,prefs));
 phone_t p;phone_init(&p);assert(p.page==PHONE_HOME && p.selected_app==4);
 assert(phone_key(&p,KEY_OK)==FX_ACCEPT);assert(p.page==PHONE_CONTACTS);
 p.selected_contact=2;
 assert(phone_key(&p,KEY_OK)==FX_DIAL);assert(p.page==PHONE_DIALING);
 assert(phone_advance(&p,2999)==FX_NONE);assert(p.page==PHONE_DIALING);
 assert(phone_incoming(&p,1)==FX_NONE);
 assert(phone_advance(&p,1)==FX_VOICE);assert(p.page==PHONE_CALL);
 phone_advance(&p,999);assert(p.call_seconds==0);phone_advance(&p,1);assert(p.call_seconds==1);
 assert(phone_key(&p,KEY_DOWN)==FX_NONE);
 assert(phone_audio_finished(&p)==FX_STOP);assert(p.page==PHONE_CONTACTS);
 assert(phone_audio_finished(&p)==FX_NONE);
 /* A cancelled old completion cannot return a later dialing screen. */
 phone_key(&p,KEY_OK);assert(phone_key(&p,KEY_BACK)==FX_STOP);
 phone_key(&p,KEY_OK);assert(phone_audio_finished(&p)==FX_NONE);assert(p.page==PHONE_DIALING);
 phone_key(&p,KEY_BACK);
 /* Unmapped voices have an explicit local busy outcome, no arbitrary recording. */
 p.selected_contact=0;phone_key(&p,KEY_OK);
 assert(phone_advance(&p,UINT32_MAX)==FX_STOP);assert(p.page==PHONE_BUSY);
 assert(phone_advance(&p,1499)==FX_NONE);assert(phone_advance(&p,1)==FX_STOP);assert(p.page==PHONE_CONTACTS);
 assert(phone_incoming(&p,6)==FX_NONE);assert(phone_incoming(&p,1)==FX_RING);
 assert(phone_key(&p,KEY_DOWN)==FX_NONE);assert(phone_key(&p,KEY_BACK)==FX_STOP);
 assert(p.page==PHONE_CONTACTS);phone_incoming(&p,1);assert(phone_key(&p,KEY_OK)==FX_ACCEPT);
 assert(p.call_seconds==0 && p.page==PHONE_CALL);assert(phone_audio_finished(&p)==FX_NONE);
 phone_key(&p,KEY_BACK);phone_key(&p,KEY_BACK);assert(p.page==PHONE_HOME);
 phone_init(&p);p.selected_app=0;phone_key(&p,KEY_UP);assert(p.selected_app==7);
 p.trackify_unlocked=true;phone_key(&p,KEY_DOWN);assert(p.selected_app==8);
 p.selected_app=1;phone_key(&p,KEY_OK);assert(p.page==PHONE_MESSAGES);
 assert(phone_key(&p,KEY_BACK)==FX_BACK);assert(p.page==PHONE_HOME);
 /* Selection is separate from applied state; cancelling preserves appearance. */
 options(&p,SET_BACKGROUND);phone_key(&p,KEY_UP);assert(p.selected_option==14);
 phone_key(&p,KEY_BACK);assert(p.settings[SET_BACKGROUND]==0 && p.page==PHONE_SETTINGS);
 options(&p,SET_BACKGROUND);phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);assert(p.settings[SET_BACKGROUND]==1);
 phone_key(&p,KEY_BACK);options(&p,SET_THEME);phone_key(&p,KEY_UP);phone_key(&p,KEY_OK);assert(p.settings[SET_THEME]==6);
 phone_key(&p,KEY_BACK);options(&p,SET_PROFILE);phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);
 assert(phone_incoming(&p,1)==FX_NONE && p.page==PHONE_SETTING_OPTIONS);
 p.settings[SET_PROFILE]=0;p.settings[SET_RINGTONE]=3;
 assert(phone_incoming(&p,1)==FX_NONE && p.page==PHONE_INCOMING);phone_key(&p,KEY_BACK);
 for(int i=0;i<10000;i++) {
  phone_key(&p,(phone_key_t)(i%4));phone_advance(&p,50);
  assert(p.selected_app<9 && p.selected_contact<6 && p.selected_setting<SET_COUNT);
  for(unsigned n=0;n<SET_COUNT;n++)assert(p.settings[n]<setting_counts[n]);
  if(p.page==PHONE_SETTING_OPTIONS)assert(p.selected_option<setting_counts[p.selected_setting]);
 }
 puts("PASS: native settings, cancellation, sleep/silent modes, timed dialing, voice completion, busy fallback and navigation bounds");
}
