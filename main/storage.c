#include "storage.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
typedef struct { uint32_t version; uint32_t values[SET_COUNT]; } preferences_t;
static bool ready;
static preferences_t saved;
static bool have_saved;
void phone_storage_load(phone_t *phone) {
    esp_err_t err=nvs_flash_init();
    if(err!=ESP_OK) {
        /* Never erase NVS to repair a failed initialization. */
        ESP_LOGW("phone","Preferences unavailable: %s",esp_err_to_name(err));return;
    }
    ready=true;
    nvs_handle_t world_handle;
    if(nvs_open("ifruit",NVS_READONLY,&world_handle)==ESP_OK){
        struct {uint32_t version;world_t world;} data={0};size_t n=sizeof(data);
        if(nvs_get_blob(world_handle,"world_v1",&data,&n)==ESP_OK && n==sizeof(data) && data.version==1 && world_valid(&data.world))phone->world=data.world;
        nvs_close(world_handle);
    }
    nvs_handle_t handle;
    err=nvs_open("ifruit",NVS_READONLY,&handle);
    if(err==ESP_ERR_NVS_NOT_FOUND)return;
    if(err!=ESP_OK){ESP_LOGW("phone","Preferences open failed: %s",esp_err_to_name(err));return;}
    preferences_t data={0};size_t size=sizeof(data);
    err=nvs_get_blob(handle,"settings_v1",&data,&size);nvs_close(handle);
    if(err==ESP_ERR_NVS_NOT_FOUND)return;
    if(err!=ESP_OK || size!=sizeof(data) || data.version!=1 || !phone_restore_settings(phone,data.values)) {
        ESP_LOGW("phone","Invalid preferences; using defaults");return;
    }
    saved=data;have_saved=true;
}
void phone_storage_save(const phone_t *phone) {
    if(!ready)return;
    preferences_t data={.version=1};
    for(unsigned i=0;i<SET_COUNT;i++)data.values[i]=phone->settings[i];
    bool same=have_saved && memcmp(&data,&saved,sizeof(data))==0;
    nvs_handle_t handle;esp_err_t err=nvs_open("ifruit",NVS_READWRITE,&handle);
    if(err!=ESP_OK){ESP_LOGW("phone","Preferences save open failed: %s",esp_err_to_name(err));return;}
    err=same?ESP_OK:nvs_set_blob(handle,"settings_v1",&data,sizeof(data));
    struct {uint32_t version;world_t world;} world_data={.version=1,.world=phone->world};
    if(err==ESP_OK)err=nvs_set_blob(handle,"world_v1",&world_data,sizeof(world_data));
    if(err==ESP_OK)err=nvs_commit(handle);
    nvs_close(handle);
    if(err==ESP_OK){saved=data;have_saved=true;}
    else ESP_LOGW("phone","Preferences not saved: %s",esp_err_to_name(err));
}
