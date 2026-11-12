#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "content.h"
#include "world.h"
#include "story.h"
#include "phonebook.h"
#include "simulation.h"
#include "communications.h"
#define SNAP_SCENES 4
#define PHONE_NUMBER_CAPACITY 11
#define PHONE_KEYPAD_ERASE 9
#define PHONE_KEYPAD_CALL 11
typedef enum { PHONE_HOME, PHONE_CONTACTS, PHONE_MESSAGES, PHONE_INCOMING, PHONE_CALL, PHONE_PENDING, PHONE_EMAIL, PHONE_CHECKLIST, PHONE_SAVE, PHONE_SETTINGS, PHONE_CAMERA, PHONE_INTERNET, PHONE_NUMBER_LIST, PHONE_DIALING, PHONE_SETTING_OPTIONS, PHONE_BUSY, PHONE_ROLE, PHONE_OPEN_MISSIONS, PHONE_TASK_DETAIL, PHONE_PROGRESS, PHONE_READER, PHONE_GALLERY, PHONE_PHOTO, PHONE_SAVE_RESULT, PHONE_PURCHASE_CONFIRM, PHONE_TRANSACTION_RESULT, PHONE_CONTACT_ACTIONS, PHONE_EASTER_EGG, PHONE_CALL_HISTORY, PHONE_VIEWFINDER, PHONE_NUMBERPAD, PHONE_DIAL_RESULT, PHONE_STORY_CALLS } phone_page_t;
typedef enum { KEY_UP, KEY_DOWN, KEY_OK, KEY_BACK, KEY_DIAL_PAD, KEY_LEFT, KEY_RIGHT, KEY_DEMO } phone_key_t;
typedef enum { FX_NONE, FX_NAV, FX_ACCEPT, FX_BACK, FX_RING, FX_VOICE, FX_STOP, FX_DIAL, FX_TEXT, FX_SAVE, FX_RING_PREVIEW, FX_SHUTTER, FX_SCENE, FX_HW_PREVIEW } phone_fx_t;
typedef struct {
    comm_state_t comm;
    comm_message_t pending[3];unsigned pending_count,selected_history;
    uint32_t notify_wait,life_wait[3],uptime_ms,last_dial_ms[3];
    uint16_t last_dial_contact[3];bool comm_active,history_return;
    world_t world;
    sim_t sim;
    unsigned purchase_item, contact_action, call_outcome;
    bool hangout;
    uint64_t read_bits[3][2];
    uint32_t photo_bits[3];
    uint32_t egg_seen;
    unsigned selected_story[3], reader_scroll, reader_limit, selected_photo;
    story_kind_t reader_kind;
    int save_status;
    unsigned selected_role, selected_list, selected_task;
    phone_page_t page, return_page;
    unsigned selected_app, selected_contact, selected_message;
    unsigned selected_setting, selected_option, settings[SET_COUNT];
    uint32_t call_seconds;
    uint32_t phase_ms;
    uint32_t clock_minutes,clock_phase_ms,clock_day;
    bool outgoing;
    bool story_call;
    int scene_id;
    unsigned demo_scene_cursor;
    uint64_t missed_calls;
    bool unread;
    bool photo_from_gallery;
    uint32_t snap_bits[3];
    unsigned selected_number, selected_digit, selected_story_call;
    char dial_number[PHONE_NUMBER_CAPACITY+1];
    uint8_t black_phone_roles;
} phone_t;
void phone_init(phone_t *p);
bool phone_restore_settings(phone_t *p, const uint32_t values[SET_COUNT]);
phone_fx_t phone_key(phone_t *p, phone_key_t key);
phone_fx_t phone_incoming(phone_t *p, unsigned contact);
void phone_tick(phone_t *p);
phone_fx_t phone_advance(phone_t *p, uint32_t elapsed_ms);
phone_fx_t phone_audio_finished(phone_t *p);
phone_fx_t phone_incoming_scene(phone_t *p,int scene_id);
const phone_contact_t *phone_current_contact(const phone_t *p);
unsigned phone_number_list_count(const phone_t *p);
int phone_number_list_contact(const phone_t *p,unsigned row);
unsigned phone_story_call_count(unsigned role);
int phone_story_call_index(unsigned role,unsigned row);
extern const char *const phone_apps[9];

void phone_save_result(phone_t *p, bool success);
phone_fx_t phone_demo_call(phone_t *p);
unsigned phone_story_count(const phone_t *p,story_kind_t kind);
const story_item_t *phone_story_item(const phone_t *p,story_kind_t kind,unsigned index);

const char *phone_ring_file(unsigned selection,unsigned role);

void phone_comm_begin(phone_t *,bool incoming);
void phone_comm_answer(phone_t *);
void phone_comm_end(phone_t *,unsigned reason);
void phone_comm_queue(phone_t *,unsigned kind,unsigned contact,int task);
phone_fx_t phone_comm_step(phone_t *,uint32_t elapsed);
unsigned phone_comm_history_count(const phone_t *);
const comm_log_t *phone_comm_history(const phone_t *,unsigned index);
unsigned phone_comm_message_count(const phone_t *);
comm_message_t *phone_comm_message(phone_t *,unsigned index);
const story_item_t *phone_comm_item(const phone_t *,unsigned index);
bool phone_story_is_read(const phone_t *,story_kind_t,unsigned);
void phone_story_mark_read(phone_t *,story_kind_t,unsigned);

unsigned phone_photo_count(const phone_t *);
unsigned phone_photo_at(const phone_t *,unsigned);
