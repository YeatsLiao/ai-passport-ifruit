#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
typedef struct { uint32_t offset, bytes, rate; } wav_t;
/* Only device-ready 16kHz mono PCM16. Reject malformed/truncated chunks. */
bool wav_open(FILE *file, wav_t *wav);
