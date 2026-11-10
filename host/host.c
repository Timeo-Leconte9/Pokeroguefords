/* Runtime hôte : implémente le strict nécessaire de libnds pour tester sur PC. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nds.h>

u16 host_vram[2][256 * 256];

/* entrées pilotées par le test / le bot */
u32 host_keys_down, host_keys_held;
int host_touch_x, host_touch_y;
void (*host_frame_hook)(void) = 0;
unsigned long host_vblanks = 0;
int host_sound_events = 0;
int host_last_sample_size = 0;

static u32 prev_held, cur_held;

void swiWaitForVBlank(void)
{
    host_vblanks++;
    if (host_frame_hook) host_frame_hook();
}

void scanKeys(void)
{
    prev_held = cur_held;
    cur_held = host_keys_held;
}
u32 keysDown(void) { return cur_held & ~prev_held; }
u32 keysHeld(void) { return cur_held; }
void touchRead(touchPosition *t)
{
    t->px = (u16)host_touch_x;
    t->py = (u16)host_touch_y;
}

void soundEnable(void) {}
int soundPlaySample(const void *data, int fmt, u32 size, u16 freq, u8 vol, u8 pan, bool loop, u16 lp)
{
    (void)data; (void)fmt; (void)freq; (void)vol; (void)pan; (void)loop; (void)lp;
    host_sound_events++;
    host_last_sample_size = (int)size;
    return 1;
}
void soundKill(int h) { (void)h; }
void soundSetVolume(int h, u8 v) { (void)h; (void)v; }

/* capture : haut + bas empilés, PPM */
void host_dump(const char *path)
{
    FILE *f = fopen(path, "wb");
    int s, y, x;
    if (!f) return;
    fprintf(f, "P6\n256 384\n255\n");
    for (s = 0; s < 2; s++)
        for (y = 0; y < 192; y++)
            for (x = 0; x < 256; x++) {
                u16 c = host_vram[s][y * 256 + x];
                unsigned char rgb[3] = {(unsigned char)((c & 31) << 3), (unsigned char)(((c >> 5) & 31) << 3), (unsigned char)(((c >> 10) & 31) << 3)};
                fwrite(rgb, 1, 3, f);
            }
    fclose(f);
}
