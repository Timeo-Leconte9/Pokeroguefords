#include "game.h"
#include <stdio.h>
s8 *audio_render_track_debug(int id,int *n,int *rate);
int main(void){
    for(int id=1; id<MUS_COUNT; id++){
        int n,r; s8*b=audio_render_track_debug(id,&n,&r);
        char p[64]; sprintf(p,"host/out/mus%d.raw",id);
        FILE*f=fopen(p,"wb"); fwrite(b,1,n,f); fclose(f);
        printf("%d %d %d\n",id,n,r);
    }
    return 0;
}
