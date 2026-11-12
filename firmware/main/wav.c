#include "wav.h"
#include "gsm.h"
#include <string.h>
#include <limits.h>
static unsigned u16(const unsigned char *b){return b[0]|(unsigned)b[1]<<8;}
static uint32_t u32(const unsigned char *b){return (uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
void wav_close(wav_t *w){if(w->decoder)gsm_destroy((gsm)w->decoder);w->decoder=NULL;}
bool wav_rewind(FILE *f,wav_t *w){
 if(fseek(f,w->offset,SEEK_SET))return false;
 w->samples_left=w->samples;w->bytes_left=w->bytes;w->pos=w->count=0;
 if(w->codec==49){wav_close(w);w->decoder=gsm_create();int on=1;if(!w->decoder)return false;gsm_option((gsm)w->decoder,GSM_OPT_WAV49,&on);}
 return true;
}
bool wav_open(FILE *f,wav_t *w){
 unsigned char h[20];bool fmt=false,data=false,fact=false;
 if(!f||!w||fseek(f,0,SEEK_END))return false;
 memset(w,0,sizeof(*w));long size=ftell(f);if(size<12||fseek(f,0,SEEK_SET))return false;
 if(fread(h,1,12,f)!=12||memcmp(h,"RIFF",4)||memcmp(h+8,"WAVE",4))return false;
 uint64_t end=(uint64_t)u32(h+4)+8,pos=12;if(end>(uint64_t)size||end<12)return false;
 while(pos+8<=end){
  if(pos>LONG_MAX||fseek(f,(long)pos,SEEK_SET)||fread(h,1,8,f)!=8)return false;
  uint32_t n=u32(h+4);uint64_t next=pos+8+n+(n&1);if(next>end)return false;
  if(!memcmp(h,"fmt ",4)){
   if(fmt||n<16||fread(h,1,16,f)!=16||u16(h+2)!=1)return false;
   w->codec=(uint16_t)u16(h);w->rate=u32(h+4);w->block_align=(uint16_t)u16(h+12);
   if(w->codec==1){if(w->rate!=16000||u32(h+8)!=32000||w->block_align!=2||u16(h+14)!=16)return false;}
   else if(w->codec==49){if(w->rate!=8000||w->block_align!=65||n<20||fread(h,1,4,f)!=4||u16(h)<2||u16(h+2)!=320)return false;}
   else return false;
   fmt=true;
  }else if(!memcmp(h,"fact",4)){if(fact||n<4||fread(h,1,4,f)!=4)return false;w->samples=u32(h);fact=true;}
  else if(!memcmp(h,"data",4)){if(data||!n||pos+8>UINT32_MAX)return false;w->offset=(uint32_t)(pos+8);w->bytes=n;data=true;}
  pos=next;
 }
 if(!fmt||!data||pos!=end)return false;
 if(w->codec==1){if(w->bytes&1)return false;w->samples=w->bytes/2;}
 else {uint64_t max=(uint64_t)(w->bytes/65)*320;if(!fact||!w->samples||w->bytes%65||w->samples>max||w->samples<=max-320)return false;}
 return wav_rewind(f,w);
}
int wav_read_pcm(FILE *f,wav_t *w,int16_t *out,unsigned cap){
 if(!f||!w||!out||!cap)return -1;
 unsigned n=cap<w->samples_left?cap:w->samples_left;
 if(w->codec==1){if(fread(out,2,n,f)!=n)return -1;w->samples_left-=n;return (int)n;}
 if(w->codec!=49||!w->decoder)return -1;
 for(unsigned i=0;i<n;i++){
  if(w->pos==w->count){unsigned char block[65];if(w->bytes_left<65||fread(block,1,65,f)!=65)return -1;w->bytes_left-=65;
   if(gsm_decode((gsm)w->decoder,block,w->pcm)||gsm_decode((gsm)w->decoder,block+33,w->pcm+160))return -1;
   w->pos=0;w->count=320;}
  out[i]=w->pcm[w->pos++];
 }
 w->samples_left-=n;return (int)n;
}
