#include "wav.h"
#include <assert.h>
#include <stdio.h>
int main(int argc,char **argv) {
    assert(argc==5);
    for(int n=1;n<argc;n++) {
        FILE *f=fopen(argv[n],"rb"); assert(f);
        wav_t wav={0}; bool ok=wav_open(f,&wav); fclose(f);
        assert(ok==(n==1));
        if(ok) { assert(wav.rate==16000); assert(wav.bytes==320); }
    }
    puts("PASS: device WAV parser accepts PCM16 and rejects wrong rate, truncation, invalid chunks");
}
