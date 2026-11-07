#include "bsp_display.h"
#include "bsp_button.h"
#include "ui.h"
#include "audio.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>
#include "esp_random.h"
#define KEY_DEMO ((phone_key_t)5)
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
static QueueHandle_t keys;
static uint32_t voice_request;
static void effect(phone_fx_t fx,const phone_t *phone) {
    if(fx==FX_NONE)return;
    voice_request=0;
    if(fx==FX_NAV) phone_audio_play("/assets/menu_navigate.wav",false);
    else if(fx==FX_ACCEPT) phone_audio_play("/assets/menu_accept.wav",false);
    else if(fx==FX_BACK || fx==FX_STOP) phone_audio_play("/assets/menu_back.wav",false);
    else if(fx==FX_DIAL) phone_audio_play(NULL,false); /* Native synthesized dial tone remains unresolved. */
    else if(fx==FX_TEXT) phone_audio_play("/assets/text_arrive_tone.wav",false);
    else if(fx==FX_RING) phone_audio_play("/assets/ringtone.wav",true);
    else if(fx==FX_VOICE) {
        char path[96];snprintf(path,sizeof(path),"/assets/contact_%u.wav",phone->selected_contact);
        voice_request=phone_audio_play(path,false);
    }
}
static void on_key(bsp_btn_t button,bsp_btn_ev_t event,void *user) {
    (void)user;
    phone_key_t key;
    if(event==BSP_BTN_DOUBLE && button==BSP_BTN_OK) key=KEY_DEMO;
    else if(event==BSP_BTN_LONG && button==BSP_BTN_UP) key=KEY_ROLE;
    else if(event==BSP_BTN_LONG && button==BSP_BTN_OK) key=KEY_BACK;
    else if(event==BSP_BTN_CLICK) key=button==BSP_BTN_UP ? KEY_UP : button==BSP_BTN_DOWN ? KEY_DOWN : KEY_OK;
    else return;
    /* Callback only enqueues; main task is sole owner of phone state. */
    if(xQueueSend(keys,&key,0)!=pdTRUE) ESP_LOGW("phone","Key queue full");
}
void app_main(void) {
    keys=xQueueCreate(16,sizeof(phone_key_t)); configASSERT(keys);
    ESP_ERROR_CHECK(phone_audio_init());
    ESP_ERROR_CHECK(bsp_display_init()); configASSERT(bsp_lvgl_init());
    phone_t phone; phone_init(&phone);
    phone_storage_load(&phone);
    if(phone.world.rng==1){phone.world.rng=esp_random();if(!phone.world.rng)phone.world.rng=1;}
    phone.selected_role=phone.world.role;phone.page=PHONE_ROLE;
    if(bsp_lvgl_lock(1000)) { phone_ui_init(); phone_ui_render(&phone); bsp_lvgl_unlock(); }
    ESP_ERROR_CHECK(bsp_button_init(on_key,NULL));
    TickType_t step=xTaskGetTickCount();
    for(;;) {
        world_t old_world=phone.world;
        phone_key_t key; bool dirty=false;
        if(xQueueReceive(keys,&key,pdMS_TO_TICKS(50))==pdTRUE) {
            bool applying=phone.page==PHONE_SETTING_OPTIONS && key==KEY_OK;
            phone_fx_t fx=key==KEY_DEMO ? phone_incoming(&phone,1) : phone_key(&phone,key);
            effect(fx,&phone);
            if(applying)phone_storage_save(&phone);
            dirty=true;
        }
        TickType_t now=xTaskGetTickCount();
        phone_page_t before=phone.page;
        uint32_t previous_seconds=phone.call_seconds;
        effect(phone_advance(&phone,(uint32_t)((now-step)*portTICK_PERIOD_MS)),&phone);step=now;
        if(before!=phone.page || previous_seconds!=phone.call_seconds)dirty=true;
        uint32_t completed;
        if(phone_audio_result(&completed) && voice_request && completed==voice_request) {
            effect(phone_audio_finished(&phone),&phone);dirty=true;
        }
        /* Persist semantic progress, not every millisecond of the countdown. */
        if(old_world.role!=phone.world.role || memcmp(old_world.active,phone.world.active,sizeof(old_world.active)) || memcmp(old_world.done,phone.world.done,sizeof(old_world.done)))phone_storage_save(&phone);
        if(dirty && bsp_lvgl_lock(100)) { phone_ui_render(&phone); bsp_lvgl_unlock(); }
    }
}
