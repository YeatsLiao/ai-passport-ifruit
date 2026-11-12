#pragma once
#include "lvgl.h"

/* Call once, after app startup, with the LVGL lock held. */
bool phone_screenshot_init(lv_display_t *display);
