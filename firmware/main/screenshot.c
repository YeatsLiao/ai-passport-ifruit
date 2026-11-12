/* USB commands: screenshot (checked text records), FAP_SCREENSHOT_V1 (official).
 * Capture LVGL's pixels BEFORE esp_lvgl_port swaps them for the SPI panel.
 * No full-frame allocation, panel readback, or managed-component edits.
 */
#include "screenshot.h"
#include "bsp_display.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_log.h"
#include "esp_rom_crc.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static lv_display_t *disp;
static bool capturing, failed, official;
static uint32_t pixels, crc;
static uint16_t capture_width,capture_height;
static int64_t started;

/* Keep every ring-buffer item small, and stop on a disconnected/stalled host.
 * Raw driver writes also avoid VFS CRLF conversion of binary pixel bytes. */
static bool send_bytes(const void *data, size_t size)
{
    const uint8_t *bytes = data;
    while (size) {
        size_t count = size > 256 ? 256 : size;
        if (usb_serial_jtag_write_bytes(bytes, count, pdMS_TO_TICKS(500)) != (int)count) return false;
        bytes += count;
        size -= count;
    }
    return true;
}

static int mute_log(const char *format, va_list args)
{
    (void)format;
    (void)args;
    return 0;
}

static void flush_start(lv_event_t *event)
{
    if (!capturing || failed) return;
    const lv_area_t *area = lv_event_get_param(event);
    lv_draw_buf_t *buf = lv_display_get_buf_active(disp);
    int width = area->x2 - area->x1 + 1;
    int height = area->y2 - area->y1 + 1;
    if (!buf || !buf->data || width <= 0 || width > 320 || height <= 0 ||
        area->x1 < 0 || area->x2 >= capture_width || area->y1 < 0 || area->y2 >= capture_height ||
        buf->header.stride < (unsigned)width * 2 ||
        (size_t)buf->header.stride * height > buf->data_size) {
        failed = true;
        return;
    }
    static const char hex[] = "0123456789abcdef";
    char encoded[1281];
    for (int row = 0; row < height; row++) {
        if (esp_timer_get_time() - started > 15000000) { failed = true; break; }
        const uint8_t *src = buf->data + row * buf->header.stride;
        if (official) {
            // A full-screen invalidate must produce full-width, ordered bands.
            // Do not silently flatten an unexpected partial/overlapping area.
            if (area->x1 != 0 || width != capture_width || pixels != (uint32_t)(area->y1 + row) * capture_width ||
                !send_bytes(src, width * 2)) { failed = true; break; }
            pixels += width;
            vTaskDelay(1);
            continue;
        }
        for (int i = 0; i < width * 2; i++) {
            encoded[i * 2] = hex[src[i] >> 4];
            encoded[i * 2 + 1] = hex[src[i] & 15];
        }
        encoded[width * 4] = 0;
        crc = esp_rom_crc32_le(crc, src, width * 2);
        pixels += width;
        char record[1344];
        int length = snprintf(record, sizeof(record), "@IFRUIT_ROW %" PRId32 " %" PRId32 " %d %s\n",
                              area->x1, (int32_t)(area->y1 + row), width, encoded);
        if (length < 0 || length >= (int)sizeof(record) || !send_bytes(record, length)) {
            failed = true;
            break;
        }
        vTaskDelay(1);
    }
}

static void capture(bool official_protocol)
{
    if (!bsp_lvgl_lock(1000)) { ESP_LOGW("screenshot", "Display busy"); return; }
    capture_width=lv_display_get_horizontal_resolution(disp);
    capture_height=lv_display_get_vertical_resolution(disp);
    if (lv_display_get_color_format(disp) != LV_COLOR_FORMAT_RGB565 ||
        !((capture_width==320&&capture_height==240)||(capture_width==240&&capture_height==320))) {
        ESP_LOGW("screenshot", "Unsupported display");
        bsp_lvgl_unlock();
        return;
    }
    pixels = crc = 0;
    failed = false;
    official = official_protocol;
    // Suppress ESP logs through the entire transfer, restoring the previous
    // sink afterwards without changing any component's configured log level.
    vprintf_like_t log_output = esp_log_set_vprintf(mute_log);
    fflush(stdout);
    if (usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(1000)) != ESP_OK) {
        esp_log_set_vprintf(log_output);
        bsp_lvgl_unlock();
        return;
    }
    started = esp_timer_get_time();
    char header[80];
    if(official)snprintf(header,sizeof(header),"FAP_SCREENSHOT_V1 %u %u RGB565LE 153600\n",(unsigned)capture_width,(unsigned)capture_height);
    else snprintf(header,sizeof(header),"@IFRUIT_BEGIN 1 %u %u RGB565LE\n",(unsigned)capture_width,(unsigned)capture_height);
    failed = !send_bytes(header, strlen(header));
    capturing = !failed;
    // Hold the LVGL lock through this redraw so animation/input cannot change
    // the scene between bands. Normal SPI flushing still runs unchanged.
    if (!failed) {
        lv_obj_invalidate(lv_screen_active());
        lv_refr_now(disp);
    }
    capturing = false;
    failed = failed || pixels != (uint32_t)capture_width * capture_height;
    if (!official) {
        char footer[80];
        int length = failed ? snprintf(footer, sizeof(footer), "@IFRUIT_ERROR incomplete_frame\n")
                            : snprintf(footer, sizeof(footer), "@IFRUIT_END %" PRIu32 " %08" PRIx32 "\n", pixels, crc);
        if (!send_bytes(footer, length)) failed = true;
    }
    if (usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(1000)) != ESP_OK) failed = true;
    esp_log_set_vprintf(log_output);
    bsp_lvgl_unlock();
    if (failed) ESP_LOGW("screenshot", "Transfer incomplete (%" PRIu32 " pixels)", pixels);
}

static void console_task(void *arg)
{
    (void)arg;
    char command[96];
    size_t used = 0;
    bool overflow=false;
    for (;;) {
        uint8_t data[64];
        int length = usb_serial_jtag_read_bytes(data, sizeof(data), pdMS_TO_TICKS(100));
        if (length <= 0) { vTaskDelay(pdMS_TO_TICKS(20)); continue; }
        for (int i = 0; i < length; i++) {
            int c = data[i];
            if (c == '\r' || c == '\n') {
                command[used]=0;
                used=0;overflow=false;continue;
            }
            if(overflow)continue;
            if (used == sizeof(command) - 1) {
                overflow=true;continue;
            }
            command[used++] = (char)c;
            command[used] = 0;
            // Official browser tools need not terminate their command with LF.
            if (strstr(command, "IFRUIT_SCREENSHOT_PING")) {
                static const char ready[] = "@IFRUIT_READY 2 FAP_SCREENSHOT_V1 screenshot\n";
                used = 0;
                send_bytes(ready, sizeof(ready) - 1);
            }
            else if (strstr(command, "FAP_SCREENSHOT_V1")) { used = 0; capture(true); }
            else if (strstr(command, "screenshot")) { used = 0; capture(false); }
        }
    }
}

bool phone_screenshot_init(lv_display_t *display)
{
    if (disp || !display) return false;
    if (!usb_serial_jtag_is_driver_installed()) {
        usb_serial_jtag_driver_config_t config = {.tx_buffer_size = 2048, .rx_buffer_size = 256};
        esp_err_t result = usb_serial_jtag_driver_install(&config);
        if (result != ESP_OK) {
            ESP_LOGE("screenshot", "USB driver init failed: %s", esp_err_to_name(result));
            return false;
        }
    }
    usb_serial_jtag_vfs_use_driver();
    disp = display;
    lv_display_add_event_cb(disp, flush_start, LV_EVENT_FLUSH_START, NULL);
    // This task calls lv_refr_now(), including image rendering, just like the
    // regular LVGL task. A 4KB serial-reader stack is not sufficient for that.
    if (xTaskCreate(console_task, "screenshot", 16384, NULL, 2, NULL) != pdPASS) {
        lv_display_remove_event_cb_with_user_data(disp, flush_start, NULL);
        disp = NULL;
        return false;
    }
    ESP_LOGI("screenshot", "USB ready: FAP_SCREENSHOT_V1 + screenshot, stack=16384");
    return true;
}
