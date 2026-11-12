#include "storage.h"
#include "nvs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char committed[512],pending[512],committed_comm[1024],pending_comm[1024];
static size_t committed_comm_size,pending_comm_size;
static size_t committed_size,pending_size;
static unsigned commits;
static uint32_t committed_snap[3],pending_snap[3];static bool have_snap;
static uint32_t committed_egg,pending_egg;
static int fail_init,fail_write,legacy;
esp_err_t nvs_flash_init(void){return fail_init;}
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *h){(void)name;(void)mode;*h=1;return 0;}
void nvs_close(nvs_handle_t h){(void)h;}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *n){
 (void)h;
 if(!strcmp(key,"snap_v1")){if(!have_snap || *n<sizeof(committed_snap))return 1;memcpy(out,committed_snap,sizeof(committed_snap));*n=sizeof(committed_snap);return 0;}
 if(!strcmp(key,"comm_v1")){if(!committed_comm_size || *n<committed_comm_size)return 1;memcpy(out,committed_comm,committed_comm_size);*n=committed_comm_size;return 0;}
 if(!strcmp(key,"egg_v1")){if(*n<sizeof(committed_egg))return 1;memcpy(out,&committed_egg,sizeof(committed_egg));*n=sizeof(committed_egg);return 0;}
 if(legacy==3 && !strcmp(key,"state_v3")){struct {uint32_t version,revision,settings[SET_COUNT];world_t world;uint64_t reads[3][2];uint32_t photos[3],missed;} old={.version=3,.revision=story_revision(),.settings={2,0,2,3,1,4,6},.world={.role=2,.active={0,0,1u<<20},.rng=42,.wait_ms=90000},.reads={{0},{0},{UINT64_C(1)<<41,0}},.photos={0,0,4},.missed=1};if(*n<sizeof(old))return 1;memcpy(out,&old,sizeof(old));*n=sizeof(old);return 0;}
 if(legacy && !strcmp(key,"settings_v1")){struct {uint32_t version,settings[5];} old={1,{2,0,2,3,1}};if(*n<sizeof(old))return 1;memcpy(out,&old,sizeof(old));*n=sizeof(old);return 0;}
 if(legacy && !strcmp(key,"world_v1")){struct {uint32_t version;world_t world;} old={.version=1,.world={.role=2,.active={0,0,1u<<4},.done={0,1u<<2,0},.rng=42,.wait_ms=90000}};if(*n<sizeof(old))return 1;memcpy(out,&old,sizeof(old));*n=sizeof(old);return 0;}
 if(strcmp(key,"state_v4") || !committed_size || *n<committed_size)return 1;
 memcpy(out,committed,committed_size);*n=committed_size;return 0;
}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t n){
 (void)h;if(fail_write)return 1;if(!strcmp(key,"snap_v1")){assert(n==sizeof(pending_snap));memcpy(pending_snap,data,n);return 0;}if(!strcmp(key,"comm_v1")){assert(n<=sizeof(pending_comm));memcpy(pending_comm,data,n);pending_comm_size=n;return 0;}if(!strcmp(key,"egg_v1")){assert(n==sizeof(pending_egg));memcpy(&pending_egg,data,n);return 0;}assert(!strcmp(key,"state_v4"));
 assert(n<=sizeof(pending));memcpy(pending,data,n);pending_size=n;return 0;
}
esp_err_t nvs_commit(nvs_handle_t h){(void)h;memcpy(committed_snap,pending_snap,sizeof(committed_snap));have_snap=true;memcpy(committed_comm,pending_comm,pending_comm_size);committed_comm_size=pending_comm_size;committed_egg=pending_egg;memcpy(committed,pending,pending_size);committed_size=pending_size;commits++;return 0;}
int main(void){
 phone_t a,b;phone_init(&a);phone_storage_load(&a);
 a.snap_bits[0]=7;
 a.comm.log_count=1;a.comm.logs[0]=(comm_log_t){.contact=1,.role=0,.status=COMM_RECEIVED,.minute=500,.seconds=12};a.comm.message_count=1;a.comm.messages[0]=(comm_message_t){.contact=14,.role=0,.kind=FOLLOW_CALL,.task=1,.read=1};a.comm.task_contact[1]=14;a.comm.dial_serial[2]=3;a.comm.revision++;
 a.comm.message_count=2;a.comm.messages[1]=(comm_message_t){.contact=PHONE_TAXI_CONTACT,.role=1,.kind=FOLLOW_TAXI,.task=-1};
 a.egg_seen=5;a.settings[SET_BRIGHTNESS]=2;a.settings[SET_VOLUME]=7;a.world.role=1;
 a.world.active[1]=1u<<10;a.read_bits[1][0]=UINT64_C(1)<<40;a.photo_bits[1]=5;a.missed_calls=UINT64_C(1)<<38;sim_reward(&a.sim,1,10);sim_buy(&a.sim,1,0);
 assert(phone_storage_save(&a));assert(commits==1);
 phone_init(&b);phone_storage_load(&b);
 assert(b.egg_seen==5 && b.snap_bits[0]==7);
 assert(b.comm.log_count==1 && b.comm.logs[0].seconds==12 && b.comm.message_count==2 && b.comm.messages[0].read && b.comm.task_contact[1]==14 && b.comm.dial_serial[2]==3);
 assert(!strcmp(phone_comm_item(&b,0)->title,"Cab dispatched"));
 assert(b.world.role==1 && b.world.active[1]==(1u<<10));
 assert(b.settings[SET_BRIGHTNESS]==2 && b.settings[SET_VOLUME]==7);
 assert(b.read_bits[1][0]==(UINT64_C(1)<<40) && b.photo_bits[1]==5 && b.missed_calls==(UINT64_C(1)<<38) && b.sim.balance[1]==10500 && b.sim.count[1]==2);
 assert(phone_storage_save(&b));assert(commits==1);
 uint32_t old_ring=13;memcpy(committed+8+SET_RINGTONE*4,&old_ring,sizeof(old_ring));phone_init(&b);phone_storage_load(&b);assert(b.settings[SET_RINGTONE]==7 && b.comm.log_count==1);
 fail_write=1;b.settings[SET_BRIGHTNESS]=3;assert(!phone_storage_save(&b));
 phone_init(&b);phone_storage_load(&b);assert(b.settings[SET_BRIGHTNESS]==2);
 fail_write=0;committed[0]=99;phone_init(&b);phone_storage_load(&b);
 assert(b.world.role==0 && b.settings[SET_BRIGHTNESS]==6 && !b.read_bits[1][0]);
 legacy=3;phone_init(&b);phone_storage_load(&b);assert(b.world.role==2 && b.world.active[2]==(1u<<20) && b.read_bits[2][0]==(UINT64_C(1)<<41) && b.sim.balance[2]==10000);
 legacy=1;phone_init(&b);phone_storage_load(&b);
 assert(b.settings[SET_BACKGROUND]==2 && b.settings[SET_THEME]==3 && b.settings[SET_VOLUME]==4 && b.settings[SET_BRIGHTNESS]==6);
 assert(b.world.role==2 && b.world.active[2]==(1u<<20) && b.world.done[1]==(1u<<10));
 committed_comm[0]=99;phone_init(&b);phone_storage_load(&b);assert(b.comm.version==1 && b.comm.log_count==0);
 fail_init=1;phone_storage_load(&b);assert(!phone_storage_save(&b));
 puts("PASS: NVS snapshot round trip, 64-bit read state, capture state, dedupe, failed write and corrupt-version fallback");
}
