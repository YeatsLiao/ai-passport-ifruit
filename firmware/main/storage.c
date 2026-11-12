#include "storage.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
typedef struct {
 uint32_t version,content_revision,settings[SET_COUNT];
 world_t world;
 uint64_t read_bits[3][2];
 uint32_t photo_bits[3];
 uint32_t missed_calls;
} snapshot_v3_t;
typedef struct {
 uint32_t version,content_revision,settings[SET_COUNT];
 world_t world;
 uint64_t read_bits[3][2];
 uint32_t photo_bits[3];
 uint64_t missed_calls;
 sim_t sim;
} snapshot_t;
static bool ready,have_saved;
static uint32_t saved_egg;
static snapshot_t saved;
static comm_state_t saved_comm;
static uint32_t saved_snap[3];
static bool have_snap_saved;
static bool have_comm_saved;
static bool valid_comm(const comm_state_t *c){
 if(c->version!=1 || c->log_count>COMM_LIMIT || c->message_count>COMM_LIMIT)return false;
 for(unsigned i=0;i<c->log_count;i++){const comm_log_t *l=&c->logs[i];if(l->role>=3 || l->status>COMM_REJECTED || l->minute>=1440 || phonebook_find(l->role,l->contact)<0)return false;}
 for(unsigned i=0;i<c->message_count;i++){const comm_message_t *m=&c->messages[i];if(m->role>=3 || m->kind<FOLLOW_CALL || m->kind>FOLLOW_TAXI || m->kind==FOLLOW_MISSED || m->read>1 || m->task < -1 || m->task>=WORLD_TASKS || (m->contact!=255 && phonebook_find(m->role,m->contact)<0))return false;}
 for(unsigned i=0;i<WORLD_TASKS;i++)if(c->task_contact[i]!=255 && c->task_contact[i]>64)return false;
 return true;
}
static bool valid(const snapshot_t *s){
 uint64_t incoming=0;for(unsigned i=0;i<call_scene_count() && i<64;i++)if(call_scene_get(i)->incoming)incoming|=UINT64_C(1)<<i;
 if(s->missed_calls&~incoming)return false;
 if(s->version!=4 || !sim_valid(&s->sim) || (call_scene_count()<64 && (s->missed_calls>>call_scene_count())) || !world_valid(&s->world))return false;
 for(unsigned i=0;i<SET_COUNT;i++)if(s->settings[i]>=setting_counts[i] && !((i==SET_RINGTONE && s->settings[i]<=13)||(i==SET_BACKGROUND && s->settings[i]<15)))return false;
 for(unsigned r=0;r<3;r++)if(s->photo_bits[r]&~UINT32_C(0x7fff))return false;
 return true;
}
static bool read_blob(nvs_handle_t h,const char *key,void *out,size_t expected){size_t n=expected;return nvs_get_blob(h,key,out,&n)==ESP_OK && n==expected;}
void phone_storage_load(phone_t *p){
 ready=false;have_saved=false;have_comm_saved=false;have_snap_saved=false;saved_egg=0;
 esp_err_t e=nvs_flash_init();
 if(e!=ESP_OK){ESP_LOGW("phone","Preferences unavailable: %s",esp_err_to_name(e));return;}
 ready=true;nvs_handle_t h;
 if(nvs_open("ifruit",NVS_READONLY,&h)!=ESP_OK)return;
 uint32_t snaps[3]={0};if(read_blob(h,"snap_v1",snaps,sizeof(snaps)) && !((snaps[0]|snaps[1]|snaps[2])&~((1u<<SNAP_SCENES)-1))){memcpy(p->snap_bits,snaps,sizeof(snaps));memcpy(saved_snap,snaps,sizeof(snaps));have_snap_saved=true;}
 comm_state_t comm={0};if(read_blob(h,"comm_v1",&comm,sizeof(comm)) && valid_comm(&comm)){p->comm=comm;saved_comm=comm;have_comm_saved=true;}
 uint32_t egg=0;if(read_blob(h,"egg_v1",&egg,sizeof(egg)) && !(egg&~7u)){p->egg_seen=egg;saved_egg=egg;}
 snapshot_t s={0};
 if(read_blob(h,"state_v4",&s,sizeof(s)) && valid(&s)){
  phone_restore_settings(p,s.settings);p->world=s.world;p->sim=s.sim;
  memcpy(p->photo_bits,s.photo_bits,sizeof(p->photo_bits));p->missed_calls=s.missed_calls;
  if(s.content_revision==story_revision())memcpy(p->read_bits,s.read_bits,sizeof(p->read_bits));
  saved=s;have_saved=true;nvs_close(h);return;
 }
 snapshot_v3_t legacy={0};
 if(read_blob(h,"state_v3",&legacy,sizeof(legacy)) && legacy.version==3){
  snapshot_t converted={.version=4};
  converted.content_revision=legacy.content_revision;memcpy(converted.settings,legacy.settings,sizeof(converted.settings));converted.world=legacy.world;memcpy(converted.read_bits,legacy.read_bits,sizeof(converted.read_bits));memcpy(converted.photo_bits,legacy.photo_bits,sizeof(converted.photo_bits));converted.missed_calls=legacy.missed_calls;sim_init(&converted.sim);
  if(valid(&converted)){phone_restore_settings(p,converted.settings);p->world=converted.world;p->sim=converted.sim;memcpy(p->photo_bits,converted.photo_bits,sizeof(p->photo_bits));p->missed_calls=converted.missed_calls;if(converted.content_revision==story_revision())memcpy(p->read_bits,converted.read_bits,sizeof(p->read_bits));nvs_close(h);return;}
 }
 /* Read old settings and migrate six-task world without erasing any NVS keys. */
 struct {uint32_t version,settings[5];} prefs={0};
 if(read_blob(h,"settings_v1",&prefs,sizeof(prefs)) && prefs.version==1){uint32_t values[SET_COUNT];for(unsigned i=0;i<SET_COUNT;i++)values[i]=p->settings[i];memcpy(values,prefs.settings,sizeof(prefs.settings));phone_restore_settings(p,values);}
 struct {uint32_t version;world_t world;} old={0};
 if(read_blob(h,"world_v1",&old,sizeof(old)) && old.version==1){
  world_t w=old.world;bool ok=w.role<3 && w.rng && w.wait_ms<=180000;
  for(unsigned r=0;r<3;r++){
   uint32_t mask=3u<<(r*2);
   if(((w.active[r]|w.done[r])&~mask) || (w.active[r]&w.done[r]))ok=false;
   w.active[r]=(w.active[r]>>(r*2))<<(r*WORLD_TASKS_PER_ROLE);
   w.done[r]=(w.done[r]>>(r*2))<<(r*WORLD_TASKS_PER_ROLE);
  }
  if(ok && world_valid(&w))p->world=w;
 }
 nvs_close(h);
}
bool phone_storage_save(const phone_t *p){
 if(!ready)return false;
 snapshot_t s={.version=4,.sim=p->sim,.content_revision=story_revision(),.world=p->world,.missed_calls=p->missed_calls};
 for(unsigned i=0;i<SET_COUNT;i++)s.settings[i]=p->settings[i];
 memcpy(s.read_bits,p->read_bits,sizeof(s.read_bits));memcpy(s.photo_bits,p->photo_bits,sizeof(s.photo_bits));
 if((p->snap_bits[0]|p->snap_bits[1]|p->snap_bits[2])&~((1u<<SNAP_SCENES)-1))return false;
 if(!valid_comm(&p->comm) || !valid(&s) || (p->egg_seen&~7u))return false;
 if(have_saved && have_snap_saved && !memcmp(p->snap_bits,saved_snap,sizeof(saved_snap)) && have_comm_saved && memcmp(&p->comm,&saved_comm,sizeof(saved_comm))==0 && saved_egg==p->egg_seen && memcmp(&s,&saved,sizeof(s))==0)return true;
 nvs_handle_t h;esp_err_t e=nvs_open("ifruit",NVS_READWRITE,&h);
 if(e!=ESP_OK)return false;
 e=nvs_set_blob(h,"state_v4",&s,sizeof(s));if(e==ESP_OK)e=nvs_set_blob(h,"egg_v1",&p->egg_seen,sizeof(p->egg_seen));if(e==ESP_OK)e=nvs_set_blob(h,"comm_v1",&p->comm,sizeof(p->comm));if(e==ESP_OK)e=nvs_set_blob(h,"snap_v1",p->snap_bits,sizeof(p->snap_bits));if(e==ESP_OK)e=nvs_commit(h);nvs_close(h);
 if(e==ESP_OK){saved=s;saved_comm=p->comm;memcpy(saved_snap,p->snap_bits,sizeof(saved_snap));have_snap_saved=true;have_comm_saved=true;saved_egg=p->egg_seen;have_saved=true;return true;}
 ESP_LOGW("phone","Save failed: %s",esp_err_to_name(e));return false;
}
