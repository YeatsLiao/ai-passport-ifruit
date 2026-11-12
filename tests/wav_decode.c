#include "wav.h"
int main(int n,char **v){if(n!=3)return 1;FILE *f=fopen(v[1],"rb"),*o=fopen(v[2],"wb");wav_t w={0};if(!f||!o||!wav_open(f,&w))return 2;int16_t p[137];int k;while((k=wav_read_pcm(f,&w,p,137))>0)fwrite(p,2,(size_t)k,o);wav_close(&w);fclose(f);fclose(o);return k<0?3:0;}
