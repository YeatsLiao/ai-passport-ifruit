#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
typedef struct {uint32_t offset,bytes,rate,samples,samples_left,bytes_left;uint16_t codec,block_align,pos,count;void *decoder;int16_t pcm[320];} wav_t;
bool wav_open(FILE *,wav_t *);
bool wav_rewind(FILE *,wav_t *);
void wav_close(wav_t *);
int wav_read_pcm(FILE *,wav_t *,int16_t *,unsigned);
