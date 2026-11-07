#include "phone.h"
#include <string.h>
const char *const phone_contacts[6] = {"Franklin", "Trevor", "Lester", "Amanda", "Jimmy", "Dave"};
const char *const phone_apps[9] = {"Email", "Texts", "Check List", "Quick Save", "Contacts", "Settings", "Snapmatic", "Internet", "Trackify"};
void phone_init(phone_t *p) { memset(p, 0, sizeof(*p)); world_init(&p->world);p->page = PHONE_HOME; p->selected_app = 4; p->settings[SET_VIBRATE]=1; }
bool phone_restore_settings(phone_t *p,const uint32_t values[SET_COUNT]) {
    for(unsigned i=0;i<SET_COUNT;i++)if(values[i]>=setting_counts[i])return false;
    for(unsigned i=0;i<SET_COUNT;i++)p->settings[i]=values[i];
    return true;
}
phone_fx_t phone_incoming(phone_t *p, unsigned contact) {
    if (p->page == PHONE_CALL || p->page == PHONE_INCOMING || p->page == PHONE_DIALING || p->page == PHONE_BUSY || contact >= 6 || p->settings[SET_PROFILE]==1) return FX_NONE;
    p->return_page = p->page; p->page = PHONE_INCOMING; p->selected_contact = contact; p->outgoing=false;
    return p->settings[SET_RINGTONE]==3 ? FX_NONE : FX_RING;
}
void phone_tick(phone_t *p) { if (p->page == PHONE_CALL && p->call_seconds < UINT32_MAX) ++p->call_seconds; }
phone_fx_t phone_advance(phone_t *p, uint32_t elapsed_ms) {
    if(world_step(&p->world,elapsed_ms,p->page==PHONE_HOME && p->settings[SET_PROFILE]==0)) {p->unread=true;return FX_TEXT;}
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
        if(contact_has_voice[p->selected_contact]) { p->page=PHONE_CALL; p->call_seconds=0; return FX_VOICE; }
        p->page=PHONE_BUSY; return FX_STOP;
    }
    if(p->page==PHONE_BUSY && p->phase_ms>=1500) { p->page=p->return_page; return FX_STOP; }
    return FX_NONE;
}
phone_fx_t phone_audio_finished(phone_t *p) {
    if(p->page!=PHONE_CALL || !p->outgoing) return FX_NONE;
    p->page=p->return_page;return FX_STOP;
}
phone_fx_t phone_key(phone_t *p, phone_key_t key) {
    if(key==KEY_ROLE) {if(p->page==PHONE_HOME){p->selected_role=p->world.role;p->page=PHONE_ROLE;return FX_ACCEPT;}return FX_NONE;}
    if (key == KEY_BACK) {
        if(p->page==PHONE_TASK_DETAIL){p->page=PHONE_OPEN_MISSIONS;return FX_BACK;}
        if(p->page==PHONE_OPEN_MISSIONS || p->page==PHONE_PROGRESS){p->page=PHONE_CHECKLIST;return FX_BACK;}
        if (p->page == PHONE_INCOMING || p->page == PHONE_CALL || p->page==PHONE_DIALING || p->page==PHONE_BUSY) { p->page = p->return_page; return FX_STOP; }
        if(p->page==PHONE_SETTING_OPTIONS) {p->page=PHONE_SETTINGS;return FX_BACK;}
        p->page = PHONE_HOME; return FX_BACK;
    }
    if (p->page == PHONE_INCOMING) {
        if (key != KEY_OK) return FX_NONE;
        p->page = PHONE_CALL; p->call_seconds = 0; p->phase_ms=0; return FX_ACCEPT; /* No verified incoming conversation yet. */
    }
    if (p->page == PHONE_CALL || p->page==PHONE_DIALING || p->page==PHONE_BUSY) {
        if (key == KEY_OK) { p->page = p->return_page; return FX_STOP; }
        return FX_NONE;
    }
    if (key == KEY_UP || key == KEY_DOWN) {
        unsigned *selection = &p->selected_app; unsigned count = p->trackify_unlocked ? 9 : 8;
        if (p->page == PHONE_CONTACTS) { selection = &p->selected_contact; count = 6; }
        else if(p->page==PHONE_ROLE){selection=&p->selected_role;count=3;}
        else if(p->page==PHONE_CHECKLIST){selection=&p->selected_list;count=2;}
        else if(p->page==PHONE_OPEN_MISSIONS){selection=&p->selected_task;count=world_count(&p->world,false);if(!count)return FX_NONE;}
        else if(p->page==PHONE_SETTINGS) {selection=&p->selected_setting;count=SET_COUNT;}
        else if(p->page==PHONE_SETTING_OPTIONS) {selection=&p->selected_option;count=setting_counts[p->selected_setting];}
        else if (p->page != PHONE_HOME) return FX_NONE;
        *selection = (*selection + (key == KEY_UP ? count - 1 : 1)) % count;
        return FX_NAV;
    }
    if (key != KEY_OK) return FX_NONE;
    if (p->page == PHONE_HOME) {
        static const phone_page_t pages[9]={PHONE_EMAIL,PHONE_MESSAGES,PHONE_CHECKLIST,PHONE_SAVE,PHONE_CONTACTS,PHONE_SETTINGS,PHONE_CAMERA,PHONE_INTERNET,PHONE_TRACKIFY};
        p->page = pages[p->selected_app];
    } else if (p->page == PHONE_CONTACTS) {
        p->return_page = PHONE_CONTACTS; p->page = PHONE_DIALING; p->phase_ms=0; p->outgoing=true; return FX_DIAL;
    } else if(p->page==PHONE_SETTINGS) {
        p->page=PHONE_SETTING_OPTIONS;p->selected_option=p->settings[p->selected_setting];
    } else if(p->page==PHONE_SETTING_OPTIONS) {
        p->settings[p->selected_setting]=p->selected_option;
    } else if(p->page==PHONE_ROLE) {
        p->world.role=p->selected_role;p->selected_task=0;p->page=PHONE_HOME;p->unread=false;
    } else if(p->page==PHONE_CHECKLIST) {
        p->selected_task=0;p->page=p->selected_list?PHONE_PROGRESS:PHONE_OPEN_MISSIONS;
    } else if(p->page==PHONE_OPEN_MISSIONS) {
        if(world_task(&p->world,p->selected_task)>=0)p->page=PHONE_TASK_DETAIL;
    } else if(p->page==PHONE_TASK_DETAIL) {
        int task=world_task(&p->world,p->selected_task);
        if(task>=0)world_complete(&p->world,(unsigned)task);
        p->selected_task=0;p->page=PHONE_OPEN_MISSIONS;
    } else if(p->page==PHONE_MESSAGES) {
        p->unread=false;
    }
    return FX_ACCEPT;
}
