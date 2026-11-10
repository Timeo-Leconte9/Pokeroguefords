#include "game.h"
void host_dump(const char *path);
int main(void){
    int d, page;
    gfx_init();
    for (page=0; page<2; page++){
        gfx_target(0); gfx_clear(COL(120,170,120)); gfx_target(1); gfx_clear(COL(120,170,120));
        for (d=0; d<76; d++){
            int dex = page*76 + d + 1; int scr = d/38; int i = d%38;
            if (dex>151) break;
            gfx_target(scr);
            sprite_draw_icon(dex, (i%8)*32, (i/8)*32);
        }
        gfx_present();
        host_dump(page? "host/out/sheet2.ppm":"host/out/sheet1.ppm");
    }
    return 0;
}
