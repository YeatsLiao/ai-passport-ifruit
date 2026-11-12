#include "gsm.h"
#include "private.h"
#include <stdio.h>
#include <stdint.h>
_Static_assert(sizeof(longword)==4,"GSM requires 32-bit longword");
int main(int argc,char **argv){
 if(argc!=3)return 2;FILE *in=fopen(argv[1],"rb"),*out=fopen(argv[2],"wb");if(!in||!out)return 3;
 gsm state=gsm_create();if(!state)return 4;int on=1;if(gsm_option(state,GSM_OPT_WAV49,&on)<0)return 5;
 unsigned char block[65];gsm_signal pcm[320];size_t n;unsigned count=0;
 while((n=fread(block,1,65,in))==65){if(gsm_decode(state,block,pcm)||gsm_decode(state,block+33,pcm+160))return 6;if(fwrite(pcm,2,320,out)!=320)return 7;count++;}
 gsm_destroy(state);fclose(in);fclose(out);if(n)return 8;
 printf("Decoded %u WAV49 blocks; state bytes=%zu\n",count,sizeof(struct gsm_state));return 0;
}
