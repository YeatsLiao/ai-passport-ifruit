#include "story.h"
#include <stddef.h>
typedef struct {const story_item_t *items;unsigned count;} story_slice_t;
#include "story_data.inc"
unsigned story_count(unsigned role,story_kind_t kind){
    return role<3 && (unsigned)kind<3?story_slices[role][kind].count:0;
}
const story_item_t *story_item(unsigned role,story_kind_t kind,unsigned index){
    return index<story_count(role,kind)?&story_slices[role][kind].items[index]:NULL;
}

uint32_t story_revision(void){return story_content_revision;}
const story_item_t *story_missed_call(void){return &story_missed_jimmy;}
