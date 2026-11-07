#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "content.h"
#include "world.h"
typedef enum { PHONE_HOME, PHONE_CONTACTS, PHONE_MESSAGES, PHONE_INCOMING, PHONE_CALL, PHONE_PENDING, PHONE_EMAIL, PHONE_CHECKLIST, PHONE_SAVE, PHONE_SETTINGS, PHONE_CAMERA, PHONE_INTERNET, PHONE_TRACKIFY, PHONE_DIALING, PHONE_SETTING_OPTIONS, PHONE_BUSY, PHONE_ROLE, PHONE_OPEN_MISSIONS, PHONE_TASK_DETAIL, PHONE_PROGRESS } phone_page_t;
typedef enum { KEY_UP, KEY_DOWN, KEY_OK, KEY_BACK, KEY_ROLE } phone_key_t;
typedef enum { FX_NONE, FX_NAV, FX_ACCEPT, FX_BACK, FX_RING, FX_VOICE, FX_STOP, FX_DIAL, FX_TEXT } phone_fx_t;
typedef struct {
    world_t world;
    unsigned selected_role, selected_list, selected_task;
    phone_page_t page, return_page;
    unsigned selected_app, selected_contact, selected_message;
    unsigned selected_setting, selected_option, settings[SET_COUNT];
    uint32_t call_seconds;
    uint32_t phase_ms;
    bool outgoing;
    bool unread;
    bool trackify_unlocked;
} phone_t;
void phone_init(phone_t *p);
bool phone_restore_settings(phone_t *p, const uint32_t values[SET_COUNT]);
phone_fx_t phone_key(phone_t *p, phone_key_t key);
phone_fx_t phone_incoming(phone_t *p, unsigned contact);
void phone_tick(phone_t *p);
phone_fx_t phone_advance(phone_t *p, uint32_t elapsed_ms);
phone_fx_t phone_audio_finished(phone_t *p);
extern const char *const phone_contacts[6];
extern const char *const phone_apps[9];
