#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
esp_err_t phone_audio_init(void);
/* Latest command wins. NULL path means stop. Paths must remain valid only during call. */
uint32_t phone_audio_play(const char *path, bool loop);
/* Main task consumes completion IDs; cancelled/replaced requests do not complete. */
bool phone_audio_result(uint32_t *request_id);
void phone_audio_status(char *out, size_t capacity);
