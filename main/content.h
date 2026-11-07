#pragma once
#include <stdbool.h>
#include <stdint.h>
enum { SET_BACKGROUND, SET_PROFILE, SET_RINGTONE, SET_THEME, SET_VIBRATE, SET_COUNT };
extern const char *const setting_names[SET_COUNT];
extern const char *const setting_options[SET_COUNT][15];
extern const unsigned setting_counts[SET_COUNT];
extern const uint32_t theme_colors[7][3];
extern const bool contact_has_voice[6];
