#include "wav.h"
#include <string.h>
#include <limits.h>
static uint32_t u32(const unsigned char *b) { return (uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24; }
static unsigned u16(const unsigned char *b) { return b[0]|(unsigned)b[1]<<8; }
bool wav_open(FILE *f, wav_t *w) {
    unsigned char h[16]; bool format=false, data=false;
    if (!f || !w || fseek(f,0,SEEK_END)) return false;
    long size=ftell(f); if(size<12 || fseek(f,0,SEEK_SET)) return false;
    if(fread(h,1,12,f)!=12 || memcmp(h,"RIFF",4) || memcmp(h+8,"WAVE",4)) return false;
    uint64_t end=(uint64_t)u32(h+4)+8;
    if(end>(uint64_t)size || end<12) return false;
    uint64_t pos=12;
    while(pos+8<=end) {
        if(pos>LONG_MAX || fseek(f,(long)pos,SEEK_SET) || fread(h,1,8,f)!=8) return false;
        uint32_t n=u32(h+4); uint64_t next=pos+8+n+(n&1);
        if(next>end) return false;
        if(!memcmp(h,"fmt ",4)) {
            if(n<16 || fread(h,1,16,f)!=16) return false;
            if(u16(h)!=1 || u16(h+2)!=1 || u32(h+4)!=16000 || u32(h+8)!=32000 || u16(h+12)!=2 || u16(h+14)!=16) return false;
            format=true;
        } else if(!memcmp(h,"data",4)) {
            if(data || !n || (n&1) || pos+8>UINT32_MAX) return false;
            w->offset=(uint32_t)(pos+8); w->bytes=n; w->rate=16000; data=true;
        }
        pos=next;
    }
    return format && data && pos==end && fseek(f,w->offset,SEEK_SET)==0;
}
