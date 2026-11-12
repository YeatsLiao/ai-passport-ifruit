#include "phone.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void options(phone_t *p,unsigned setting) {
 p->page=PHONE_SETTINGS;p->selected_setting=setting;
 assert(phone_key(p,KEY_OK)==FX_ACCEPT);assert(p->page==PHONE_SETTING_OPTIONS);
}
static void communication_checks(void){
 phone_t p;phone_init(&p);assert(phone_comm_history_count(&p)==0);p.page=PHONE_CALL_HISTORY;phone_key(&p,KEY_BACK);assert(p.page==PHONE_SETTINGS);p.page=PHONE_HOME;
 int scene=call_scene_default_incoming(0);phone_incoming_scene(&p,scene);assert(phone_comm_history_count(&p)==1 && phone_comm_history(&p,0)->status==COMM_MISSED);
 phone_key(&p,KEY_BACK);assert(phone_comm_history(&p,0)->status==COMM_REJECTED);phone_advance(&p,4000);assert(p.pending_count==0);
 p.page=PHONE_CALL_HISTORY;p.selected_history=0;phone_key(&p,KEY_OK);if(p.page==PHONE_CONTACT_ACTIONS)phone_key(&p,KEY_OK);assert(p.page==PHONE_DIALING && p.return_page==PHONE_CALL_HISTORY);phone_key(&p,KEY_BACK);assert(p.page==PHONE_CALL_HISTORY);
 phone_init(&p);phone_incoming_scene(&p,scene);phone_key(&p,KEY_OK);assert(phone_comm_history(&p,0)->status==COMM_RECEIVED);p.call_seconds=9;phone_audio_finished(&p);assert(phone_comm_history(&p,0)->seconds==9 && p.pending_count==1);
 phone_advance(&p,3999);assert(phone_comm_message_count(&p)==0);phone_advance(&p,1);assert(phone_comm_message_count(&p)==1 && !phone_story_is_read(&p,STORY_TEXT,0));assert(!strcmp(phone_story_item(&p,STORY_TEXT,0)->source_key,"local:linked-scene"));phone_story_mark_read(&p,STORY_TEXT,0);assert(phone_story_is_read(&p,STORY_TEXT,0));
 p.page=PHONE_TASK_DETAIL;p.selected_task=0;phone_key(&p,KEY_OK);assert(world_count(&p.world,true)==1);p.page=PHONE_HOME;phone_advance(&p,4000);assert(phone_comm_message_count(&p)==2 && phone_comm_message(&p,0)->kind==FOLLOW_TASK && phone_comm_message(&p,0)->contact==14);
 phone_init(&p);p.page=PHONE_CONTACTS;p.selected_contact=(unsigned)phonebook_find(0,1);phone_key(&p,KEY_OK);phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);assert(p.call_outcome==0);phone_advance(&p,3000);assert(call_scene_get((unsigned)p.scene_id)->outcome==0);phone_audio_finished(&p);
 uint32_t balance=p.sim.balance[0];p.page=PHONE_CONTACTS;phone_key(&p,KEY_OK);phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);assert(p.call_outcome==2);phone_advance(&p,3000);assert(call_scene_get((unsigned)p.scene_id)->outcome==2);phone_audio_finished(&p);assert(p.sim.balance[0]==balance);
 p.page=PHONE_CONTACTS;phone_advance(&p,45001);phone_key(&p,KEY_OK);phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);assert(p.call_outcome==1);phone_advance(&p,3000);assert(call_scene_get((unsigned)p.scene_id)->outcome==1);phone_audio_finished(&p);assert(p.sim.balance[0]==balance);
 for(unsigned r=0;r<3;r++)for(unsigned i=0;i<phonebook_count(r);i++){const phone_contact_t *c=phonebook_contact(r,i);if(c->outgoing_scene>=0)for(unsigned v=0;v<3;v++)assert(call_scene_outgoing(r,c->native_id,v)>=0);}
 for(unsigned i=0;i<20;i++){p.page=PHONE_HOME;phone_incoming_scene(&p,scene);phone_key(&p,KEY_BACK);}assert(p.comm.log_count==COMM_LIMIT);
 p.world.role=2;assert(phone_comm_history_count(&p)==0);
 phone_init(&p);p.world.wait_ms=180000;p.life_wait[0]=1;phone_advance(&p,1);assert(p.page==PHONE_INCOMING && !strcmp(call_scene_get((unsigned)p.scene_id)->id,"mifl_prem"));
 phone_init(&p);p.world.wait_ms=180000;p.settings[SET_PROFILE]=1;p.life_wait[0]=1;phone_advance(&p,5000);assert(p.page==PHONE_HOME && p.comm.log_count==0);
 phone_comm_queue(&p,FOLLOW_TASK,255,0);assert(phone_advance(&p,4000)==FX_NONE && p.comm.message_count==1);
 phone_init(&p);p.world.wait_ms=180000;for(unsigned i=0;i<13;i++){phone_comm_queue(&p,FOLLOW_TASK,255,0);phone_advance(&p,4000);}assert(p.comm.message_count==COMM_LIMIT);
 uint32_t old[SET_COUNT]={0};old[SET_RINGTONE]=13;assert(phone_restore_settings(&p,old) && p.settings[SET_RINGTONE]==7);
 printf("PASS: recent calls/callback, delayed linked SMS/read state, role cooldown, three original outgoing outcomes, ring-setting migration\n");
}
int main(void) {
 communication_checks();
 phone_t roles;phone_init(&roles);roles.page=PHONE_SETTINGS;roles.selected_setting=SET_COUNT-1;phone_key(&roles,KEY_DOWN);assert(roles.selected_setting==SET_COUNT);
 phone_key(&roles,KEY_OK);assert(roles.page==PHONE_ROLE && roles.selected_role==roles.world.role);
 for(unsigned i=0;i<3;i++){roles.page=PHONE_SETTINGS;roles.selected_setting=SET_COUNT;phone_key(&roles,KEY_OK);roles.selected_role=i;phone_key(&roles,KEY_OK);assert(roles.page==PHONE_HOME && roles.world.role==i && roles.selected_contact==0);}

 phone_t dial;phone_init(&dial);phone_key(&dial,KEY_OK);phone_key(&dial,KEY_DIAL_PAD);
 assert(dial.page==PHONE_NUMBER_LIST && dial.selected_number==0);
 int taxi=phone_number_list_contact(&dial,0);
 assert(taxi>=0 && !strcmp(phonebook_contact(0,(unsigned)taxi)->number,"3235555555"));
 assert(phone_key(&dial,KEY_OK)==FX_DIAL && dial.page==PHONE_DIALING);
 assert(phone_advance(&dial,3000)==FX_VOICE && dial.page==PHONE_CALL);
 assert(phone_audio_finished(&dial)==FX_ACCEPT);
 assert(dial.page==PHONE_DIAL_RESULT && dial.return_page==PHONE_NUMBER_LIST);
 phone_key(&dial,KEY_OK);assert(dial.page==PHONE_NUMBER_LIST);
 phone_advance(&dial,4000);
 assert(!strcmp(phone_comm_item(&dial,0)->title,"Cab dispatched"));
 phone_key(&dial,KEY_DOWN);phone_key(&dial,KEY_OK);assert(dial.page==PHONE_NUMBERPAD);
 assert(phone_key(&dial,KEY_OK)==FX_ACCEPT && !strcmp(dial.dial_number,"1"));
 phone_key(&dial,KEY_UP);assert(dial.selected_digit==PHONE_KEYPAD_CALL);
 phone_key(&dial,KEY_UP);phone_key(&dial,KEY_UP);assert(dial.selected_digit==PHONE_KEYPAD_ERASE);
 phone_key(&dial,KEY_OK);assert(!*dial.dial_number);
 strcpy(dial.dial_number,"3465550118");dial.selected_digit=PHONE_KEYPAD_CALL;
 assert(phone_key(&dial,KEY_OK)==FX_DIAL && phone_current_contact(&dial)->native_id==17);
 assert(phone_advance(&dial,3000)==FX_VOICE);
 phone_audio_finished(&dial);assert(dial.page==PHONE_NUMBERPAD);
 strcpy(dial.dial_number,"19993673767");
 phone_key(&dial,KEY_OK);assert(phone_advance(&dial,3000)==FX_ACCEPT && dial.black_phone_roles==1);
 phone_key(&dial,KEY_OK);phone_key(&dial,KEY_OK);
 assert(phone_advance(&dial,3000)==FX_ACCEPT && dial.black_phone_roles==0);
 phone_key(&dial,KEY_OK);strcpy(dial.dial_number,"999");phone_key(&dial,KEY_OK);
 assert(phone_advance(&dial,3000)==FX_STOP && dial.page==PHONE_BUSY);
 phone_advance(&dial,1500);assert(dial.page==PHONE_NUMBERPAD);
 phone_key(&dial,KEY_BACK);assert(dial.page==PHONE_NUMBER_LIST);
 phone_key(&dial,KEY_BACK);assert(dial.page==PHONE_CONTACTS);
 assert(phone_key(&dial,KEY_DIAL_PAD)==FX_ACCEPT && dial.page==PHONE_NUMBER_LIST);
 unsigned story_total=0;
 for(unsigned i=0;i<call_scene_count();i++)story_total+=call_scene_get(i)->incoming;
 assert(story_total==39);
 for(unsigned role=0;role<3;role++){
  phone_t archive;phone_init(&archive);archive.world.role=role;archive.selected_app=8;
  uint32_t missions=archive.world.active[role];
  assert(phone_key(&archive,KEY_OK)==FX_ACCEPT && archive.page==PHONE_STORY_CALLS);
  unsigned count=phone_story_call_count(role);
  assert(count>0 && phone_story_call_index(role,count)<0);
  for(unsigned i=0;i<count;i++){
   int index=phone_story_call_index(role,i);assert(index>=0);
   const call_scene_t *s=call_scene_get((unsigned)index);
   assert(s->incoming && s->title[0] && (s->role_mask&(1u<<role)));
   assert(phone_key(&archive,KEY_OK)==FX_RING && archive.page==PHONE_INCOMING && archive.scene_id==index);
   assert(phone_key(&archive,KEY_OK)==FX_SCENE && archive.page==PHONE_CALL);
   assert(phone_audio_finished(&archive)==FX_STOP && archive.page==PHONE_STORY_CALLS && archive.selected_story_call==i);
   assert(phone_key(&archive,KEY_DOWN)==FX_NAV);
  }
  assert(archive.selected_story_call==0);
  assert(phone_key(&archive,KEY_OK)==FX_RING && phone_advance(&archive,20000)==FX_STOP);
  assert(archive.page==PHONE_STORY_CALLS);
  archive.settings[SET_PROFILE]=1;
  assert(phone_key(&archive,KEY_OK)==FX_NONE && archive.page==PHONE_INCOMING);
  assert(phone_key(&archive,KEY_BACK)==FX_STOP && archive.page==PHONE_STORY_CALLS);
  assert(!archive.missed_calls && !archive.pending_count && !archive.comm.revision && archive.world.active[role]==missions);
 }
 phone_t progress;phone_init(&progress);progress.page=PHONE_PROGRESS;assert(phone_key(&progress,KEY_DOWN)==FX_NONE);assert(phone_key(&progress,KEY_UP)==FX_NONE);phone_key(&progress,KEY_OK);assert(progress.page==PHONE_CHECKLIST);
 phone_t controls;phone_init(&controls);
 controls.page=PHONE_VIEWFINDER;phone_key(&controls,KEY_DOWN);assert(controls.selected_photo==1);phone_key(&controls,KEY_UP);assert(controls.selected_photo==0);
 phone_key(&controls,KEY_OK);assert(controls.page==PHONE_PHOTO && !controls.snap_bits[0]);phone_key(&controls,KEY_BACK);assert(controls.page==PHONE_VIEWFINDER);
 phone_key(&controls,KEY_OK);phone_key(&controls,KEY_OK);assert(controls.page==PHONE_GALLERY && controls.snap_bits[0]==1 && controls.settings[SET_BACKGROUND]==0);phone_key(&controls,KEY_OK);assert(controls.photo_from_gallery);phone_key(&controls,KEY_BACK);assert(controls.page==PHONE_GALLERY);
 options(&controls,SET_VOLUME);controls.selected_option=10;assert(phone_key(&controls,KEY_DOWN)==FX_NONE && controls.selected_option==10);controls.selected_option=0;assert(phone_key(&controls,KEY_UP)==FX_NONE && controls.selected_option==0);
 phone_key(&controls,KEY_OK);assert(controls.page==PHONE_SETTINGS);
 controls.page=PHONE_READER;controls.reader_kind=STORY_TEXT;controls.reader_limit=2;
 phone_key(&controls,KEY_DOWN);assert(controls.reader_scroll==1 && controls.selected_story[1]==0);phone_key(&controls,KEY_DOWN);assert(controls.reader_scroll==2);phone_key(&controls,KEY_DOWN);assert(controls.reader_scroll==0 && controls.selected_story[1]==1 && (controls.read_bits[0][1]&2));phone_key(&controls,KEY_UP);assert(controls.selected_story[1]==0);assert(phone_key(&controls,KEY_UP)==FX_NONE);phone_key(&controls,KEY_OK);assert(controls.page==PHONE_MESSAGES);
 phone_t grid;phone_init(&grid);
 unsigned count=9;
 for(unsigned start=0;start<count;start++)for(unsigned k=0;k<4;k++){
  phone_key_t keys[4]={KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT};grid.selected_app=start;
  unsigned expected=(start+count+((k==0||k==2)?-1:1))%count;
  phone_key(&grid,keys[k]);assert(grid.selected_app==expected);
 }
 phone_advance(&grid,1999);assert(grid.clock_minutes==581);phone_advance(&grid,1);assert(grid.clock_minutes==582);

 phone_t egg;phone_init(&egg);unsigned texts=phone_story_count(&egg,STORY_TEXT);
 egg.page=PHONE_INTERNET;egg.selected_story[STORY_WEB]=phone_story_count(&egg,STORY_WEB)-1;
 assert(!strcmp(phone_story_item(&egg,STORY_WEB,egg.selected_story[STORY_WEB])->source_key,"local:vice-city-easter-egg"));
 assert(phone_key(&egg,KEY_OK)==FX_ACCEPT && egg.page==PHONE_EASTER_EGG);
 assert(phone_key(&egg,KEY_DOWN)==FX_NONE && !egg.egg_seen);
 assert(phone_key(&egg,KEY_BACK)==FX_TEXT && egg.page==PHONE_INTERNET && egg.egg_seen==1);
 assert(phone_story_count(&egg,STORY_TEXT)==texts+1);
 assert(!strcmp(phone_story_item(&egg,STORY_TEXT,texts)->title,"Wrong city. Right number."));
 phone_key(&egg,KEY_OK);assert(phone_key(&egg,KEY_OK)==FX_BACK && phone_story_count(&egg,STORY_TEXT)==texts+1);
 egg.world.role=1;egg.selected_story[STORY_WEB]=phone_story_count(&egg,STORY_WEB)-1;phone_key(&egg,KEY_OK);assert(phone_key(&egg,KEY_BACK)==FX_TEXT && egg.egg_seen==3);

 assert(setting_counts[SET_RINGTONE]==8);
 for(unsigned r=0;r<3;r++){assert(!strcmp(phone_ring_file(7,r),phone_ring_file(r,0)));assert(phone_ring_file(3,r)==NULL);}
 assert(phone_ring_file(14,0)==NULL && phone_ring_file(7,3)==NULL);

 sim_t account;sim_init(&account);assert(sim_valid(&account));assert(sim_reward(&account,0,1));assert(account.balance[0]==11000);assert(!sim_reward(&account,0,1));assert(!sim_reward(&account,1,1));assert(sim_buy(&account,0,0));assert(!sim_buy(&account,0,0));assert(account.balance[0]==10500 && account.balance[1]==10000);account.balance[1]=10;assert(!sim_buy(&account,1,1));
 for(unsigned role=0;role<3;role++){
  assert(phonebook_count(role)>0);for(unsigned c=0;c<phonebook_count(role);c++){
   const phone_contact_t *contact=phonebook_contact(role,c);assert(contact && contact->name && contact->portrait_key);
   assert(phonebook_find(role,contact->native_id)==(int)c);
  }
  assert(phonebook_contact(role,phonebook_count(role))==NULL);
  phone_t incoming;phone_init(&incoming);incoming.world.role=role;
  for(unsigned i=0;i<call_scene_count();i++){
   const call_scene_t *scene=call_scene_get(i);phone_page_t before=incoming.page;
   if(!scene->incoming || !(scene->role_mask&(1u<<role))){assert(phone_incoming_scene(&incoming,(int)i)==FX_NONE);assert(incoming.page==before);}
   else if(phonebook_find(role,scene->contact_native_id)>=0){assert(phone_incoming_scene(&incoming,(int)i)==FX_RING);assert(incoming.scene_id==(int)i);assert(phone_key(&incoming,KEY_OK)==FX_SCENE);assert(phone_audio_finished(&incoming)==FX_STOP);}
  }
 }
 assert(call_scene_get(call_scene_count())==NULL);
 unsigned outgoing_count=0;
 for(unsigned role=0;role<3;role++)for(unsigned c=0;c<phonebook_count(role);c++){
  const phone_contact_t *contact=phonebook_contact(role,c);if(contact->outgoing_scene<0)continue;
  phone_t dial;phone_init(&dial);dial.world.role=role;dial.page=PHONE_CONTACTS;dial.selected_contact=c;
  assert(phone_key(&dial,KEY_OK)==FX_ACCEPT && dial.page==PHONE_CONTACT_ACTIONS);
  assert(phone_key(&dial,KEY_BACK)==FX_BACK && dial.page==PHONE_CONTACTS);
  phone_key(&dial,KEY_OK);phone_key(&dial,KEY_DOWN);assert(phone_key(&dial,KEY_OK)==FX_DIAL);
  assert(phone_advance(&dial,3000)==FX_SCENE && dial.page==PHONE_CALL);
  assert(dial.scene_id==contact->outgoing_scene);
  assert(phone_audio_finished(&dial)==FX_STOP && dial.sim.count[role]==1 && dial.sim.balance[role]==10000);
  phone_key(&dial,KEY_OK);assert(phone_key(&dial,KEY_OK)==FX_DIAL);
  assert(phone_advance(&dial,3000)!=FX_SCENE);outgoing_count++;
 }
 assert(outgoing_count==12);
 phone_t checkout;phone_init(&checkout);checkout.page=PHONE_INTERNET;checkout.selected_story[STORY_WEB]=1;
 phone_key(&checkout,KEY_OK);assert(checkout.page==PHONE_PURCHASE_CONFIRM);
 phone_key(&checkout,KEY_BACK);assert(checkout.sim.balance[0]==10000);
 phone_key(&checkout,KEY_OK);assert(phone_key(&checkout,KEY_OK)==FX_TEXT && checkout.sim.balance[0]==9500);
 phone_key(&checkout,KEY_BACK);phone_key(&checkout,KEY_OK);phone_key(&checkout,KEY_OK);assert(checkout.save_status==-1 && checkout.sim.balance[0]==9500);

 phone_t hardware;phone_init(&hardware);options(&hardware,SET_VOLUME);
 assert(phone_key(&hardware,KEY_DOWN)==FX_HW_PREVIEW);assert(hardware.settings[SET_VOLUME]==4 && hardware.selected_option==5);
 phone_key(&hardware,KEY_BACK);assert(hardware.settings[SET_VOLUME]==4);
 options(&hardware,SET_BRIGHTNESS);hardware.selected_option=0;phone_key(&hardware,KEY_OK);assert(hardware.settings[SET_BRIGHTNESS]==0);
 assert(setting_counts[SET_BRIGHTNESS]==10 && setting_counts[SET_VOLUME]==11);
 phone_t missed;phone_init(&missed);unsigned prior=phone_story_count(&missed,STORY_TEXT);
 assert(phone_demo_call(&missed)==FX_RING);phone_key(&missed,KEY_BACK);
 assert(phone_story_count(&missed,STORY_TEXT)==prior+1);assert(!strcmp(phone_story_item(&missed,STORY_TEXT,prior-2)->source_key,"YB_JIMTXT"));
 phone_init(&missed);phone_demo_call(&missed);assert(phone_advance(&missed,20000)==FX_STOP);assert(missed.missed_calls==1);
 phone_init(&missed);phone_demo_call(&missed);phone_key(&missed,KEY_OK);phone_audio_finished(&missed);assert(!missed.missed_calls);
 world_t w;world_init(&w);assert(world_valid(&w));
 assert(!world_step(&w,UINT32_MAX,false) && w.wait_ms==45000);
 assert(world_step(&w,45000,true));int task=world_task(&w,0);assert(task>=0 && task<10);
 assert(!world_complete(&w,10));assert(world_complete(&w,(unsigned)task));assert(!world_complete(&w,(unsigned)task));
 assert(world_step(&w,UINT32_MAX,true));assert(world_task(&w,0)!=task);
 w.role=1;assert(world_count(&w,false)==0 && world_count(&w,true)==0);
 assert(world_step(&w,UINT32_MAX,true));assert(world_task(&w,0)>=10 && world_task(&w,0)<20);
 w.active[1]|=1;assert(!world_valid(&w));
 phone_t event_phone;phone_init(&event_phone);event_phone.page=PHONE_INCOMING;
 phone_advance(&event_phone,UINT32_MAX);assert(!world_count(&event_phone.world,false));
 event_phone.page=PHONE_HOME;event_phone.settings[SET_PROFILE]=1;
 phone_advance(&event_phone,UINT32_MAX);assert(!world_count(&event_phone.world,false));
 event_phone.settings[SET_PROFILE]=0;assert(phone_advance(&event_phone,45000)==FX_RING);assert(event_phone.story_call);
 assert(phone_key(&event_phone,KEY_OK)==FX_SCENE);assert(phone_audio_finished(&event_phone)==FX_STOP);
 event_phone.selected_app=2;phone_key(&event_phone,KEY_OK);phone_key(&event_phone,KEY_OK);
 assert(event_phone.page==PHONE_OPEN_MISSIONS);phone_key(&event_phone,KEY_OK);
 assert(event_phone.page==PHONE_TASK_DETAIL);phone_key(&event_phone,KEY_OK);
 assert(world_count(&event_phone.world,true)==1 && !world_count(&event_phone.world,false));
 phone_key(&event_phone,KEY_BACK);phone_key(&event_phone,KEY_BACK);
 assert(event_phone.page==PHONE_HOME && phone_key(&event_phone,KEY_DIAL_PAD)==FX_NONE);
 assert(phone_key(&event_phone,KEY_BACK)==FX_ACCEPT && event_phone.page==PHONE_ROLE);
 assert(event_phone.selected_role==event_phone.world.role);
 phone_key(&event_phone,KEY_DOWN);phone_key(&event_phone,KEY_OK);assert(event_phone.world.role==1);
 phone_t rich;phone_init(&rich);rich.selected_app=3;
 assert(phone_key(&rich,KEY_OK)==FX_SAVE);assert(rich.page==PHONE_SAVE);
 phone_save_result(&rich,false);assert(rich.save_status==-1 && rich.page==PHONE_SAVE_RESULT);
 phone_key(&rich,KEY_BACK);rich.selected_app=6;phone_key(&rich,KEY_OK);
 assert(rich.page==PHONE_CAMERA);phone_key(&rich,KEY_OK);assert(rich.page==PHONE_VIEWFINDER);assert(phone_key(&rich,KEY_OK)==FX_SHUTTER);
 assert(rich.page==PHONE_PHOTO && rich.snap_bits[0]==0);phone_key(&rich,KEY_OK);assert(rich.page==PHONE_GALLERY && rich.snap_bits[0]==1);
 phone_key(&rich,KEY_OK);assert(rich.page==PHONE_PHOTO && rich.photo_from_gallery);phone_key(&rich,KEY_BACK);assert(rich.page==PHONE_GALLERY);
 phone_key(&rich,KEY_BACK);assert(rich.page==PHONE_CAMERA);phone_key(&rich,KEY_BACK);
 rich.selected_app=0;phone_key(&rich,KEY_OK);if(story_count(0,STORY_EMAIL)){
 phone_key(&rich,KEY_OK);assert(rich.page==PHONE_READER && (rich.read_bits[0][0]&1));
 phone_key(&rich,KEY_DOWN);phone_key(&rich,KEY_BACK);assert(rich.page==PHONE_EMAIL);}
 phone_t restored;phone_init(&restored);
 uint32_t prefs[SET_COUNT]={14,1,3,6,0};assert(phone_restore_settings(&restored,prefs));
 assert(restored.settings[SET_BACKGROUND]==0 && restored.settings[SET_THEME]==6);
 prefs[SET_THEME]=7;prefs[SET_BACKGROUND]=0;
 assert(!phone_restore_settings(&restored,prefs));assert(restored.settings[SET_BACKGROUND]==0);
 prefs[SET_THEME]=UINT32_MAX;assert(!phone_restore_settings(&restored,prefs));
 phone_t p;phone_init(&p);assert(p.page==PHONE_HOME && p.selected_app==4);
 assert(phone_key(&p,KEY_OK)==FX_ACCEPT);assert(p.page==PHONE_CONTACTS);
 int lester=phonebook_find(0,12);assert(lester>=0);p.selected_contact=(unsigned)lester;
 assert(phone_key(&p,KEY_OK)==FX_DIAL);assert(p.page==PHONE_DIALING);
 assert(phone_advance(&p,2999)==FX_NONE);assert(p.page==PHONE_DIALING);
 assert(phone_incoming(&p,1)==FX_NONE);
 phone_fx_t connected=phone_advance(&p,1);assert(connected==FX_VOICE || connected==FX_SCENE);assert(p.page==PHONE_CALL);
 phone_advance(&p,999);assert(p.call_seconds==0);phone_advance(&p,1);assert(p.call_seconds==1);
 assert(phone_key(&p,KEY_DOWN)==FX_NONE);
 assert(phone_audio_finished(&p)==FX_STOP);assert(p.page==PHONE_CONTACTS);
 assert(phone_audio_finished(&p)==FX_NONE);
 /* A cancelled old completion cannot return a later dialing screen. */
 phone_key(&p,KEY_OK);assert(phone_key(&p,KEY_BACK)==FX_STOP);
 phone_key(&p,KEY_OK);assert(phone_audio_finished(&p)==FX_NONE);assert(p.page==PHONE_DIALING);
 phone_key(&p,KEY_BACK);
 /* Unmapped voices have an explicit local busy outcome, no arbitrary recording. */
 int unavailable=-1;for(unsigned c=0;c<phonebook_count(0);c++){const phone_contact_t *item=phonebook_contact(0,c);if(item->native_id!=PHONE_TAXI_CONTACT && item->outgoing_scene<0 && (!item->answerphone_file || !*item->answerphone_file)){unavailable=(int)c;break;}}
 assert(unavailable>=0);p.selected_contact=(unsigned)unavailable;phone_key(&p,KEY_OK);
 assert(phone_advance(&p,UINT32_MAX)==FX_STOP);assert(p.page==PHONE_BUSY);
 assert(phone_advance(&p,1499)==FX_NONE);assert(phone_advance(&p,1)==FX_STOP);assert(p.page==PHONE_CONTACTS);
 assert(phone_incoming(&p,phonebook_count(p.world.role))==FX_NONE);assert(phone_incoming(&p,1)==FX_RING);
 assert(phone_key(&p,KEY_DOWN)==FX_NONE);assert(phone_key(&p,KEY_BACK)==FX_STOP);
 assert(p.page==PHONE_CONTACTS);phone_incoming(&p,1);assert(phone_key(&p,KEY_OK)==FX_ACCEPT);
 assert(p.call_seconds==0 && p.page==PHONE_CALL);assert(phone_audio_finished(&p)==FX_NONE);
 phone_key(&p,KEY_BACK);phone_key(&p,KEY_BACK);assert(p.page==PHONE_HOME);
 phone_init(&p);p.selected_app=0;phone_key(&p,KEY_UP);assert(p.selected_app==8);
 phone_key(&p,KEY_OK);assert(p.page==PHONE_STORY_CALLS);phone_key(&p,KEY_BACK);phone_key(&p,KEY_RIGHT);assert(p.selected_app==0);phone_key(&p,KEY_RIGHT);assert(p.selected_app==1);
 p.selected_app=1;phone_key(&p,KEY_OK);assert(p.page==PHONE_MESSAGES);
 assert(phone_key(&p,KEY_BACK)==FX_BACK);assert(p.page==PHONE_HOME);
 /* Selection is separate from applied state; cancelling preserves appearance. */
 options(&p,SET_BACKGROUND);phone_key(&p,KEY_UP);assert(p.selected_option==setting_counts[SET_BACKGROUND]-1);
 phone_key(&p,KEY_BACK);assert(p.settings[SET_BACKGROUND]==0 && p.page==PHONE_SETTINGS);
 options(&p,SET_BACKGROUND);phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);assert(p.settings[SET_BACKGROUND]==1);
 phone_key(&p,KEY_BACK);options(&p,SET_THEME);phone_key(&p,KEY_UP);phone_key(&p,KEY_OK);assert(p.settings[SET_THEME]==6);
 phone_key(&p,KEY_BACK);options(&p,SET_PROFILE);phone_key(&p,KEY_DOWN);phone_key(&p,KEY_OK);
 assert(phone_incoming(&p,1)==FX_NONE && p.page==PHONE_SETTINGS);
 p.settings[SET_PROFILE]=0;p.settings[SET_RINGTONE]=3;
 assert(phone_incoming(&p,1)==FX_NONE && p.page==PHONE_INCOMING);phone_key(&p,KEY_BACK);
 for(int i=0;i<10000;i++) {
  phone_key(&p,(phone_key_t)(i%4));phone_advance(&p,50);
  assert(p.selected_app<9 && p.selected_contact<phonebook_count(p.world.role) && p.selected_setting<SET_COUNT);
  for(unsigned n=0;n<SET_COUNT;n++)assert(p.settings[n]<setting_counts[n]);
  if(p.page==PHONE_SETTING_OPTIONS)assert(p.selected_option<setting_counts[p.selected_setting]);
 }
 puts("PASS: native settings, cancellation, sleep/silent modes, timed dialing, voice completion, busy fallback and navigation bounds");
}
