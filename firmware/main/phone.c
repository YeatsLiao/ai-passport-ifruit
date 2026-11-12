#include "phone.h"
#include <string.h>
#include <stdio.h>
const char *const phone_apps[9] = {"Email", "Texts", "Check List", "Quick Save", "Contacts", "Settings", "Snapmatic", "Internet", "Story Calls"};
static const char black_phone_number[]="19993673767";
void phone_init(phone_t *p) { memset(p, 0, sizeof(*p));p->scene_id=-1;p->clock_minutes=581;p->clock_day=2; p->comm.version=1;memset(p->comm.task_contact,255,sizeof(p->comm.task_contact));for(unsigned r=0;r<3;r++){p->life_wait[r]=120000;p->last_dial_contact[r]=UINT16_MAX;}world_init(&p->world);sim_init(&p->sim);p->page = PHONE_HOME; p->selected_app = 4; p->settings[SET_RINGTONE]=7;p->settings[SET_VIBRATE]=1;p->settings[SET_VOLUME]=4;p->settings[SET_BRIGHTNESS]=6; }
bool phone_restore_settings(phone_t *p,const uint32_t values[SET_COUNT]) {
    for(unsigned i=0;i<SET_COUNT;i++)if(values[i]>=setting_counts[i] && !((i==SET_RINGTONE && values[i]<=13)||(i==SET_BACKGROUND && values[i]<15)))return false;
    for(unsigned i=0;i<SET_COUNT;i++)p->settings[i]=i==SET_RINGTONE && values[i]>=7?7:i==SET_BACKGROUND && values[i]>=setting_counts[SET_BACKGROUND]?0:values[i];
    return true;
}
static phone_fx_t show_incoming(phone_t *p){
    p->return_page=p->page;p->page=PHONE_INCOMING;p->outgoing=false;p->story_call=false;p->scene_id=-1;p->phase_ms=0;
    return p->settings[SET_RINGTONE]==3 || p->settings[SET_PROFILE] ? FX_NONE : FX_RING;
}
phone_fx_t phone_incoming(phone_t *p, unsigned contact) {
    if (p->page == PHONE_CALL || p->page == PHONE_INCOMING || p->page == PHONE_DIALING || p->page == PHONE_BUSY || contact >= phonebook_count(p->world.role) || p->settings[SET_PROFILE]==1) return FX_NONE;
    p->selected_contact=contact;
    phone_fx_t fx=show_incoming(p);
    phone_comm_begin(p,true);
    return fx;
}
static void mark_story_missed(phone_t *p){
    if(p->return_page!=PHONE_STORY_CALLS && p->story_call && p->scene_id>=0 && p->scene_id<64)
        p->missed_calls|=UINT64_C(1)<<p->scene_id;
}
void phone_tick(phone_t *p) { if (p->page == PHONE_CALL && p->call_seconds < UINT32_MAX) ++p->call_seconds; }
phone_fx_t phone_advance(phone_t *p, uint32_t elapsed_ms) {
    p->uptime_ms=elapsed_ms>UINT32_MAX-p->uptime_ms?UINT32_MAX:p->uptime_ms+elapsed_ms;
    uint64_t clock=(uint64_t)p->clock_phase_ms+elapsed_ms;uint64_t minutes=p->clock_minutes+clock/2000;p->clock_day=(p->clock_day+(unsigned)(minutes/1440))%7;p->clock_minutes=(unsigned)(minutes%1440);p->clock_phase_ms=(uint32_t)(clock%2000);
    if(world_step(&p->world,elapsed_ms,p->page==PHONE_HOME && p->settings[SET_PROFILE]==0)) {
        p->unread=true;
        for(unsigned i=0;i<call_scene_count();i++){const call_scene_t *scene=call_scene_get(i);if(scene && scene->incoming && (scene->role_mask&(1u<<p->world.role)) && scene->mission_task==world_task(&p->world,0))return phone_incoming_scene(p,(int)i);}
        return FX_TEXT;
    }
    phone_page_t life_before=p->page;phone_fx_t life_fx=phone_comm_step(p,elapsed_ms);if(life_fx!=FX_NONE || p->page!=life_before)return life_fx;
    if(p->page==PHONE_INCOMING){
        p->phase_ms=elapsed_ms>UINT32_MAX-p->phase_ms?UINT32_MAX:p->phase_ms+elapsed_ms;
        if(p->phase_ms>=20000){mark_story_missed(p);phone_comm_end(p,1);p->story_call=false;p->page=p->return_page;return FX_STOP;}
        return FX_NONE;
    }
    if(p->page==PHONE_CALL) {
        uint64_t total=(uint64_t)p->phase_ms+elapsed_ms;
        uint64_t seconds=(uint64_t)p->call_seconds+total/1000;
        p->call_seconds=seconds>UINT32_MAX?UINT32_MAX:(uint32_t)seconds;
        p->phase_ms=(uint32_t)(total%1000);
        return FX_NONE;
    }
    if(p->page!=PHONE_DIALING && p->page!=PHONE_BUSY) return FX_NONE;
    p->phase_ms = elapsed_ms > UINT32_MAX-p->phase_ms ? UINT32_MAX : p->phase_ms+elapsed_ms;
    /* Fixed local demo delay; not a claim about native call setup duration. */
    if(p->page==PHONE_DIALING && p->phase_ms>=3000) {
        p->phase_ms=0;
        const phone_contact_t *contact=phone_current_contact(p);
        if(contact){
            if(contact->native_id==PHONE_TAXI_CONTACT){
                p->page=PHONE_CALL;
                return FX_VOICE;
            }
            int branch=call_scene_outgoing(p->world.role,contact->native_id,p->call_outcome);
            const call_scene_t *scene=branch>=0?call_scene_get((unsigned)branch):NULL;
            if(p->hangout && scene && !scene->incoming && (scene->role_mask&(1u<<p->world.role)) && scene->contact_native_id==contact->native_id){p->scene_id=branch;p->story_call=true;p->page=PHONE_CALL;p->call_seconds=0;return FX_SCENE;}
            if(contact->answerphone_file && *contact->answerphone_file){p->scene_id=-1;p->page=PHONE_CALL;p->call_seconds=0;return FX_VOICE;}
        }
        if(p->return_page==PHONE_NUMBERPAD && !strcmp(p->dial_number,black_phone_number)){
            p->black_phone_roles^=1u<<p->world.role;
            p->page=PHONE_DIAL_RESULT;
            return FX_ACCEPT;
        }
        phone_comm_end(p,2);p->page=PHONE_BUSY; return FX_STOP;
    }
    if(p->page==PHONE_BUSY && p->phase_ms>=1500) { p->page=p->return_page; return FX_STOP; }
    return FX_NONE;
}
phone_fx_t phone_audio_finished(phone_t *p) {
    if(p->page!=PHONE_CALL || (!p->outgoing&&!p->story_call)) return FX_NONE;
    phone_comm_end(p,0);
    const phone_contact_t *contact=phone_current_contact(p);
    if(p->outgoing && contact && contact->native_id==PHONE_TAXI_CONTACT){
        phone_comm_queue(p,FOLLOW_TAXI,PHONE_TAXI_CONTACT,-1);
        p->page=PHONE_DIAL_RESULT;
        return FX_ACCEPT;
    }
    if(p->outgoing && p->story_call && p->hangout && p->call_outcome==0){sim_hangout(&p->sim,p->world.role,phone_current_contact(p)->native_id);p->unread=true;}
    p->story_call=false;p->page=p->return_page;return FX_STOP;
}
const phone_contact_t *phone_current_contact(const phone_t *p){return phonebook_contact(p->world.role,p->selected_contact);}
unsigned phone_number_list_count(const phone_t *p){
    unsigned count=1; /* Enter a number */
    for(unsigned i=0;i<phonebook_count(p->world.role);i++)if(*phonebook_contact(p->world.role,i)->number)count++;
    return count;
}
int phone_number_list_contact(const phone_t *p,unsigned row){
    if(row==0)return phonebook_find(p->world.role,PHONE_TAXI_CONTACT);
    if(row==1)return -1;
    row-=2;
    for(unsigned i=0;i<phonebook_count(p->world.role);i++){
        const phone_contact_t *c=phonebook_contact(p->world.role,i);
        if(c->native_id!=PHONE_TAXI_CONTACT && *c->number && row--==0)return (int)i;
    }
    return -1;
}
static phone_fx_t begin_outgoing(phone_t *p,phone_page_t return_page,bool hangout){
    const phone_contact_t *contact=phone_current_contact(p);
    p->hangout=hangout;
    p->scene_id=-1;p->story_call=false;p->return_page=return_page;
    unsigned role=p->world.role;
    if(contact){
        p->call_outcome=hangout?(p->last_dial_contact[role]==contact->native_id && p->uptime_ms-p->last_dial_ms[role]<45000?2:p->comm.dial_serial[role]++%2):0;
        p->last_dial_contact[role]=(uint16_t)contact->native_id;p->last_dial_ms[role]=p->uptime_ms;
        phone_comm_begin(p,false);
    }
    if(return_page==PHONE_CALL_HISTORY)p->selected_history=0;
    p->page=PHONE_DIALING;p->phase_ms=0;p->outgoing=true;
    return FX_DIAL;
}
static phone_fx_t dial_number_contact(phone_t *p,int contact){
    p->selected_contact=contact<0?phonebook_count(p->world.role):(unsigned)contact;
    const phone_contact_t *person=phone_current_contact(p);
    return begin_outgoing(p,p->page,person && !*person->answerphone_file && person->outgoing_scene>=0);
}
static bool incoming_scene_for_role(const call_scene_t *scene,unsigned role){
    return scene && scene->incoming && role<3 && (scene->role_mask&(1u<<role));
}
unsigned phone_story_call_count(unsigned role){
    unsigned count=0;
    for(unsigned i=0;i<call_scene_count();i++)count+=incoming_scene_for_role(call_scene_get(i),role);
    return count;
}
int phone_story_call_index(unsigned role,unsigned row){
    for(unsigned i=0;i<call_scene_count();i++)
        if(incoming_scene_for_role(call_scene_get(i),role) && row--==0)return (int)i;
    return -1;
}
phone_fx_t phone_incoming_scene(phone_t *p,int scene_id){
    if(scene_id<0)return FX_NONE;
    const call_scene_t *scene=call_scene_get((unsigned)scene_id);
    if(!incoming_scene_for_role(scene,p->world.role))return FX_NONE;
    bool replay=p->page==PHONE_STORY_CALLS;
    int contact=replay?-1:phonebook_find(p->world.role,scene->contact_native_id);
    if(!replay && (contact<0 || p->settings[SET_PROFILE] || p->page==PHONE_CALL || p->page==PHONE_INCOMING || p->page==PHONE_DIALING || p->page==PHONE_BUSY))return FX_NONE;
    phone_fx_t fx=replay?show_incoming(p):phone_incoming(p,(unsigned)contact);
    p->scene_id=scene_id;p->story_call=true;
    if(!replay && scene->mission_task>=0 && scene->mission_task<WORLD_TASKS && scene->mission_task/WORLD_TASKS_PER_ROLE==(int)p->world.role && !(p->world.done[p->world.role]&(1u<<scene->mission_task))){p->world.active[p->world.role]|=1u<<scene->mission_task;p->comm.task_contact[scene->mission_task]=(uint8_t)scene->contact_native_id;p->comm.revision++;}
    return fx;
}
phone_fx_t phone_demo_call(phone_t *p){
    if(p->page!=PHONE_HOME || p->settings[SET_PROFILE])return FX_NONE;
    unsigned count=call_scene_count();
    for(unsigned n=0;n<count;n++){
        unsigned i=(p->demo_scene_cursor+n)%count;
        const call_scene_t *scene=call_scene_get(i);
        if(incoming_scene_for_role(scene,p->world.role) && phonebook_find(p->world.role,scene->contact_native_id)>=0){p->demo_scene_cursor=(i+1)%count;return phone_incoming_scene(p,(int)i);}
    }
    return FX_NONE;
}
static unsigned missed_count(const phone_t *p){unsigned n=0;for(unsigned i=0;i<call_scene_count() && i<64;i++){const call_scene_t *scene=call_scene_get(i);if((p->missed_calls&(UINT64_C(1)<<i)) && scene && (scene->role_mask&(1u<<p->world.role)))n++;}return n;}
static unsigned legacy_story_count(const phone_t *p,story_kind_t kind){return story_count(p->world.role,kind)+(kind==STORY_TEXT?missed_count(p)+2+((p->egg_seen>>p->world.role)&1u):kind==STORY_WEB?4:0);}
unsigned phone_story_count(const phone_t *p,story_kind_t kind){return legacy_story_count(p,kind)+(kind==STORY_TEXT?phone_comm_message_count(p):0);}
const story_item_t *phone_story_item(const phone_t *p,story_kind_t kind,unsigned index){
    unsigned messages=kind==STORY_TEXT?phone_comm_message_count(p):0;
    if(index<messages)return phone_comm_item(p,index);
    index-=messages;
    static char account_body[640];static story_item_t account;
    if(kind==STORY_WEB){
        if(index==story_count(p->world.role,STORY_WEB)+3){static const story_item_t ad={"Vice City Travel","Vice City Travel","VICE CITY - Channel VI\nYour next chapter is calling...\nConnection lost.","local:vice-city-easter-egg"};return &ad;}
        if(index==0){sim_summary(&p->sim,p->world.role,account_body,sizeof(account_body));account=(story_item_t){"Offline account","Account",account_body,"local:virtual-account"};return &account;}
        if(index<3){static const story_item_t offers[2]={{"Epsilon / offline sandbox","Donate $500","Original request amount: $500 (EMSTR_24).\n\nThis is a local sandbox transaction, not the live game website.","EMSTR_24+local:checkout"},{"Epsilon / offline sandbox","Donate $5000","Original request amount: $5000 (EMSTR_26).\n\nThis is a local sandbox transaction, not the live game website.","EMSTR_26+local:checkout"}};return &offers[index-1];}
        return story_item(p->world.role,kind,index-3);
    }
    unsigned base=story_count(p->world.role,kind);
    if(kind==STORY_TEXT && (p->egg_seen&(1u<<p->world.role)) && index==base+missed_count(p)+2){static const story_item_t egg={"Unknown / VI","Wrong city. Right number.","Wrong city. Right number.\n\nNext stop: Vice City.\n\nChannel VI - Connection lost.","local:original-fan-easter-egg"};return &egg;}
    if(kind==STORY_TEXT && index==base+missed_count(p)+1){static char task_body[240];int task=world_task(&p->world,0);snprintf(task_body,sizeof(task_body),"LOCAL REPLAY\n\nOpen: %s\nCompleted: %u / 10\n\nComplete tasks in Check List. Rewards and progress are local simulation.",task>=0?mission_names[task]:"None",world_count(&p->world,true));account=(story_item_t){"Local replay","Task update",task_body,"local:task-update"};return &account;}
    if(kind==STORY_TEXT && index==base+missed_count(p)){sim_summary(&p->sim,p->world.role,account_body,sizeof(account_body));account=(story_item_t){"Offline account","Account receipt",account_body,"local:account-receipt"};return &account;}
    if(kind==STORY_TEXT && index>=base){unsigned row=index-base;
        for(unsigned i=0;i<call_scene_count() && i<64;i++){
            const call_scene_t *scene=call_scene_get(i);
            if(!(p->missed_calls&(UINT64_C(1)<<i)) || !scene || !(scene->role_mask&(1u<<p->world.role)))continue;
            if(row--!=0)continue;
            if(!strcmp(scene->id,"fam1_jimmy_yacht") && p->world.role==0)return story_missed_call();
            int contact_index=phonebook_find(p->world.role,scene->contact_native_id);
            const phone_contact_t *contact=contact_index<0?NULL:phonebook_contact(p->world.role,(unsigned)contact_index);
            static story_item_t item;
            item=(story_item_t){contact?contact->name:"Unknown","Missed call","Local call history: you missed this call.",scene->id};return &item;
        }
        return NULL;
    }
    return story_item(p->world.role,kind,index);
}
bool phone_story_is_read(const phone_t *p,story_kind_t kind,unsigned index){
 unsigned messages=kind==STORY_TEXT?phone_comm_message_count(p):0;
 if(index<messages){comm_message_t *m=phone_comm_message((phone_t *)p,index);return m&&m->read;}
 index-=messages;return kind<STORY_WEB && index<64 && (p->read_bits[p->world.role][kind]&(UINT64_C(1)<<index));
}
void phone_story_mark_read(phone_t *p,story_kind_t kind,unsigned index){
 unsigned messages=kind==STORY_TEXT?phone_comm_message_count(p):0;
 if(index<messages){comm_message_t *m=phone_comm_message(p,index);if(m&&!m->read){m->read=1;p->comm.revision++;}return;}
 index-=messages;if(kind<STORY_WEB && index<64)p->read_bits[p->world.role][kind]|=UINT64_C(1)<<index;
}
void phone_save_result(phone_t *p,bool success){p->save_status=success?1:-1;p->page=PHONE_SAVE_RESULT;}
phone_fx_t phone_key(phone_t *p, phone_key_t key) {
    if(p->page==PHONE_EASTER_EGG){
        if(key!=KEY_OK && key!=KEY_BACK)return FX_NONE;
        p->page=PHONE_INTERNET;
        if(!(p->egg_seen&(1u<<p->world.role))){p->egg_seen|=1u<<p->world.role;p->unread=true;unsigned index=legacy_story_count(p,STORY_TEXT)-1;if(index<64)p->read_bits[p->world.role][STORY_TEXT]&=~(UINT64_C(1)<<index);return FX_TEXT;}
        return FX_BACK;
    }
    if(p->page==PHONE_DIAL_RESULT){
        if(key!=KEY_BACK && key!=KEY_OK)return FX_NONE;
        p->page=p->return_page;
        return FX_BACK;
    }
    if(p->page==PHONE_NUMBERPAD){
        if(key==KEY_BACK){p->page=PHONE_NUMBER_LIST;return FX_BACK;}
        if(key==KEY_UP || key==KEY_DOWN){
            unsigned count=PHONE_KEYPAD_CALL+1;
            p->selected_digit=(p->selected_digit+(key==KEY_UP?count-1:1))%count;
            return FX_NAV;
        }
        if(key!=KEY_OK)return FX_NONE;
        size_t length=strlen(p->dial_number);
        if(p->selected_digit==PHONE_KEYPAD_CALL){
            if(!length)return FX_NONE;
            for(unsigned i=0;i<phonebook_count(p->world.role);i++)
                if(!strcmp(phonebook_contact(p->world.role,i)->number,p->dial_number))return dial_number_contact(p,(int)i);
            return dial_number_contact(p,-1);
        }
        if(p->selected_digit==PHONE_KEYPAD_ERASE){
            if(!length)return FX_NONE;
            p->dial_number[length-1]=0;
        }else{
            if(length==PHONE_NUMBER_CAPACITY)return FX_NONE;
            p->dial_number[length]=p->selected_digit==10?'0':'1'+p->selected_digit;
            p->dial_number[length+1]=0;
        }
        return FX_ACCEPT;
    }
    if(key==KEY_DIAL_PAD) {
        if(p->page==PHONE_CONTACTS){p->selected_number=0;p->page=PHONE_NUMBER_LIST;return FX_ACCEPT;}
        return FX_NONE;
    }
    if (key == KEY_BACK) {
        if(p->page==PHONE_HOME){p->selected_role=p->world.role;p->page=PHONE_ROLE;return FX_ACCEPT;}
        if(p->page==PHONE_CALL_HISTORY){p->page=PHONE_SETTINGS;return FX_BACK;}
        if(p->page==PHONE_NUMBER_LIST){p->page=PHONE_CONTACTS;if(!phone_current_contact(p))p->selected_contact=0;return FX_BACK;}
        if(p->page==PHONE_CONTACT_ACTIONS){p->page=p->history_return?PHONE_CALL_HISTORY:PHONE_CONTACTS;return FX_BACK;}
        if(p->page==PHONE_PURCHASE_CONFIRM || p->page==PHONE_TRANSACTION_RESULT){p->page=PHONE_INTERNET;return FX_BACK;}
        if(p->page==PHONE_INCOMING)mark_story_missed(p);
        if(p->page==PHONE_READER){p->page=p->reader_kind==STORY_EMAIL?PHONE_EMAIL:p->reader_kind==STORY_TEXT?PHONE_MESSAGES:PHONE_INTERNET;return FX_BACK;}
        if(p->page==PHONE_PHOTO){p->page=p->photo_from_gallery?PHONE_GALLERY:PHONE_VIEWFINDER;return FX_BACK;}
        if(p->page==PHONE_VIEWFINDER){p->page=PHONE_CAMERA;return FX_BACK;}
        if(p->page==PHONE_GALLERY){p->page=PHONE_CAMERA;return FX_BACK;}

        if(p->page==PHONE_TASK_DETAIL){p->page=PHONE_OPEN_MISSIONS;return FX_BACK;}
        if(p->page==PHONE_OPEN_MISSIONS || p->page==PHONE_PROGRESS){p->page=PHONE_CHECKLIST;return FX_BACK;}
        if (p->page == PHONE_INCOMING || p->page == PHONE_CALL || p->page==PHONE_DIALING || p->page==PHONE_BUSY) { phone_comm_end(p,2);p->story_call=false;p->page = p->return_page; return FX_STOP; }
        if(p->page==PHONE_SETTING_OPTIONS) {p->page=PHONE_SETTINGS;return FX_BACK;}
        p->page = PHONE_HOME; return FX_BACK;
    }
    if (p->page == PHONE_INCOMING) {
        if (key != KEY_OK) return FX_NONE;
        phone_comm_answer(p);p->page = PHONE_CALL; p->call_seconds = 0; p->phase_ms=0; return p->story_call?FX_SCENE:FX_ACCEPT;
    }
    if (p->page == PHONE_CALL || p->page==PHONE_DIALING || p->page==PHONE_BUSY) {
        if (key == KEY_OK) {phone_comm_end(p,2);p->story_call=false;p->page = p->return_page; return FX_STOP; }
        return FX_NONE;
    }
    if(p->page==PHONE_HOME && (key==KEY_UP || key==KEY_DOWN || key==KEY_LEFT || key==KEY_RIGHT)){
        unsigned count=9;
        bool previous=key==KEY_UP || key==KEY_LEFT;
        p->selected_app=(p->selected_app+count+(previous?-1:1))%count;
        return FX_NAV;
    }
    if (key == KEY_UP || key == KEY_DOWN) {
        if(p->page==PHONE_VIEWFINDER){p->selected_photo=(p->selected_photo+SNAP_SCENES+(key==KEY_UP?-1:1))%SNAP_SCENES;return FX_NAV;}
        if(p->page==PHONE_PHOTO){if(!p->photo_from_gallery)return FX_NONE;unsigned count=phone_photo_count(p);if(!count)return FX_NONE;p->selected_list=(p->selected_list+count+(key==KEY_UP?-1:1))%count;p->selected_photo=phone_photo_at(p,p->selected_list);return FX_NAV;}

        if(p->page==PHONE_READER){
            if(key==KEY_UP && p->reader_scroll){p->reader_scroll--;return FX_NAV;}
            if(key==KEY_DOWN && p->reader_scroll<p->reader_limit){p->reader_scroll++;return FX_NAV;}
            if(p->reader_kind==STORY_WEB)return FX_NONE;
            unsigned *n=&p->selected_story[p->reader_kind],count=phone_story_count(p,p->reader_kind);
            if((key==KEY_UP && *n==0)||(key==KEY_DOWN && *n+1>=count))return FX_NONE;
            *n+=key==KEY_UP?-1:1;p->reader_scroll=0;p->reader_limit=0;
            phone_story_mark_read(p,p->reader_kind,*n);
            return FX_NAV;
        }

        unsigned *selection = &p->selected_app; unsigned count = 9;
        if (p->page == PHONE_CONTACTS) { selection = &p->selected_contact; count = phonebook_count(p->world.role);if(!count)return FX_NONE; }
        else if(p->page==PHONE_CALL_HISTORY){selection=&p->selected_history;count=phone_comm_history_count(p);if(!count)return FX_NONE;}
        else if(p->page==PHONE_NUMBER_LIST){selection=&p->selected_number;count=phone_number_list_count(p);}
        else if(p->page==PHONE_STORY_CALLS){selection=&p->selected_story_call;count=phone_story_call_count(p->world.role);if(!count)return FX_NONE;}
        else if(p->page==PHONE_CONTACT_ACTIONS){selection=&p->contact_action;count=2;}
        else if(p->page==PHONE_ROLE){selection=&p->selected_role;count=3;}
        else if(p->page==PHONE_CHECKLIST){selection=&p->selected_list;count=2;}
        else if(p->page==PHONE_OPEN_MISSIONS){selection=&p->selected_task;count=world_count(&p->world,false);if(!count)return FX_NONE;}
        else if(p->page==PHONE_EMAIL || p->page==PHONE_MESSAGES || p->page==PHONE_INTERNET){unsigned kind=p->page==PHONE_EMAIL?STORY_EMAIL:p->page==PHONE_MESSAGES?STORY_TEXT:STORY_WEB;selection=&p->selected_story[kind];count=phone_story_count(p,(story_kind_t)kind);if(!count)return FX_NONE;}
        else if(p->page==PHONE_CAMERA){selection=&p->selected_list;count=2;}
        else if(p->page==PHONE_GALLERY){selection=&p->selected_list;count=phone_photo_count(p);if(!count)return FX_NONE;}
        else if(p->page==PHONE_SETTINGS) {selection=&p->selected_setting;count=SET_COUNT+2;}
        else if(p->page==PHONE_SETTING_OPTIONS) {selection=&p->selected_option;count=setting_counts[p->selected_setting];}
        else if (p->page != PHONE_HOME) return FX_NONE;
        if(p->page==PHONE_SETTING_OPTIONS && (p->selected_setting==SET_VOLUME || p->selected_setting==SET_BRIGHTNESS)){
            if((key==KEY_UP && !*selection)||(key==KEY_DOWN && *selection+1>=count))return FX_NONE;
        }
        *selection = (*selection + (key == KEY_UP ? count - 1 : 1)) % count;
        if(p->page==PHONE_SETTING_OPTIONS && (p->selected_setting==SET_VOLUME || p->selected_setting==SET_BRIGHTNESS))return FX_HW_PREVIEW;
        return p->page==PHONE_SETTING_OPTIONS && p->selected_setting==SET_RINGTONE?FX_RING_PREVIEW:FX_NAV;
    }
    if (key != KEY_OK) return FX_NONE;
    if(p->page==PHONE_PROGRESS){p->page=PHONE_CHECKLIST;return FX_BACK;}
    if(p->page==PHONE_READER){p->page=p->reader_kind==STORY_EMAIL?PHONE_EMAIL:p->reader_kind==STORY_TEXT?PHONE_MESSAGES:PHONE_INTERNET;return FX_BACK;}
    if(p->page==PHONE_SAVE_RESULT || p->page==PHONE_TRANSACTION_RESULT){p->page=p->page==PHONE_SAVE_RESULT?PHONE_HOME:PHONE_INTERNET;return FX_BACK;}
    if(p->page==PHONE_PURCHASE_CONFIRM){bool ok=sim_buy(&p->sim,p->world.role,p->purchase_item);p->save_status=ok?1:-1;p->page=PHONE_TRANSACTION_RESULT;p->unread=ok;return ok?FX_TEXT:FX_BACK;}
    if (p->page == PHONE_HOME) {
        static const phone_page_t pages[9]={PHONE_EMAIL,PHONE_MESSAGES,PHONE_CHECKLIST,PHONE_SAVE,PHONE_CONTACTS,PHONE_SETTINGS,PHONE_CAMERA,PHONE_INTERNET,PHONE_STORY_CALLS};
        p->page = pages[p->selected_app];
        if(p->page==PHONE_CAMERA)p->selected_list=0;
        if(p->page==PHONE_CONTACTS && !phone_current_contact(p))p->selected_contact=0;
        if(p->page==PHONE_STORY_CALLS)p->selected_story_call=0;
        if(p->page==PHONE_SAVE){p->save_status=0;return FX_SAVE;}
    } else if(p->page==PHONE_NUMBER_LIST){
        if(p->selected_number==1){
            p->dial_number[0]=0;p->selected_digit=0;p->page=PHONE_NUMBERPAD;
            return FX_ACCEPT;
        }
        int contact=phone_number_list_contact(p,p->selected_number);
        return contact<0?FX_NONE:dial_number_contact(p,contact);
    } else if(p->page==PHONE_STORY_CALLS){
        return phone_incoming_scene(p,phone_story_call_index(p->world.role,p->selected_story_call));
    } else if (p->page == PHONE_CONTACTS || p->page==PHONE_CONTACT_ACTIONS || p->page==PHONE_CALL_HISTORY) {
        if(p->page==PHONE_CONTACTS)p->history_return=false;
        if(p->page==PHONE_CALL_HISTORY){const comm_log_t *log=phone_comm_history(p,p->selected_history);if(!log)return FX_NONE;int c=phonebook_find(p->world.role,log->contact);if(c<0)return FX_NONE;p->selected_contact=(unsigned)c;p->history_return=true;}
        const phone_contact_t *contact=phone_current_contact(p);if(!contact)return FX_NONE;
        if((p->page==PHONE_CONTACTS || p->page==PHONE_CALL_HISTORY) && contact->outgoing_scene>=0){p->contact_action=0;p->page=PHONE_CONTACT_ACTIONS;return FX_ACCEPT;}
        return begin_outgoing(p,p->history_return?PHONE_CALL_HISTORY:PHONE_CONTACTS,p->page==PHONE_CONTACT_ACTIONS && p->contact_action==1);
    } else if(p->page==PHONE_EMAIL || p->page==PHONE_MESSAGES || p->page==PHONE_INTERNET) {
        p->reader_kind=p->page==PHONE_EMAIL?STORY_EMAIL:p->page==PHONE_MESSAGES?STORY_TEXT:STORY_WEB;
        unsigned n=p->selected_story[p->reader_kind];
        if(p->reader_kind==STORY_WEB && n==story_count(p->world.role,STORY_WEB)+3){p->page=PHONE_EASTER_EGG;return FX_ACCEPT;}
        if(p->reader_kind==STORY_WEB && n>=1 && n<=2){p->purchase_item=n-1;p->page=PHONE_PURCHASE_CONFIRM;return FX_ACCEPT;}
        if(n<phone_story_count(p,p->reader_kind)){
            p->page=PHONE_READER;p->reader_scroll=0;p->reader_limit=0;
            phone_story_mark_read(p,p->reader_kind,n);
        }
    } else if(p->page==PHONE_CAMERA){p->selected_photo=p->world.role;p->selected_list=p->selected_list?1:0;if(p->selected_list){p->selected_list=0;p->page=PHONE_GALLERY;}else p->page=PHONE_VIEWFINDER;
    } else if(p->page==PHONE_VIEWFINDER){p->photo_from_gallery=false;p->page=PHONE_PHOTO;return FX_SHUTTER;
    } else if(p->page==PHONE_GALLERY){if(!phone_photo_count(p))return FX_NONE;p->selected_photo=phone_photo_at(p,p->selected_list);p->photo_from_gallery=true;p->page=PHONE_PHOTO;
    } else if(p->page==PHONE_PHOTO){p->snap_bits[p->world.role]|=1u<<p->selected_photo;p->selected_list=0;for(unsigned i=0;i<p->selected_photo;i++)if(p->snap_bits[p->world.role]&(1u<<i))p->selected_list++;p->page=PHONE_GALLERY;
    } else if(p->page==PHONE_SETTINGS) {
        if(p->selected_setting==SET_COUNT+1){p->selected_history=0;p->page=PHONE_CALL_HISTORY;return FX_ACCEPT;}
        if(p->selected_setting==SET_COUNT){p->selected_role=p->world.role;p->page=PHONE_ROLE;return FX_ACCEPT;}
        p->page=PHONE_SETTING_OPTIONS;p->selected_option=p->settings[p->selected_setting];
    } else if(p->page==PHONE_SETTING_OPTIONS) {
        p->settings[p->selected_setting]=p->selected_option;p->page=PHONE_SETTINGS;
    } else if(p->page==PHONE_ROLE) {
        p->world.role=p->selected_role;p->selected_task=0;p->selected_contact=0;p->scene_id=-1;memset(p->selected_story,0,sizeof(p->selected_story));p->page=PHONE_HOME;p->unread=false;
    } else if(p->page==PHONE_CHECKLIST) {
        p->selected_task=0;p->page=p->selected_list?PHONE_PROGRESS:PHONE_OPEN_MISSIONS;
    } else if(p->page==PHONE_OPEN_MISSIONS) {
        if(world_task(&p->world,p->selected_task)>=0)p->page=PHONE_TASK_DETAIL;
    } else if(p->page==PHONE_TASK_DETAIL) {
        int task=world_task(&p->world,p->selected_task);
        if(task>=0 && world_complete(&p->world,(unsigned)task)){phone_comm_queue(p,FOLLOW_TASK,p->comm.task_contact[task],task);sim_reward(&p->sim,p->world.role,(unsigned)task);p->unread=true;p->selected_task=0;p->page=PHONE_OPEN_MISSIONS;return FX_TEXT;}
        p->selected_task=0;p->page=PHONE_OPEN_MISSIONS;
    } else if(p->page==PHONE_MESSAGES) {
        p->unread=false;
    }
    return FX_ACCEPT;
}

const char *phone_ring_file(unsigned selection,unsigned role){
 static const char *const native[3]={"/assets/native_ring_1.wav","/assets/native_ring_2.wav","/assets/native_ring_3.wav"};
 static const char *const clips[3]={"/assets/ring_clip_0.wav","/assets/ring_clip_1.wav","/assets/ring_clip_2.wav"};
 if(selection==7){if(role>=3)return NULL;selection=role;}
 if(selection<3)return native[selection];
 return selection>=4 && selection<7?clips[selection-4]:NULL;
}

unsigned phone_photo_count(const phone_t *p){unsigned n=0;for(unsigned i=0;i<SNAP_SCENES;i++)n+=(p->snap_bits[p->world.role]>>i)&1u;return n;}
unsigned phone_photo_at(const phone_t *p,unsigned row){for(unsigned i=0;i<SNAP_SCENES;i++)if((p->snap_bits[p->world.role]&(1u<<i)) && row--==0)return i;return 0;}
