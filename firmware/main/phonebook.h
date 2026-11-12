#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef struct {
 unsigned native_id;
 const char *name, *portrait_key, *answerphone_file;
 int outgoing_scene;
 const char *number;
} phone_contact_t;
#define PHONE_TAXI_CONTACT 131
typedef struct {
 const char *id;
 unsigned contact_native_id;
 uint8_t role_mask;
 bool incoming;
 int mission_task;
 const char *device_file;
 unsigned outcome;
 const char *title, *caller_name, *portrait_key;
} call_scene_t;
unsigned phonebook_count(unsigned role);
const phone_contact_t *phonebook_contact(unsigned role,unsigned index);
int phonebook_find(unsigned role,unsigned native_id);
const char *phone_taxi_voice_file(unsigned role);
int call_scene_outgoing(unsigned role,unsigned native_id,unsigned outcome);
unsigned call_scene_count(void);
const call_scene_t *call_scene_get(unsigned index);
int call_scene_default_incoming(unsigned role);
