#include "bsp_display.h"
#include "bsp_button.h"
#include "ui.h"
#include "audio.h"
#include "storage.h"
#include "screenshot.h"
#include <stdio.h>
#include <string.h>
#include "esp_random.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
static QueueHandle_t keys;
static uint32_t voice_request;
static void phone_effect_audio(const phone_t *phone,const char *name) {
    char path[64];
    if(phone->world.role)snprintf(path,sizeof(path),"/assets/role_%u_%s.wav",(unsigned)phone->world.role,name);
    else snprintf(path,sizeof(path),"/assets/%s.wav",name);
    phone_audio_play(path,false);
}
static void effect(phone_fx_t fx,phone_t *phone) {
    if(fx==FX_NONE)return;
    voice_request=0;
    unsigned volume=phone->settings[SET_VOLUME],brightness=phone->settings[SET_BRIGHTNESS];
    if(fx==FX_HW_PREVIEW){if(phone->selected_setting==SET_VOLUME)volume=phone->selected_option;else brightness=phone->selected_option;}
    phone_audio_set_volume(volume*10);bsp_display_backlight((brightness+1)*10);
    if(fx==FX_NAV || fx==FX_HW_PREVIEW) phone_effect_audio(phone,"menu_navigate");
    else if(fx==FX_ACCEPT) phone_effect_audio(phone,"menu_accept");
    else if(fx==FX_BACK || fx==FX_STOP) phone_effect_audio(phone,"menu_back");
    else if(fx==FX_DIAL) phone_audio_play("/assets/dial.wav",false); /* Original DAT10 offline rendition; local setup delay. */
    else if(fx==FX_TEXT) phone_effect_audio(phone,"text_arrive_tone");
    else if(fx==FX_RING || fx==FX_RING_PREVIEW){
        unsigned selected=fx==FX_RING?phone->settings[SET_RINGTONE]:phone->selected_option;
        phone_audio_play(phone_ring_file(selected,phone->world.role),fx==FX_RING);
    }
    else if(fx==FX_SAVE)phone_save_result(phone,phone_storage_save(phone));
    else if(fx==FX_SHUTTER)phone_effect_audio(phone,"camera_shoot");
    else if(fx==FX_SCENE){const call_scene_t *scene=call_scene_get((unsigned)phone->scene_id);if(scene)voice_request=phone_audio_play(scene->device_file,false);}
    else if(fx==FX_VOICE) {
        const phone_contact_t *contact=phone_current_contact(phone);
        if(contact){
            const char *file=contact->native_id==PHONE_TAXI_CONTACT?phone_taxi_voice_file(phone->world.role):contact->answerphone_file;
            if(file)voice_request=phone_audio_play(file,false);
        }
    }
}
static void on_key(bsp_btn_t button,bsp_btn_ev_t event,void *user) {
    (void)user;
    phone_key_t key;
    if(event==BSP_BTN_DOUBLE)key=button==BSP_BTN_OK?KEY_DEMO:button==BSP_BTN_UP?KEY_LEFT:KEY_RIGHT;
    else if(event==BSP_BTN_LONG && button==BSP_BTN_UP) key=KEY_DIAL_PAD;
    else if(event==BSP_BTN_LONG && button==BSP_BTN_OK) key=KEY_BACK;
    else if(event==BSP_BTN_CLICK) key=button==BSP_BTN_UP ? KEY_UP : button==BSP_BTN_DOWN ? KEY_DOWN : KEY_OK;
    else return;
    /* Callback only enqueues; main task is sole owner of phone state. */
    if(xQueueSend(keys,&key,0)!=pdTRUE) ESP_LOGW("phone","Key queue full");
}
void app_main(void) {
    keys=xQueueCreate(16,sizeof(phone_key_t)); configASSERT(keys);
    ESP_ERROR_CHECK(phone_audio_init());
    ESP_ERROR_CHECK(bsp_display_init());
    /* Initialization must execute even in builds with assertions disabled. */
    if(!bsp_lvgl_init())ESP_ERROR_CHECK(ESP_FAIL);
    static phone_t phone; phone_init(&phone);
    phone_storage_load(&phone);
    if(phone.world.rng==1){phone.world.rng=esp_random();if(!phone.world.rng)phone.world.rng=1;}
    phone.selected_role=phone.world.role;phone.page=PHONE_ROLE;
    if(!bsp_lvgl_lock(0))ESP_ERROR_CHECK(ESP_ERR_TIMEOUT);
    phone_ui_init();phone_ui_render(&phone);if(!phone_screenshot_init(lv_display_get_default()))ESP_LOGW("phone","Screenshot USB unavailable");bsp_lvgl_unlock();
    /* BSP deliberately starts LEDC at duty=0. Panel DISPON alone does not light it. */
    bsp_display_backlight((phone.settings[SET_BRIGHTNESS]+1)*10);
    phone_audio_set_volume(phone.settings[SET_VOLUME]*10);
    ESP_LOGI("phone","UI created; saved brightness applied; free heap=%lu",(unsigned long)esp_get_free_heap_size());
    ESP_ERROR_CHECK(bsp_button_init(on_key,NULL));
    TickType_t step=xTaskGetTickCount();
    for(;;) {
        world_t old_world=phone.world;sim_t old_sim=phone.sim;
        uint64_t old_missed=phone.missed_calls;uint32_t old_egg=phone.egg_seen,old_comm=phone.comm.revision;
        uint64_t old_read[3][2];uint32_t old_photos[3],old_snaps[3];
        memcpy(old_read,phone.read_bits,sizeof(old_read));memcpy(old_photos,phone.photo_bits,sizeof(old_photos));memcpy(old_snaps,phone.snap_bits,sizeof(old_snaps));
        phone_key_t key; bool dirty=false;
        if(xQueueReceive(keys,&key,pdMS_TO_TICKS(50))==pdTRUE) {
            bool applying=(phone.page==PHONE_SETTING_OPTIONS || phone.page==PHONE_PHOTO) && (key==KEY_OK || key==KEY_DEMO);
            phone_fx_t fx;
            /* Keep both inputs when rapidly entering a repeated digit or moving focus. */
            bool repeat_digit=key==KEY_DEMO && phone.page==PHONE_NUMBERPAD && phone.selected_digit!=PHONE_KEYPAD_CALL;
            if(key==KEY_LEFT || key==KEY_RIGHT || repeat_digit){
                phone_key_t repeated=repeat_digit?KEY_OK:key==KEY_LEFT?KEY_UP:KEY_DOWN;
                fx=phone_key(&phone,repeated);
                phone_fx_t second=phone_key(&phone,repeated);
                if(second!=FX_NONE)fx=second;
            }
            else fx=key==KEY_DEMO ? (phone.page==PHONE_HOME?phone_demo_call(&phone):phone_key(&phone,KEY_OK)) : phone_key(&phone,key);
            effect(fx,&phone);
            if(applying)phone_storage_save(&phone);
            dirty=true;
        }
        TickType_t now=xTaskGetTickCount();
        phone_page_t before=phone.page;
        uint32_t previous_seconds=phone.call_seconds,previous_clock=phone.clock_minutes;
        phone_fx_t advance_fx=phone_advance(&phone,(uint32_t)((now-step)*portTICK_PERIOD_MS));effect(advance_fx,&phone);step=now;
        if(advance_fx!=FX_NONE)dirty=true;
        if(before!=phone.page || previous_seconds!=phone.call_seconds || previous_clock!=phone.clock_minutes)dirty=true;
        uint32_t completed;
        if(phone_audio_result(&completed) && voice_request && completed==voice_request) {
            effect(phone_audio_finished(&phone),&phone);dirty=true;
        }
        /* Persist semantic progress, not every millisecond of the countdown. */
        if(old_comm!=phone.comm.revision)dirty=true;
        if(memcmp(old_snaps,phone.snap_bits,sizeof(old_snaps)) || old_comm!=phone.comm.revision || old_egg!=phone.egg_seen || memcmp(&old_sim,&phone.sim,sizeof(old_sim)) || old_world.role!=phone.world.role || old_missed!=phone.missed_calls || memcmp(old_world.active,phone.world.active,sizeof(old_world.active)) || memcmp(old_world.done,phone.world.done,sizeof(old_world.done)) || memcmp(old_read,phone.read_bits,sizeof(old_read)) || memcmp(old_photos,phone.photo_bits,sizeof(old_photos)))phone_storage_save(&phone);
        if(dirty && bsp_lvgl_lock(100)) { phone_ui_render(&phone); bsp_lvgl_unlock(); }
    }
}
