/* PokéRogue DS - point d'entrée */
#include "game.h"
#include <fat.h>

void game_init_fat(void);

int main(void)
{
    gfx_init();
    game_init_fat();
    audio_init();
    rng_seed(0x13579BDFu);
    memset(&run, 0, sizeof(run));
    game_main_loop();
    return 0;
}
