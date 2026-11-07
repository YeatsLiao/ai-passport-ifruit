#include "audio.h"
#include "wav.h"
#include "bsp_audio.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
typedef struct { char path[96]; bool loop; uint32_t id; } command_t;
static QueueHandle_t commands;
static QueueHandle_t results;
static uint32_t sequence;
static void finished(uint32_t id) { xQueueOverwrite(results,&id); }
bool phone_audio_result(uint32_t *id) { return results && xQueueReceive(results,id,0)==pdTRUE; }
static portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
static char status[64]="Audio not initialized";
static void report(const char *s) {
    portENTER_CRITICAL(&mux); snprintf(status,sizeof(status),"%s",s); portEXIT_CRITICAL(&mux);
}
void phone_audio_status(char *out,size_t n) {
    portENTER_CRITICAL(&mux); snprintf(out,n,"%s",status); portEXIT_CRITICAL(&mux);
}
static void worker(void *arg) {
    (void)arg;
    const esp_vfs_spiffs_conf_t fs={.base_path="/assets",.partition_label="assets",.max_files=2,.format_if_mount_failed=false};
    esp_err_t mount=esp_vfs_spiffs_register(&fs);
    if(mount!=ESP_OK)ESP_LOGW("phone_audio","Assets mount failed (%s). Flash the assets image at 0x310000; continuing without audio",esp_err_to_name(mount));
    else ESP_LOGI("phone_audio","Assets mounted at /assets");
    esp_err_t codec=ESP_ERR_INVALID_STATE; /* Initialize lazily once actual audio exists. */
    bool initialized=false;
    command_t cmd; FILE *file=NULL; wav_t wav={0}; uint32_t left=0;
    int16_t pcm[256];
    report(mount==ESP_OK ? "Audio ready" : "Assets not installed");
    for(;;) {
        if(xQueueReceive(commands,&cmd,file ? 0 : portMAX_DELAY)==pdTRUE) {
            if(file) { fclose(file); file=NULL; }
            if(!cmd.path[0]) { report("Stopped"); continue; }
            if(mount!=ESP_OK) { report("Assets not installed"); finished(cmd.id); continue; }
            file=fopen(cmd.path,"rb");
            if(!file) { report("Voice / sound not installed"); finished(cmd.id); continue; }
            if(!wav_open(file,&wav)) { fclose(file); file=NULL; report("Invalid device WAV"); finished(cmd.id); continue; }
            if(!initialized) { codec=bsp_audio_init(); initialized=true; }
            if(codec!=ESP_OK || bsp_audio_set_format(wav.rate,16,1)!=ESP_OK) {
                fclose(file); file=NULL; report("Audio hardware error"); finished(cmd.id); continue;
            }
            bsp_audio_set_volume(35); left=wav.bytes; report("Playing");
        }
        if(!file) continue;
        size_t n=left<sizeof(pcm) ? left : sizeof(pcm);
        if(fread(pcm,1,n,file)!=n || bsp_audio_write(pcm,n)!=ESP_OK) {
            fclose(file); file=NULL; report("Audio read / output error"); finished(cmd.id); continue;
        }
        left-=(uint32_t)n;
        if(!left) {
            if(cmd.loop && fseek(file,wav.offset,SEEK_SET)==0) left=wav.bytes;
            else { fclose(file); file=NULL; report("Finished"); finished(cmd.id); }
        }
        taskYIELD();
    }
}
esp_err_t phone_audio_init(void) {
    commands=xQueueCreate(1,sizeof(command_t)); if(!commands) return ESP_ERR_NO_MEM;
    results=xQueueCreate(1,sizeof(uint32_t)); if(!results) {vQueueDelete(commands);commands=NULL;return ESP_ERR_NO_MEM;}
    if(xTaskCreate(worker,"phone_audio",4096,NULL,4,NULL)!=pdPASS) {
        vQueueDelete(results);results=NULL;vQueueDelete(commands); commands=NULL; return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
uint32_t phone_audio_play(const char *path,bool loop) {
    if(!commands) return 0;
    command_t cmd={.loop=loop};
    if(path && strlen(path)>=sizeof(cmd.path)) { report("Audio path too long"); return 0; }
    if(path) snprintf(cmd.path,sizeof(cmd.path),"%s",path);
    if(++sequence==0)++sequence;
    cmd.id=sequence;
    xQueueOverwrite(commands,&cmd);
    return cmd.id;
}
