#include "game.h"
void host_dump(const char *path);
int main(void){
    gfx_init(); gfx_target(0); gfx_clear(COL(30,40,70));
    gfx_text(4,4,"Nidoran♀ Nidoran♂ ♥ Œuf œil",COL(255,255,255));
    gfx_text_big(4,20,"♀♂♥",COL(255,255,255),4);
    gfx_present();
    host_dump("host/out/font.ppm");
    return 0;
}
