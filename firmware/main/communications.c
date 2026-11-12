#include "phone.h"
#include <string.h>
#include <stdio.h>
static const char *name(unsigned role,unsigned native){int i=phonebook_find(role,native);const phone_contact_t *c=i<0?NULL:phonebook_contact(role,(unsigned)i);return c?c->name:"Replay";}
unsigned phone_comm_history_count(const phone_t *p){unsigned n=0;for(unsigned i=0;i<p->comm.log_count;i++)n+=p->comm.logs[i].role==p->world.role;return n;}
const comm_log_t *phone_comm_history(const phone_t *p,unsigned row){for(unsigned i=0;i<p->comm.log_count;i++)if(p->comm.logs[i].role==p->world.role && row--==0)return &p->comm.logs[i];return NULL;}
unsigned phone_comm_message_count(const phone_t *p){unsigned n=0;for(unsigned i=0;i<p->comm.message_count;i++)n+=p->comm.messages[i].role==p->world.role;return n;}
comm_message_t *phone_comm_message(phone_t *p,unsigned row){for(unsigned n=p->comm.message_count;n>0;n--){unsigned i=n-1;if(p->comm.messages[i].role==p->world.role && row--==0)return &p->comm.messages[i];}return NULL;}
void phone_comm_begin(phone_t *p,bool incoming){
 const phone_contact_t *c=phone_current_contact(p);if(!c)return;
 if(p->comm.log_count<COMM_LIMIT)p->comm.log_count++;
 memmove(p->comm.logs+1,p->comm.logs,(p->comm.log_count-1)*sizeof(comm_log_t));
 p->comm.logs[0]=(comm_log_t){.contact=(uint16_t)c->native_id,.minute=(uint16_t)p->clock_minutes,.role=(uint8_t)p->world.role,.status=incoming?COMM_MISSED:COMM_DIALLED};
 p->comm.revision++;p->comm_active=true;p->call_seconds=0;p->life_wait[p->world.role]=120000+(p->world.rng^p->uptime_ms)%120001;
}
void phone_comm_answer(phone_t *p){if(p->comm_active){p->comm.logs[0].status=COMM_RECEIVED;p->comm.revision++;}}
void phone_comm_queue(phone_t *p,unsigned kind,unsigned contact,int task){
 if(p->pending_count==3){memmove(p->pending,p->pending+1,2*sizeof(comm_message_t));p->pending_count=2;}
 p->pending[p->pending_count++]=(comm_message_t){.role=(uint8_t)p->world.role,.contact=(uint16_t)contact,.kind=(uint8_t)kind,.task=(int16_t)task};
 if(p->pending_count==1)p->notify_wait=4000;
}
void phone_comm_end(phone_t *p,unsigned reason){
 if(!p->comm_active)return;
 comm_log_t *log=&p->comm.logs[0];log->seconds=p->call_seconds>65535?65535:(uint16_t)p->call_seconds;
 if(reason==2 && p->page==PHONE_INCOMING)log->status=COMM_REJECTED;
 if(reason==1 || (reason==2 && p->page==PHONE_INCOMING))phone_comm_queue(p,FOLLOW_MISSED,log->contact,-1);
 else if(reason==0 && p->story_call){
  const call_scene_t *s=p->scene_id<0?NULL:call_scene_get((unsigned)p->scene_id);
  unsigned kind=p->outgoing?(s&&s->outcome?FOLLOW_REFUSAL:FOLLOW_INVITE):FOLLOW_CALL;
  phone_comm_queue(p,kind,log->contact,s?s->mission_task:-1);
 }
 p->comm_active=false;p->comm.revision++;
}
const story_item_t *phone_comm_item(const phone_t *p,unsigned row){
 const comm_message_t *m=phone_comm_message((phone_t *)p,row);if(!m)return NULL;
 if(m->kind==FOLLOW_TAXI){
  static const story_item_t taxi={"Downtown Cab Co.","Cab dispatched","Your taxi is on its way.\n\n[Local dispatch]","local:taxi-dispatch"};
  return &taxi;
 }
 static char body[640];static story_item_t item;
 const char *text=m->kind==FOLLOW_INVITE?"See you soon.":m->kind==FOLLOW_REFUSAL?"Not now. Try calling again later.":"Keep your phone close. I'll be in touch.";
 if(m->kind==FOLLOW_TASK)snprintf(body,sizeof(body),"Done: %s\n\nYour replay progress is updated.\n\n[Local scene]",m->task>=0&&m->task<WORLD_TASKS?mission_names[m->task]:"Task");
 else if(m->task>=0&&m->task<WORLD_TASKS)snprintf(body,sizeof(body),"%s\n\nCheck List: %s\n\n[Local scene]",text,mission_names[m->task]);
 else snprintf(body,sizeof(body),"%s\n\n[Local scene]",text);
 item=(story_item_t){name(m->role,m->contact),m->kind==FOLLOW_TASK?"Replay completed":"Replay follow-up",body,"local:linked-scene"};return &item;
}
phone_fx_t phone_comm_step(phone_t *p,uint32_t elapsed){
 bool idle=p->page==PHONE_HOME;
 if(!idle && p->page!=PHONE_CONTACTS && p->page!=PHONE_CALL_HISTORY && p->page!=PHONE_MESSAGES && p->page!=PHONE_EMAIL && p->page!=PHONE_INTERNET && p->page!=PHONE_NUMBER_LIST && p->page!=PHONE_NUMBERPAD && p->page!=PHONE_DIAL_RESULT)return FX_NONE;
 if(p->pending_count){
  if(elapsed<p->notify_wait){p->notify_wait-=elapsed;return FX_NONE;}
  comm_message_t m=p->pending[0];memmove(p->pending,p->pending+1,(--p->pending_count)*sizeof(m));p->notify_wait=4000;
  if(m.kind!=FOLLOW_MISSED){
   if(p->comm.message_count==COMM_LIMIT){memmove(p->comm.messages,p->comm.messages+1,(COMM_LIMIT-1)*sizeof(m));p->comm.message_count--;}
   p->comm.messages[p->comm.message_count++]=m;p->comm.revision++;if(m.role==p->world.role)p->selected_story[STORY_TEXT]=0;
  }
  p->unread=m.role==p->world.role;return p->settings[SET_PROFILE] || m.role!=p->world.role?FX_NONE:FX_TEXT;
 }
 if(!idle)return FX_NONE;
 unsigned r=p->world.role;if(p->settings[SET_PROFILE])return FX_NONE;
 if(elapsed<p->life_wait[r]){p->life_wait[r]-=elapsed;return FX_NONE;}
 p->life_wait[r]=120000+(p->world.rng^p->uptime_ms)%120001;
 static const char *ids[3][2]={{"mifl_prem","mifl_tr1"},{"frfl_1lc","arfl_call_ct"},{"pafl_02","chfl_call1"}};
 const char *id=ids[r][p->comm.life_cursor[r]%2];
 for(unsigned i=0;i<call_scene_count();i++){const call_scene_t *s=call_scene_get(i);if(!strcmp(s->id,id)){p->comm.life_cursor[r]++;p->comm.revision++;return phone_incoming_scene(p,(int)i);}}
 return FX_NONE;
}
