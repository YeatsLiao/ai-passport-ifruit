#pragma once
typedef enum { STORY_EMAIL, STORY_TEXT, STORY_WEB } story_kind_t;
typedef struct { const char *sender, *title, *body, *source_key; } story_item_t;
unsigned story_count(unsigned role, story_kind_t kind);
const story_item_t *story_item(unsigned role, story_kind_t kind, unsigned index);
#include <stdint.h>
uint32_t story_revision(void);
const story_item_t *story_missed_call(void);
