/* Bot de test : joue automatiquement (touche / A) pour vérifier qu'une partie se déroule sans plantage. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

extern u32 host_keys_down, host_keys_held;
extern int host_touch_x, host_touch_y;
extern void (*host_frame_hook)(void);
extern unsigned long host_vblanks;
void host_dump(const char *path);

static int cad;           /* cadence : appui 2 frames, repos 3 */
static int starterStep;
static int shots[32];
static int maxFrames = 400000;
static int stopWave = 12;
static int lastWave;
static int battlesLogged;

static void shot(int id, const char *name)
{
    char p[96];
    if (shots[id]) return;
    shots[id] = 1;
    snprintf(p, sizeof(p), "host/out/shot_%s.ppm", name);
    host_dump(p);
}

static void press_touch(int x, int y) { host_touch_x = x; host_touch_y = y; host_keys_held |= KEY_TOUCH; }

static void bot_hook(void)
{
    cad++;
    host_keys_held = 0;
    if (host_vblanks > (unsigned long)maxFrames) { printf("BOT: limite de frames atteinte, vague %d\n", run.wave); exit(0); }
    if (run.wave != lastWave) { if (getenv("BOT_TRACE")) fprintf(stderr, "W%lu wave=%d lvl=%d party=%d\n", host_vblanks, run.wave, run.partyCount ? run.party[0].level : 0, run.partyCount); lastWave = run.wave; if (run.wave >= stopWave) { printf("BOT: vague %d atteinte (frames=%lu, equipe=%d)\n", run.wave, host_vblanks, run.partyCount); exit(0); } }
    if (getenv("BOT_TRACE") && host_vblanks % 20000 == 0 && cad % 5 == 0) fprintf(stderr, "T%lu wave=%d scr=%d mode=%d wait=%d title='%s' msg='%.50s' foe=%d\n", host_vblanks, run.wave, uiScreen, bottom.mode, uiWaitingChoice, bottom.title, scene.msg, scene.foe.visible);
    if (cad % 5 > 1) return;
    if (uiScreen != 1) starterStep = 0;
    if (uiScreen == 1) {
        shot(0, "starter");
        switch (starterStep++) {
        case 0: host_keys_held = KEY_A; break;
        case 1: host_keys_held = KEY_RIGHT; break;
        case 2: host_keys_held = KEY_RIGHT; break;
        case 3: host_keys_held = KEY_RIGHT; break;
        case 4: host_keys_held = KEY_A; break;
        case 5: host_keys_held = KEY_DOWN; break;
        case 6: host_keys_held = KEY_A; break;
        default: host_keys_held = KEY_START; break;
        }
        return;
    }
    if (uiWaitingChoice) {
        int pick = bottom.sel, i, x, y, w, h;
        if (bottom.mode == BM_MENU) {
            shot(1, "menu");
            pick = 0;
            if (rand() % 12 == 0 && run.balls[0] > 0) pick = 1;
        } else if (bottom.mode == BM_MOVES) {
            int best = -1, bp = -1;
            shot(2, "moves");
            fprintf(stderr, "MOVES gimShow=%d av=%d%d%d\n", bottom.gimShow, bottom.gimAvail[0], bottom.gimAvail[1], bottom.gimAvail[2]);
            for (i = 0; i < bottom.count; i++)
                if (bottom.enabled[i]) {
                    int pw = moves[bottom.moveId[i]].power;
                    if (pw > bp) { bp = pw; best = i; }
                }
            pick = best >= 0 ? best : 0;
            if (bottom.gimShow) {
                int g = rand() % 3, t;
                for (t = 0; t < 3 && !bottom.gimAvail[g]; t++) g = (g + 1) % 3;
                if (rand() % 2 && bottom.gimAvail[g]) { bottom.gimSel = g; shot(9 + g * 0 + 3 + g, g == 0 ? "mega" : g == 1 ? "dyna" : "tera"); }
            }
        } else if (bottom.mode == BM_LIST) {
            if (strstr(bottom.title, "compens")) { static int n; shot(3, "reward"); pick = (++n % 3 == 0) ? bottom.count - 1 : rand() % 3; if (n % 7 == 1) pick = bottom.count - 2; }
            else if (strstr(bottom.title, "Boutique")) { shot(15, "shop"); pick = -1; }
            else if (strstr(bottom.title, "Choisis ta dest")) { shot(4, "biome"); pick = rand() % 3; }
            else if (strstr(bottom.title, "Sur quel")) { shot(5, "target"); { static int tries; int any = -1; for (i = 0; i < bottom.count; i++) if (bottom.enabled[i]) { any = i; break; } tries++; pick = (any < 0 || tries % 4 == 0) ? -1 : any; } }
            else if (strstr(bottom.title, "Oublier")) pick = bottom.count - 1;
            else if (strstr(bottom.title, "POKEROGUE")) { shot(6, "title"); pick = 0; for (i = 0; i < bottom.count; i++) if (strcmp(bottom.label[i], "Nouvelle partie") == 0) pick = i; }
            else if (strstr(bottom.title, "Envoyer")) { for (i = 0; i < bottom.count; i++) if (bottom.enabled[i]) { pick = i; break; } }
            else if (strstr(bottom.title, "Remplacer")) pick = -1;
            else if (bottom.sel < 0 || !bottom.enabled[bottom.sel]) pick = 0;
        }
        if (getenv("BOT_TRACE") && strstr(bottom.title, "Sur quel")) fprintf(stderr, "V%lu pick=%d sel=%d\n", host_vblanks, pick, bottom.sel);
        if (pick < 0) { host_keys_held = KEY_B; return; }
        ui_btn_rect(pick, &x, &y, &w, &h);
        press_touch(x + w / 2, y + h / 2);
        return;
    }
    { static int force = -1; if (force < 0) { const char *e = getenv("BOT_FORCE"); force = e ? atoi(e) : 0; }
      if (force > 0 && run.partyCount > 0 && run.party[0].species != force && !scene.foe.visible) {
          run.party[0].species = (u8)force; run.party[0].level = 40; run.party[0].exp = mon_exp_for(40, force); mon_recalc(&run.party[0]); run.party[0].hp = run.party[0].stat[0]; } }
    shot(7, "msg");
    if (scene.foe.visible && scene.ally.visible && battlesLogged == 0 && scene.msgLen >= scene.msgFull) { battlesLogged = 1; shot(8, "battle"); }
    if (scene.fx) shot(9, "fx");
    if (scene.ally.gim && scene.ally.visible && scene.msgLen >= scene.msgFull) shot(12 + scene.ally.gim, scene.ally.gim == 1 ? "g_mega" : scene.ally.gim == 2 ? "g_dyna" : "g_tera");
    host_keys_held = KEY_A;
}

__attribute__((constructor)) static void bot_install(void)
{
    const char *e = getenv("BOT_WAVE");
    if (e) stopWave = atoi(e);
    e = getenv("BOT_FRAMES");
    if (e) maxFrames = atoi(e);
    e = getenv("BOT_SPEED"); if (e) gameSpeed = atoi(e);
    e = getenv("BOT_SEED");
    srand(e ? (unsigned)atoi(e) : 1);
    host_frame_hook = bot_hook;
}
