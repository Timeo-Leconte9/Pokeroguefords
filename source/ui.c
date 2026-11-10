/* ui.c : rendu de la scène de combat (écran du haut), menus tactiles (écran du bas), messages, animations */
#include "game.h"
#include <stdarg.h>

Scene scene;
Bottom bottom;
int uiWaitingChoice;

static void (*topCustom)(void);
static void (*botCustom)(void);
static int fadeLevel;
static const Mon *allyMon, *foeMon;
static u16 bgCache[SCR_W * SCR_H];
static int bgBiome = -1;

#define WHITE COL(255, 255, 255)
#define BLACK COL(16, 16, 24)
#define NAVY COL(24, 32, 64)

void ui_set_custom(void (*top)(void), void (*bot)(void))
{
    topCustom = top;
    botCustom = bot;
}

/* ------------------------------------------------------------ décors */
typedef struct { u16 skyT, skyB, gndT, gndB, plat; } Look;
static const Look looks[BIOME_COUNT] = {
    {COL(110, 180, 250), COL(200, 235, 250), COL(120, 200, 95), COL(70, 150, 70), COL(90, 170, 80)},     /* Plaine */
    {COL(90, 150, 120), COL(170, 215, 150), COL(70, 140, 65), COL(35, 90, 45), COL(55, 115, 55)},        /* Forêt */
    {COL(120, 200, 250), COL(250, 235, 205), COL(240, 222, 165), COL(205, 185, 125), COL(225, 205, 145)},/* Plage */
    {COL(60, 110, 200), COL(130, 185, 235), COL(50, 120, 200), COL(20, 60, 140), COL(70, 140, 215)},     /* Mer */
    {COL(130, 160, 215), COL(215, 225, 245), COL(155, 145, 135), COL(105, 98, 92), COL(135, 125, 118)},  /* Montagne */
    {COL(28, 24, 44), COL(62, 52, 84), COL(88, 78, 98), COL(46, 42, 58), COL(70, 62, 82)},               /* Grotte */
    {COL(250, 195, 115), COL(255, 238, 175), COL(232, 202, 132), COL(200, 160, 92), COL(215, 182, 112)}, /* Désert */
    {COL(105, 125, 88), COL(165, 175, 112), COL(85, 105, 62), COL(48, 68, 44), COL(70, 90, 52)},         /* Marais */
    {COL(36, 28, 68), COL(94, 72, 124), COL(66, 76, 66), COL(34, 40, 40), COL(52, 60, 52)},              /* Cimetière */
    {COL(170, 200, 240), COL(242, 247, 255), COL(242, 247, 255), COL(198, 214, 236), COL(222, 232, 248)},/* Neige */
    {COL(88, 88, 110), COL(172, 172, 192), COL(122, 122, 132), COL(78, 78, 94), COL(104, 104, 116)},     /* Centrale */
    {COL(92, 28, 28), COL(236, 112, 50), COL(86, 54, 48), COL(48, 30, 30), COL(70, 44, 40)},             /* Volcan */
    {COL(184, 142, 100), COL(232, 202, 152), COL(205, 165, 112), COL(165, 122, 78), COL(190, 148, 98)},  /* Dojo */
    {COL(60, 100, 124), COL(152, 202, 202), COL(132, 152, 132), COL(90, 112, 96), COL(112, 135, 115)}    /* Ruines */
};

#define HORIZON 74

static void build_bg(int b)
{
    const Look *lk;
    int i;
    if (b < 0 || b >= BIOME_COUNT) b = 0;
    lk = &looks[b];
    gfx_target(0);
    gfx_gradient(0, HORIZON, lk->skyT, lk->skyB);
    gfx_gradient(HORIZON, SCR_H, lk->gndT, lk->gndB);
    switch (b) {
    case 0:
        for (i = 0; i < 3; i++) {
            int x = 30 + i * 85, y = 14 + (i * 13) % 20;
            gfx_ellipse(x, y, 16, 5, WHITE);
            gfx_ellipse(x + 10, y - 3, 10, 5, WHITE);
        }
        gfx_ellipse(40, HORIZON + 2, 60, 14, COL(100, 185, 90));
        gfx_ellipse(190, HORIZON + 3, 80, 12, COL(90, 170, 80));
        gfx_gradient(HORIZON + 4, SCR_H, lk->gndT, lk->gndB);
        break;
    case 1:
        for (i = 0; i < 12; i++) {
            int x = 8 + i * 22;
            gfx_rect(x - 2, HORIZON - 14, 5, 16, COL(90, 60, 40));
            gfx_ellipse(x, HORIZON - 20 - (i % 3) * 3, 12, 14, COL(40, 100 + (i % 3) * 12, 50));
        }
        break;
    case 2:
        gfx_rect(0, HORIZON - 14, SCR_W, 14, COL(70, 150, 225));
        for (i = 0; i < 10; i++) gfx_rect(i * 27 + 4, HORIZON - 9 + (i % 3) * 3, 14, 1, WHITE);
        gfx_ellipse(210, 24, 14, 14, COL(255, 240, 150));
        break;
    case 3:
        for (i = 0; i < 40; i++) gfx_rect((i * 37) % 250, HORIZON + 6 + (i * 19) % 110, 10, 1, COL(150, 200, 245));
        gfx_ellipse(40, 22, 22, 6, COL(230, 240, 250));
        break;
    case 4:
        for (i = 0; i < 4; i++) {
            int x = 20 + i * 70, h = 36 + (i * 17) % 22;
            gfx_rect(x, HORIZON, 1, 1, 0);
            { int k; for (k = 0; k < h; k++) gfx_rect(x - k * 3 / 2 - 1, HORIZON - k, k * 3 + 3, 1, k > h - 12 ? WHITE : COL(120, 125, 150)); }
        }
        break;
    case 5:
        for (i = 0; i < 14; i++) {
            int x = i * 19 + (i % 3) * 4, h = 10 + (i * 11) % 22, k;
            for (k = 0; k < h; k++) gfx_rect(x - (h - k) / 4, k, (h - k) / 2 + 1, 1, COL(52, 44, 70));
        }
        for (i = 0; i < 6; i++) {
            int x = 18 + i * 44 + (i % 2) * 10, k;
            for (k = 0; k < 8; k++) gfx_rect(x - k / 2, HORIZON - k, k + 1, 1, COL(120, 220, 240));
        }
        break;
    case 6:
        gfx_ellipse(60, 26, 16, 16, COL(255, 245, 190));
        gfx_ellipse(60, HORIZON + 6, 90, 14, COL(240, 212, 140));
        gfx_ellipse(200, HORIZON + 8, 90, 12, COL(228, 196, 124));
        gfx_gradient(HORIZON + 10, SCR_H, lk->gndT, lk->gndB);
        gfx_rect(218, 40, 6, 36, COL(70, 140, 70));
        gfx_rect(212, 52, 6, 3, COL(70, 140, 70));
        gfx_rect(212, 46, 3, 9, COL(70, 140, 70));
        break;
    case 7:
        for (i = 0; i < 7; i++) {
            int x = 10 + i * 40;
            gfx_rect(x, HORIZON - 30 - (i % 3) * 6, 3, 34 + (i % 3) * 6, COL(52, 44, 36));
            gfx_rect(x - 6, HORIZON - 22, 7, 2, COL(52, 44, 36));
            gfx_rect(x + 2, HORIZON - 14, 8, 2, COL(52, 44, 36));
        }
        gfx_tint(0, HORIZON - 16, SCR_W, 30, COL(190, 200, 150), 90);
        break;
    case 8:
        gfx_ellipse(200, 26, 14, 14, COL(250, 245, 200));
        gfx_ellipse(194, 23, 13, 13, COL(60, 46, 96));
        for (i = 0; i < 7; i++) {
            int x = 14 + i * 37, h = 12 + (i * 7) % 8;
            gfx_rect(x, HORIZON + 6 - h, 14, h, COL(110, 112, 120));
            gfx_ellipse(x + 7, HORIZON + 6 - h, 7, 4, COL(110, 112, 120));
        }
        break;
    case 9:
        for (i = 0; i < 8; i++) {
            int x = 14 + i * 33, k;
            for (k = 0; k < 24; k++) gfx_rect(x - k / 3, HORIZON - 26 + k, k * 2 / 3 + 1, 1, k < 8 ? WHITE : COL(60, 120, 80));
            gfx_rect(x - 1, HORIZON - 2, 3, 6, COL(90, 70, 50));
        }
        for (i = 0; i < 50; i++) gfx_rect((i * 53) % 254, (i * 29) % 170, 2, 2, WHITE);
        break;
    case 10:
        for (i = 0; i < 5; i++) {
            int x = 24 + i * 56;
            gfx_rect(x, HORIZON - 40, 3, 44, COL(70, 70, 82));
            gfx_rect(x - 10, HORIZON - 36, 23, 3, COL(70, 70, 82));
            gfx_rect(x - 8, HORIZON - 28, 19, 2, COL(70, 70, 82));
        }
        for (i = 0; i < 16; i++) gfx_rect(i * 16, HORIZON + 4, 8, 3, COL(250, 210, 50));
        break;
    case 11: {
        int k;
        for (k = 0; k < 50; k++) gfx_rect(110 - k * 2, HORIZON - 50 + k, k * 4 + 1, 1, k < 6 ? COL(255, 160, 60) : COL(60, 36, 34));
        for (i = 0; i < 6; i++) gfx_rect(20 + i * 40, HORIZON + 20 + (i * 17) % 90, 22, 2, COL(255, 140, 40));
        break;
    }
    case 12:
        for (i = 0; i < 5; i++) {
            gfx_rect(10 + i * 56, 0, 10, HORIZON + 4, COL(150, 40, 40));
            gfx_rect(8 + i * 56, 0, 14, 5, COL(220, 190, 90));
        }
        for (i = 0; i < 12; i++) gfx_rect(0, HORIZON + 6 + i * 10, SCR_W, 1, COL(150, 110, 70));
        break;
    case 13:
        for (i = 0; i < 6; i++) {
            int x = 12 + i * 42, h = 24 + (i * 13) % 26;
            gfx_rect(x, HORIZON - h, 9, h + 4, COL(170, 190, 180));
            gfx_rect(x - 2, HORIZON - h, 13, 3, COL(190, 210, 200));
            gfx_rect(x + 2, HORIZON - h + 8, 2, 8, COL(120, 150, 130));
        }
        break;
    default:
        break;
    }
    /* plateformes */
    gfx_ellipse(186, 90, 58, 10, gfx_darken(lk->plat, 70));
    gfx_ellipse(186, 88, 56, 9, lk->plat);
    gfx_ellipse(186, 87, 46, 6, gfx_lighten(lk->plat, 30));
    gfx_ellipse(62, 153, 72, 13, gfx_darken(lk->plat, 70));
    gfx_ellipse(62, 151, 70, 12, lk->plat);
    gfx_ellipse(62, 150, 58, 8, gfx_lighten(lk->plat, 30));
    gfx_save_to(bgCache);
    bgBiome = b;
}

/* ------------------------------------------------------------ éléments */
void ui_draw_hpbar(int x, int y, int w, int hp, int hpMax)
{
    int fill = hpMax > 0 ? (hp * (w - 2)) / hpMax : 0;
    u16 c;
    if (hp > 0 && fill < 1) fill = 1;
    c = (hp * 100 > hpMax * 50) ? COL(80, 220, 100) : (hp * 100 > hpMax * 20) ? COL(240, 200, 50) : COL(235, 70, 60);
    gfx_rect(x, y, w, 6, COL(30, 30, 40));
    gfx_rect(x + 1, y + 1, w - 2, 4, COL(80, 80, 96));
    gfx_rect(x + 1, y + 1, fill, 4, c);
    gfx_rect(x + 1, y + 1, fill, 1, gfx_lighten(c, 90));
}

void ui_status_tag(int x, int y, int st)
{
    static const char *const tags[] = {"", "BRU", "PAR", "PSN", "TOX", "SOM", "GEL", "CNF"};
    static const u16 cols[] = {0, COL(240, 128, 48), COL(220, 190, 40), COL(160, 64, 160), COL(130, 40, 140), COL(130, 130, 150), COL(100, 200, 220), COL(230, 110, 160)};
    if (st <= 0 || st > 7) return;
    gfx_rect(x, y, 20, 8, cols[st]);
    gfx_text(x + 1, y - 1, tags[st], WHITE);
}

void ui_type_badge(int x, int y, int type)
{
    char buf[8];
    const char *n;
    int k = 0, chars = 0;
    if (type < 0 || type >= TYPE_COUNT) return;
    gfx_rect(x, y, 22, 8, gfx_darken(typeColors[type], 40));
    gfx_rect(x, y, 22, 1, gfx_lighten(typeColors[type], 80));
    /* 2 premiers caractères UTF-8 du nom */
    for (n = typeNames[type]; *n && chars < 2 && k < 6; ) {
        buf[k++] = *n++;
        while (((unsigned char)*n & 0xC0) == 0x80 && k < 7) buf[k++] = *n++;
        chars++;
    }
    buf[k] = 0;
    gfx_text(x + 2, y - 1, buf, WHITE);
}

static void draw_box_info(const Actor *a, int x, int y, int isAlly)
{
    int w = isAlly ? 120 : 122, h = isAlly ? 48 : 40;
    u16 border = a->boss ? COL(240, 70, 70) : COL(225, 228, 240);
    gfx_box(x, y, w, h, NAVY, border);
    gfx_text_shadow(x + 6, y + 4, a->name, WHITE, BLACK);
    gfx_printf(x + w - 38, y + 4, COL(255, 235, 140), "Nv.%d", a->level);
    gfx_text(x + 6, y + 15, "PV", COL(250, 190, 70));
    ui_draw_hpbar(x + 20, y + 16, w - 28, a->hpShown, a->hpMax);
    if (a->boss) {
        int k;
        for (k = 1; k < 4; k++) gfx_rect(x + 20 + (w - 28) * k / 4, y + 16, 1, 6, COL(30, 30, 40));
    }
    if (isAlly) {
        gfx_printf(x + w - 6 - gfx_text_width("000/000") + 0, y + 24, WHITE, "%d/%d", a->hpShown, a->hpMax);
        gfx_rect(x + 6, y + 40, w - 12, 3, COL(40, 40, 60));
        gfx_rect(x + 6, y + 40, (w - 12) * a->expPct / 100, 3, COL(90, 160, 255));
        ui_type_badge(x + 6, y + 27, a->t1);
        if (a->t2 != NO_TYPE) ui_type_badge(x + 30, y + 27, a->t2);
        ui_status_tag(x + 56, y + 27, a->status);
    } else {
        ui_type_badge(x + 6, y + 27, a->t1);
        if (a->t2 != NO_TYPE) ui_type_badge(x + 30, y + 27, a->t2);
        ui_status_tag(x + 56, y + 27, a->status);
        if (a->shiny) gfx_text(x + w - 12, y + 26, "*", COL(255, 230, 80));
        if (a->boss) gfx_text(x + w - 36, y + 26, "BOSS", COL(255, 110, 110));
    }
}

/* effets visuels d'attaque */
static void draw_fx(void)
{
    static const int dxs[8] = {4, 3, 0, -3, -4, -3, 0, 3}, dys[8] = {0, 3, 4, 3, 0, -3, -4, -3};
    int t = scene.fxT, cx, cy, i;
    u16 c;
    if (!scene.fx) return;
    c = typeColors[scene.fx - 1];
    cx = scene.fxTarget ? 186 : 62;
    cy = scene.fxTarget ? 56 : 112;
    if (t < 6) gfx_ellipse(cx, cy, 6 + t * 4, 6 + t * 4, gfx_lighten(c, 130));
    for (i = 0; i < 8; i++) {
        int r = t * 2 + 4;
        int px = cx + dxs[i] * r / 4, py = cy + dys[i] * r / 4;
        gfx_rect(px - 2, py - 2, 5, 5, c);
        gfx_rect(px - 1, py - 1, 3, 3, gfx_lighten(c, 140));
    }
    if (t > 4) gfx_tint(cx - 34, cy - 30, 68, 60, c, 60);
}

static void draw_ball_icon(int x, int y, int r)
{
    gfx_ellipse(x, y, r, r, COL(220, 50, 50));
    gfx_rect(x - r, y, r * 2 + 1, r / 2 + 1, COL(240, 240, 245));
    gfx_rect(x - r, y - 1, r * 2 + 1, 2, COL(30, 30, 40));
    gfx_ellipse(x, y, r / 3 + 1, r / 3 + 1, COL(250, 250, 250));
}

static int wrap_lines(const char *s, int *starts, int *lens, int maxLines)
{
    int n = 0, pos = 0, len = (int)strlen(s);
    while (pos < len && n < maxLines) {
        int lineEnd = pos, lastSpace = -1, w = 0, p = pos;
        while (p < len) {
            unsigned char c = (unsigned char)s[p];
            int clen = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : 4;
            if (c == ' ') lastSpace = p;
            if (c == '\n') { lineEnd = p; break; }
            if (w + 6 > 238) {
                if (lastSpace > pos) lineEnd = lastSpace;
                else lineEnd = p;
                goto done;
            }
            w += 6;
            p += clen;
            lineEnd = p;
        }
        if (p >= len) lineEnd = len;
    done:
        starts[n] = pos;
        lens[n] = lineEnd - pos;
        n++;
        pos = lineEnd;
        while (pos < len && (s[pos] == ' ' || s[pos] == '\n')) pos++;
    }
    return n;
}

static void draw_msgbox(void)
{
    int starts[3], lens[3], n, i, typed = scene.msgLen;
    gfx_box(2, 152, 252, 38, NAVY, COL(230, 232, 245));
    gfx_rect(4, 154, 248, 1, COL(70, 84, 130));
    n = wrap_lines(scene.msg, starts, lens, 3);
    for (i = 0; i < n && i < 3; i++) {
        char buf[100];
        int take = lens[i];
        if (take > 98) take = 98;
        if (typed < starts[i]) break;
        if (typed - starts[i] < take) {
            take = typed - starts[i];
            while (take < lens[i] && ((unsigned char)scene.msg[starts[i] + take] & 0xC0) == 0x80) take++;
        }
        memcpy(buf, scene.msg + starts[i], (size_t)take);
        buf[take] = 0;
        gfx_text_shadow(9, 158 + i * 11, buf, WHITE, BLACK);
    }
}

static void draw_actor(const Actor *a, int back, int cx, int baseY, int scale)
{
    int dy = 0;
    if (!a->visible || a->hidden) return;
    if (a->fade > 0) {
        gfxClipY1 = baseY + 2;
        dy = a->fade * SPR * scale / 256;
    }
    if (a->gim == 2) scale++;
    sprite_draw(a->species, back, a->shiny, cx + a->dx, baseY + a->dy + dy, scale, a->white);
    gfxClipY1 = SCR_H;
}

static void render_scene_body(void)
{
    char tag[40];
    gfx_target(0);
    gfx_copy_from(bgCache);
    /* ennemi ou dresseur */
    if (scene.trainer && (!scene.foe.visible || scene.foe.hidden)) draw_trainer(186, 88, 2, scene.trainerSeed);
    if (scene.foe.visible && !scene.foe.hidden) {
        if (scene.foe.boss) gfx_tint(150, 14, 72, 76, COL(240, 60, 60), 40);
        draw_actor(&scene.foe, 0, 186, 88, 2);
    }
    if (scene.ally.visible && !scene.ally.hidden) draw_actor(&scene.ally, 1, 62, 150, 3);
    if (scene.showParty) {
        int i;
        for (i = 0; i < run.partyCount && i < MAX_PARTY; i++) {
            int x = 24 + i * 40;
            sprite_draw(run.party[i].species, 0, run.party[i].shiny, x, 140 + ((frameCount / 10 + i) % 2), 1, 0);
            ui_draw_hpbar(x - 14, 144, 28, run.party[i].hp, run.party[i].stat[0]);
        }
    }
    draw_fx();
    if (scene.ballShow) draw_ball_icon(scene.ballX + scene.ballAngle, scene.ballY, 5);
    if (scene.foe.visible) draw_box_info(&scene.foe, 4, 4, 0);
    if (scene.ally.visible) draw_box_info(&scene.ally, 132, 102, 1);
    if (scene.ally.visible && scene.ally.gim) {
        static const char *const gn[4] = {"", "MÉGA", "DYNAMAX", "TÉRA"};
        static const u16 gc[4] = {0, COL(255, 120, 200), COL(230, 50, 70), COL(80, 200, 230)};
        gfx_box(132, 91, 52, 11, gfx_darken(gc[scene.ally.gim], 90), gc[scene.ally.gim]);
        gfx_text_center(158, 93, gn[scene.ally.gim], WHITE);
    }
    if (scene.showWave) {
        snprintf(tag, sizeof(tag), "Vague %d - %s", scene.wave, biomes[scene.biome].name);
        gfx_box(132, 2, 122, 13, NAVY, COL(160, 170, 200));
        gfx_text_center(193, 4, tag, WHITE);
    }
    draw_msgbox();
    if (scene.flash > 0) gfx_tint(0, 0, SCR_W, SCR_H, WHITE, scene.flash > 256 ? 256 : scene.flash);
}

/* ------------------------------------------------------------ écran du bas */
void ui_draw_button(int x, int y, int w, int h, u16 color, int selected, int enabled)
{
    u16 c = enabled ? color : COL(110, 110, 120);
    gfx_box(x, y, w, h, gfx_darken(c, 70), selected ? WHITE : gfx_darken(c, 160));
    gfx_rect(x + 3, y + 2, w - 6, h / 2 - 2, gfx_lighten(c, 30));
    gfx_rect(x + 3, y + h / 2, w - 6, h / 2 - 3, c);
    if (selected) {
        gfx_rect(x + 1, y + 1, w - 2, 1, WHITE);
        gfx_rect(x + 1, y + h - 2, w - 2, 1, WHITE);
    }
}

void ui_btn_rect(int i, int *x, int *y, int *w, int *h)
{
    if (bottom.mode == BM_MENU) {
        *w = 118; *h = 56;
        *x = 8 + (i % 2) * 122;
        *y = 66 + (i / 2) * 60;
    } else if (bottom.mode == BM_MOVES) {
        *w = 120; *h = 50;
        *x = 6 + (i % 2) * 124;
        *y = 56 + (i / 2) * 54;
    } else {
        int cols = bottom.cols < 1 ? 1 : bottom.cols;
        int rows = (bottom.count + cols - 1) / cols;
        int y0 = 24, y1 = bottom.back ? 164 : 188;
        int gap = 4;
        if (rows < 1) rows = 1;
        *w = (244 - gap * (cols - 1)) / cols;
        *h = (y1 - y0 - gap * (rows - 1)) / rows;
        if (*h > 44) *h = 44;
        *x = 6 + (i % cols) * (*w + gap);
        *y = y0 + (i / cols) * (*h + gap);
    }
}

static void gim_rect(int g, int *x, int *y, int *w, int *h)
{
    *x = 70 + g * 61; *y = 168; *w = 59; *h = 20;
}
static int back_w(void) { return (bottom.mode == BM_MOVES && bottom.gimShow) ? 58 : 244; }

static int in_rect(int px, int py, int x, int y, int w, int h) { return px >= x && px < x + w && py >= y && py < y + h; }

static void render_bottom(void)
{
    int i;
    gfx_target(1);
    if (botCustom) { botCustom(); return; }
    gfx_gradient(0, SCR_H, COL(26, 34, 70), COL(12, 16, 36));
    if (bottom.mode == BM_INFO || bottom.mode == BM_NONE) {
        gfx_text_center(128, 8, "POKEROGUE DS", COL(255, 90, 90));
        gfx_printf(10, 28, WHITE, "Vague %d / %d", run.wave, MAX_WAVE);
        gfx_printf(10, 40, COL(190, 220, 255), "Biome : %s", biomes[run.biome].name);
        gfx_printf(10, 52, COL(255, 225, 120), "Argent : %d G", run.money);
        gfx_printf(10, 64, WHITE, "Balles: %d/%d/%d/%d/%d", run.balls[0], run.balls[1], run.balls[2], run.balls[3], run.balls[4]);
        for (i = 0; i < run.partyCount && i < MAX_PARTY; i++) {
            const Mon *m = &run.party[i];
            int x = 10 + (i % 2) * 124, y = 82 + (i / 2) * 36;
            gfx_box(x - 2, y - 2, 118, 34, COL(30, 40, 80), COL(100, 112, 160));
            sprite_draw_icon(m->species, x, y - 1);
            gfx_text(x + 34, y + 1, species[m->species].name, WHITE);
            gfx_printf(x + 34, y + 11, COL(255, 235, 140), "Nv.%d", m->level);
            ui_draw_hpbar(x + 34, y + 21, 74, m->hp, m->stat[0]);
            if (m->hp <= 0) gfx_text(x + 70, y + 11, "K.O.", COL(255, 100, 100));
        }
        return;
    }
    if (bottom.title[0] && !(bottom.mode == BM_MENU && allyMon)) gfx_text_center(128, 8, bottom.title, COL(255, 235, 150));
    if (bottom.mode == BM_MENU && allyMon) {
        const Mon *m = allyMon;
        sprite_draw_icon(m->species, 8, 8);
        gfx_printf(46, 8, WHITE, "%s  Nv.%d", species[m->species].name, m->level);
        gfx_printf(46, 20, COL(160, 230, 160), "PV %d/%d", m->hp, m->stat[0]);
        gfx_printf(46, 32, COL(190, 200, 230), "Nature : %s", natureNames[m->nature]);
        gfx_text_center(128, 50, bottom.title, COL(255, 235, 150));
    }
    if (bottom.mode == BM_MOVES && bottom.sel >= 0 && bottom.sel < bottom.count) {
        const Move *mv = &moves[bottom.moveId[bottom.sel]];
        static const char *const cn[3] = {"Physique", "Spécial", "Statut"};
        ui_type_badge(10, 26, mv->type);
        gfx_text(38, 26, cn[mv->cat], COL(220, 230, 255));
        if (mv->cat != CAT_STAT) gfx_printf(100, 26, COL(255, 235, 150), "Puiss.%d", mv->power > 1 ? mv->power : 0);
        gfx_printf(170, 26, COL(190, 230, 255), "Préc.%d", mv->acc ? mv->acc : 100);
    }
    for (i = 0; i < bottom.count; i++) {
        int x, y, w, h;
        u16 col = bottom.color[i] ? bottom.color[i] : COL(70, 110, 200);
        int en = bottom.enabled[i];
        ui_btn_rect(i, &x, &y, &w, &h);
        ui_draw_button(x, y, w, h, col, i == bottom.sel, en);
        if (bottom.mode == BM_MENU) {
            gfx_text_big(x + (w - gfx_text_width(bottom.label[i]) * 2) / 2, y + (h - 20) / 2, bottom.label[i], WHITE, 2);
        } else if (bottom.mode == BM_MOVES) {
            gfx_text_shadow(x + 6, y + 8, bottom.label[i], WHITE, BLACK);
            gfx_text_shadow(x + 6, y + 28, bottom.sub[i], COL(255, 255, 255), BLACK);
        } else {
            int tx = x + 6;
            if (bottom.icon[i] > 0) {
                sprite_draw_icon(bottom.icon[i], x + 3, y + (h - 32) / 2);
                tx = x + 38;
            }
            gfx_text_shadow(tx, y + (bottom.sub[i][0] ? 5 : (h - 9) / 2), bottom.label[i], WHITE, BLACK);
            if (bottom.sub[i][0]) gfx_text_shadow(tx, y + 17, bottom.sub[i], COL(235, 240, 255), BLACK);
            if (bottom.hpmax[i] > 0) ui_draw_hpbar(tx, y + h - 11, w - (tx - x) - 8, bottom.hp[i], bottom.hpmax[i]);
        }
    }
    if (bottom.back) {
        int bx = 6, by = 168, bw = back_w(), bh = 20;
        ui_draw_button(bx, by, bw, bh, COL(150, 60, 60), 0, 1);
        gfx_text_center(bx + bw / 2, by + 6, bw < 100 ? "Retour" : "Retour (B)", WHITE);
    }
    if (bottom.mode == BM_MOVES && bottom.gimShow) {
        static const char *const gl[3] = {"MÉGA", "DYNAMAX", "TÉRA"};
        static const u16 gcol[3] = {COL(220, 80, 170), COL(210, 50, 60), COL(50, 160, 200)};
        for (i = 0; i < 3; i++) {
            int x, y, w, h, on = (bottom.gimSel == i);
            gim_rect(i, &x, &y, &w, &h);
            ui_draw_button(x, y, w, h, gcol[i], on, bottom.gimAvail[i]);
            gfx_text_center(x + w / 2, y + 6, gl[i], bottom.gimAvail[i] ? WHITE : COL(170, 170, 175));
            if (on) gfx_rect(x + 2, y + h - 4, w - 4, 2, COL(255, 235, 120));
        }
    }
}

/* ------------------------------------------------------------ boucle d'image */
static void anim_step(void)
{
    scene.fxT += scene.fx ? 1 : 0;
    if (scene.msgLen < scene.msgFull) {
        scene.msgLen += 2 * gameSpeed;
        if (scene.msgLen > scene.msgFull) scene.msgLen = scene.msgFull;
    }
}

/* ---- rendu par zones sales (écran du haut) ---- */
typedef struct { int x, y, w, h; } Rc;
static Scene prevScene;
static int prevValid, forceFullTop = 1, forceFullBot = 1, prevFade, prevTopCustom, prevBotCustom;
static u32 prevBotHash;
int gameSpeed = 1;

static u32 fnv(u32 h, const void *p, int n)
{
    const unsigned char *b = (const unsigned char *)p;
    while (n--) { h ^= *b++; h *= 16777619u; }
    return h;
}
static u32 sig_sprite(const Actor *a)
{
    int v[11];
    v[10] = a->gim;
    v[0] = a->visible; v[1] = a->hidden; v[2] = a->species; v[3] = a->shiny; v[4] = a->boss;
    v[5] = a->dx; v[6] = a->dy; v[7] = a->white; v[8] = a->fade; v[9] = a->ballPhase;
    return fnv(2166136261u, v, sizeof(v));
}
static u32 sig_box(const Actor *a)
{
    int v[12];
    v[10] = a->t1; v[11] = a->t2;
    v[9] = a->gim;
    v[0] = a->visible; v[1] = a->level; v[2] = a->hpShown; v[3] = a->hpMax; v[4] = a->status; v[5] = a->expPct;
    v[6] = a->species; v[7] = a->boss; v[8] = a->shiny;
    return fnv(2166136261u, v, sizeof(v));
}

static void pass(const Rc *r)
{
    gfx_set_clip(r->x, r->y, r->w, r->h);
    render_scene_body();
    gfx_mark(0, r->x, r->y, r->w, r->h);
}

static void render_top_frame(void)
{
    Rc list[10];
    int n = 0, full = forceFullTop;
    gfx_reset_clip();
    gfx_target(0);
    if (bgBiome != scene.biome) { build_bg(scene.biome); full = 1; }
    if (!prevValid || scene.flash > 0 || prevScene.flash > 0 || scene.trainer != prevScene.trainer ||
        scene.biome != prevScene.biome || scene.shake != prevScene.shake)
        full = 1;
    if (full) {
        gfx_reset_clip();
        render_scene_body();
        gfx_mark_all(0);
    } else {
#define ADD(X, Y, W, H) do { list[n].x = (X); list[n].y = (Y); list[n].w = (W); list[n].h = (H); n++; } while (0)
        if (sig_sprite(&scene.foe) != sig_sprite(&prevScene.foe) || scene.foe.boss != prevScene.foe.boss) ADD(116, 8, 140, 92);
        if (sig_sprite(&scene.ally) != sig_sprite(&prevScene.ally)) ADD(0, 36, 134, 120);
        if (sig_box(&scene.foe) != sig_box(&prevScene.foe) || strcmp(scene.foe.name, prevScene.foe.name)) ADD(4, 4, 122, 40);
        if (sig_box(&scene.ally) != sig_box(&prevScene.ally) || strcmp(scene.ally.name, prevScene.ally.name)) ADD(132, 90, 120, 60);
        if (scene.msgLen != prevScene.msgLen || scene.msgFull != prevScene.msgFull || strcmp(scene.msg, prevScene.msg)) ADD(2, 152, 252, 38);
        if (scene.showWave != prevScene.showWave || scene.wave != prevScene.wave) ADD(132, 2, 122, 14);
        if (scene.fx || prevScene.fx) {
            int tf = scene.fx ? scene.fxTarget : prevScene.fxTarget;
            if (tf) ADD(132, 4, 124, 100); else ADD(10, 58, 110, 100);
        }
        if (scene.ballShow || prevScene.ballShow) ADD(56, 14, 160, 96);
        if (scene.showParty || prevScene.showParty) ADD(0, 100, 256, 52);
        if (scene.ally.dx != prevScene.ally.dx) { }
#undef ADD
        {
            int i;
            for (i = 0; i < n; i++) pass(&list[i]);
        }
    }
    gfx_reset_clip();
    prevScene = scene;
    prevValid = 1;
    forceFullTop = 0;
}

static u32 bottom_hash(void)
{
    u32 h = fnv(2166136261u, &bottom, sizeof(bottom));
    int i;
    int v[6];
    v[0] = run.wave; v[1] = run.money; v[2] = run.biome; v[3] = run.partyCount;
    v[4] = (int)(run.balls[0] | (run.balls[1] << 8) | (run.balls[2] << 16) | (run.balls[3] << 24));
    v[5] = run.balls[4];
    h = fnv(h, v, sizeof(v));
    for (i = 0; i < run.partyCount && i < MAX_PARTY; i++) {
        int m[4];
        m[0] = run.party[i].species; m[1] = run.party[i].level; m[2] = run.party[i].hp; m[3] = run.party[i].stat[0];
        h = fnv(h, m, sizeof(m));
    }
    if (allyMon) {
        int m[3];
        m[0] = allyMon->species; m[1] = allyMon->level; m[2] = allyMon->hp;
        h = fnv(h, m, sizeof(m));
    }
    return h;
}

void ui_force_redraw(void) { forceFullTop = 1; forceFullBot = 1; prevValid = 0; }

void frame(void)
{
    int fading;
    input_poll();
    frameCount++;
    run.frames++;
    anim_step();
    fading = (fadeLevel > 0 || prevFade > 0);
    /* écran du haut */
    if (topCustom || prevTopCustom || fading) {
        gfx_reset_clip();
        gfx_target(0);
        if (topCustom) topCustom();
        else { if (bgBiome != scene.biome) build_bg(scene.biome); gfx_target(0); render_scene_body(); }
        gfx_mark_all(0);
        if (!topCustom) { prevValid = 0; forceFullTop = 1; }
    } else render_top_frame();
    prevTopCustom = topCustom != 0;
    /* écran du bas */
    {
        u32 h = bottom_hash();
        if (botCustom || prevBotCustom || fading || forceFullBot || h != prevBotHash) {
            gfx_reset_clip();
            gfx_target(1);
            render_bottom();
            gfx_mark_all(1);
            prevBotHash = h;
            forceFullBot = 0;
        }
        prevBotCustom = botCustom != 0;
    }
    if (fadeLevel > 0) {
        gfx_reset_clip();
        gfx_target(0);
        gfx_shade(0, 0, SCR_W, SCR_H, fadeLevel > 256 ? 256 : fadeLevel);
        gfx_target(1);
        gfx_shade(0, 0, SCR_W, SCR_H, fadeLevel > 256 ? 256 : fadeLevel);
        gfx_mark_all(0);
        gfx_mark_all(1);
    }
    prevFade = fadeLevel;
    gfx_present();
    audio_update();
}

/* image d'animation : à vitesse x2..x4 on n'affiche qu'une image logique sur N */
void anim_frame(void)
{
    static int cnt;
    if (gameSpeed <= 1 || (++cnt % gameSpeed) == 0) frame();
}

void ui_fade_out(void)
{
    int i;
    for (i = 0; i <= 8; i++) { fadeLevel = i * 32; frame(); }
}
void ui_fade_in(void)
{
    int i;
    for (i = 8; i >= 0; i--) { fadeLevel = i * 32; frame(); }
    fadeLevel = 0;
}

void ui_reset_scene(void)
{
    memset(&scene, 0, sizeof(scene));
    memset(&bottom, 0, sizeof(bottom));
    scene.biome = run.biome;
    scene.wave = run.wave;
    scene.showWave = 1;
    bottom.mode = BM_INFO;
    allyMon = foeMon = 0;
}

void ui_wait(int frames)
{
    while (frames-- > 0) anim_frame();
}

static void set_msg(const char *s)
{
    strncpy(scene.msg, s, sizeof(scene.msg) - 1);
    scene.msg[sizeof(scene.msg) - 1] = 0;
    scene.msgLen = 0;
    scene.msgFull = (int)strlen(scene.msg);
}

void ui_msg_nowait(const char *fmt, ...)
{
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    set_msg(buf);
    scene.msgLen = scene.msgFull;
    frame();
}

void ui_msg(const char *fmt, ...)
{
    char buf[160];
    int hold, wait, skipped = 0;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    set_msg(buf);
    hold = 0;
    wait = (scene.msgFull > 40 ? 46 : 30) / gameSpeed;
    if (wait < 6) wait = 6;
    for (;;) {
        frame();
        if (scene.msgLen < scene.msgFull) {
            if ((kDown & (KEY_A | KEY_B)) || touchNew) { scene.msgLen = scene.msgFull; skipped = 1; }
            continue;
        }
        hold++;
        if (skipped) { hold = hold < 10 ? 10 : hold; skipped = 0; }
        if ((kDown & (KEY_A | KEY_B)) || touchNew || hold > wait) break;
    }
}

void ui_wait_key(void)
{
    for (;;) {
        frame();
        if ((kDown & (KEY_A | KEY_B | KEY_START)) || touchNew) break;
    }
}

void ui_flash(int frames)
{
    int i;
    for (i = 0; i < frames; i++) {
        scene.flash = 256 - i * 256 / frames;
        anim_frame();
    }
    scene.flash = 0;
}

void ui_shake(int frames)
{
    int i;
    for (i = 0; i < frames; i++) {
        int d = (i & 1) ? 3 : -3;
        scene.foe.dx = scene.fxTarget ? d : 0;
        scene.ally.dx = scene.fxTarget ? 0 : d;
        anim_frame();
    }
    scene.foe.dx = scene.ally.dx = 0;
}

void ui_lunge(int side)
{
    Actor *a = side ? &scene.foe : &scene.ally;
    int dir = side ? -1 : 1, i;
    for (i = 1; i <= 5; i++) { a->dx = dir * i * 4; a->dy = -dir * i; anim_frame(); }
    for (i = 4; i >= 0; i--) { a->dx = dir * i * 4; a->dy = -dir * i; anim_frame(); }
    a->dx = a->dy = 0;
}

void ui_attack_fx(int type, int toFoe)
{
    int i;
    scene.fx = type + 1;
    scene.fxT = 0;
    scene.fxTarget = toFoe;
    for (i = 0; i < 16; i++) anim_frame();
    scene.fx = 0;
}

static void fill_actor(Actor *a, const Mon *m, int boss)
{
    const Species *sp = &species[m->species];
    memset(a, 0, sizeof(*a));
    a->visible = 1;
    a->species = m->species;
    a->level = m->level;
    a->hp = a->hpShown = m->hp;
    a->hpMax = m->stat[0];
    a->status = m->status;
    a->shiny = m->shiny;
    a->boss = boss;
    a->gim = m->gim;
    a->t1 = m->tOv ? m->tmpT1 : sp->t1;
    a->t2 = m->tOv ? m->tmpT2 : sp->t2;
    strncpy(a->name, sp->name, sizeof(a->name) - 1);
}

static int exp_pct(const Mon *m)
{
    int lo = mon_exp_for(m->level, m->species), hi = mon_exp_for(m->level + 1, m->species);
    if (hi <= lo) return 0;
    int p = (m->exp - lo) * 100 / (hi - lo);
    return p < 0 ? 0 : p > 100 ? 100 : p;
}

void ui_set_foe(const Mon *m, int boss)
{
    foeMon = m;
    fill_actor(&scene.foe, m, boss);
}
void ui_set_ally(const Mon *m)
{
    allyMon = m;
    fill_actor(&scene.ally, m, 0);
    scene.ally.expPct = exp_pct(m);
}

void ui_sync_hp(void)
{
    if (foeMon) {
        scene.foe.hp = foeMon->hp;
        scene.foe.hpMax = foeMon->stat[0];
        scene.foe.status = foeMon->status;
        scene.foe.level = foeMon->level;
    }
    if (allyMon) {
        scene.ally.hp = allyMon->hp;
        scene.ally.hpMax = allyMon->stat[0];
        scene.ally.status = allyMon->status;
        scene.ally.level = allyMon->level;
        scene.ally.gim = allyMon->gim;
        scene.ally.t1 = allyMon->tOv ? allyMon->tmpT1 : species[allyMon->species].t1;
        scene.ally.t2 = allyMon->tOv ? allyMon->tmpT2 : species[allyMon->species].t2;
        scene.ally.expPct = exp_pct(allyMon);
    }
}

void ui_animate_hp(void)
{
    int guard = 0;
    ui_sync_hp();
    while ((scene.foe.hpShown != scene.foe.hp || scene.ally.hpShown != scene.ally.hp) && guard++ < 400) {
        Actor *as[2];
        int i;
        as[0] = &scene.foe;
        as[1] = &scene.ally;
        for (i = 0; i < 2; i++) {
            Actor *a = as[i];
            int step = a->hpMax / 60 + 1;
            if (a->hpShown < a->hp) { a->hpShown += step; if (a->hpShown > a->hp) a->hpShown = a->hp; }
            else if (a->hpShown > a->hp) { a->hpShown -= step; if (a->hpShown < a->hp) a->hpShown = a->hp; }
        }
        anim_frame();
    }
    scene.foe.hpShown = scene.foe.hp;
    scene.ally.hpShown = scene.ally.hp;
}

void ui_bottom_info(void)
{
    bottom.mode = BM_INFO;
    bottom.count = 0;
    bottom.back = 0;
    bottom.title[0] = 0;
}

/* ------------------------------------------------------------ menus */
static int choose_inner(int mode, const char *title, int n, const char labels[][28], const char subs[][28],
                        const u16 *colors, const u8 *enabled, int cols, int allowBack)
{
    int i, firstEn = -1;
    if (n > 8) n = 8;
    bottom.mode = mode;
    bottom.count = n;
    bottom.cols = cols;
    bottom.back = allowBack;
    strncpy(bottom.title, title ? title : "", sizeof(bottom.title) - 1);
    bottom.title[sizeof(bottom.title) - 1] = 0;
    for (i = 0; i < n; i++) {
        strncpy(bottom.label[i], labels[i], 27);
        bottom.label[i][27] = 0;
        if (subs) { strncpy(bottom.sub[i], subs[i], 27); bottom.sub[i][27] = 0; }
        else bottom.sub[i][0] = 0;
        bottom.color[i] = colors ? colors[i] : 0;
        bottom.enabled[i] = enabled ? enabled[i] : 1;
        if (bottom.enabled[i] && firstEn < 0) firstEn = i;
    }
    bottom.sel = firstEn < 0 ? 0 : firstEn;
    /* attendre le relâchement */
    for (i = 0; i < 90 && touchHeld; i++) frame();
    frame();
    uiWaitingChoice = 1;
    for (;;) {
        int k;
        frame();
        if (touchNew) {
            for (k = 0; k < n; k++) {
                int x, y, w, h;
                ui_btn_rect(k, &x, &y, &w, &h);
                if (in_rect(touchX, touchY, x, y, w, h) && bottom.enabled[k]) {
                    bottom.sel = k;
                    audio_sfx(SFX_SELECT);
                    return k;
                }
            }
            if (mode == BM_MOVES && bottom.gimShow) {
                for (k = 0; k < 3; k++) {
                    int x, y, w, h;
                    gim_rect(k, &x, &y, &w, &h);
                    if (bottom.gimAvail[k] && in_rect(touchX, touchY, x, y, w, h)) {
                        bottom.gimSel = (bottom.gimSel == k) ? -1 : k;
                        audio_sfx(SFX_SELECT);
                    }
                }
            }
            if (allowBack && in_rect(touchX, touchY, 6, 168, back_w(), 20)) { audio_sfx(SFX_CANCEL); return -1; }
        }
        if (kDown & KEY_A) {
            if (bottom.enabled[bottom.sel]) { audio_sfx(SFX_SELECT); return bottom.sel; }
        }
        if ((kDown & KEY_X) && mode == BM_MOVES && bottom.gimShow) {
            int g = bottom.gimSel, t;
            for (t = 0; t < 4; t++) {
                g++;
                if (g > 2) g = -1;
                if (g < 0 || bottom.gimAvail[g]) break;
            }
            bottom.gimSel = g;
            audio_sfx(SFX_SELECT);
        }
        if ((kDown & KEY_B) && allowBack) { audio_sfx(SFX_CANCEL); return -1; }
        {
            int d = 0, c = (mode == BM_LIST) ? (cols < 1 ? 1 : cols) : 2;
            int s;
            if (kDown & KEY_RIGHT) d = 1;
            else if (kDown & KEY_LEFT) d = -1;
            else if (kDown & KEY_DOWN) d = c;
            else if (kDown & KEY_UP) d = -c;
            if (d) {
                s = bottom.sel + d;
                if (s >= 0 && s < n) bottom.sel = s;
            }
        }
    }
}

int ui_choose(int mode, const char *title, int n, const char labels[][28], const char subs[][28],
              const u16 *colors, const u8 *enabled, int cols, int allowBack)
{
    int r = choose_inner(mode, title, n, labels, subs, colors, enabled, cols, allowBack);
    uiWaitingChoice = 0;
    memset(bottom.icon, 0, sizeof(bottom.icon));
    memset(bottom.hpmax, 0, sizeof(bottom.hpmax));
    memset(bottom.moveId, 0, sizeof(bottom.moveId));
    return r;
}

int ui_main_menu(void)
{
    static const char labels[4][28] = {"COMBAT", "BALLE", "EQUIPE", "FUITE"};
    static const u16 cols[4] = {COL(210, 60, 50), COL(60, 110, 210), COL(70, 170, 90), COL(120, 120, 140)};
    return ui_choose(BM_MENU, "Que faire ?", 4, labels, 0, cols, 0, 2, 0);
}

int ui_pick_party(const char *title, int allowBack, int onlyAlive, int onlyFainted)
{
    char labels[8][28], subs[8][28];
    u8 en[8];
    u16 cols[8];
    int i, n = run.partyCount, r;
    if (n > 6) n = 6;
    for (i = 0; i < n; i++) {
        const Mon *m = &run.party[i];
        snprintf(labels[i], 28, "%s Nv.%d", species[m->species].name, m->level);
        if (m->hp <= 0) snprintf(subs[i], 28, "K.O.");
        else if (m->status) {
            static const char *const nm[] = {"", "Brûlé", "Paralysé", "Empoisonné", "Intoxiqué", "Endormi", "Gelé", ""};
            snprintf(subs[i], 28, "%s", nm[m->status]);
        } else snprintf(subs[i], 28, "%d/%d PV", m->hp, m->stat[0]);
        en[i] = 1;
        if (onlyAlive && m->hp <= 0) en[i] = 0;
        if (onlyFainted && m->hp > 0) en[i] = 0;
        cols[i] = m->hp <= 0 ? COL(120, 70, 70) : COL(70, 130, 90);
    }
    for (i = 0; i < n; i++) {
        bottom.icon[i] = run.party[i].species;
        bottom.hp[i] = run.party[i].hp;
        bottom.hpmax[i] = run.party[i].stat[0];
    }
    r = ui_choose(BM_LIST, title, n, (const char (*)[28])labels, (const char (*)[28])subs, cols, en, 2, allowBack);
    return r;
}

/* ------------------------------------------------------------ fiche Pokémon */
static const Mon *cardMon;

static void card_top(void)
{
    const Mon *m = cardMon;
    const Species *sp = &species[m->species];
    static const char *const sn[6] = {"PV", "Atq", "Déf", "AtqSp", "DéfSp", "Vit"};
    int i;
    gfx_gradient(0, SCR_H, COL(30, 50, 96), COL(16, 24, 56));
    gfx_ellipse(52, 82, 38, 7, COL(20, 30, 60));
    sprite_draw(m->species, 0, m->shiny, 52, 80, 2, 0);
    gfx_text_shadow(8, 6, sp->name, WHITE, BLACK);
    gfx_printf(8, 18, COL(255, 235, 140), "Nv.%d  %s", m->level, natureNames[m->nature]);
    ui_type_badge(8, 30, sp->t1);
    if (sp->t2 != NO_TYPE) ui_type_badge(32, 30, sp->t2);
    for (i = 0; i < 6; i++) {
        int y = 8 + i * 14, v = m->stat[i];
        gfx_text(112, y, sn[i], COL(190, 210, 255));
        gfx_rect(150, y + 1, 70, 6, COL(30, 36, 60));
        {
            int w = v * 70 / (m->level * 4 + 60);
            if (w > 70) w = 70;
            gfx_rect(150, y + 1, w, 6, i == 0 ? COL(80, 210, 110) : COL(240, 190, 70));
        }
        gfx_printf(224, y, WHITE, "%d", v);
    }
    gfx_text(8, 98, "Attaques", COL(255, 235, 150));
    gfx_printf(112, 98, COL(190, 230, 255), "Talent : %s", ability_name(mon_ability(m)));
    for (i = 0; i < 4; i++) {
        int x = 8 + (i % 2) * 124, y = 110 + (i / 2) * 22;
        if (m->moves[i] == 255) continue;
        gfx_rect(x, y, 118, 18, gfx_darken(typeColors[moves[m->moves[i]].type], 60));
        gfx_text_shadow(x + 4, y + 2, moves[m->moves[i]].name, WHITE, BLACK);
        gfx_printf(x + 4, y + 10, COL(235, 235, 245), "PP %d/%d", m->pp[i], m->ppmax[i]);
    }
    {
        char hb[40];
        int y = 158;
        static const char *const hn[HI_COUNT] = {"Restes", "Orbe Vie", "Lentille", "Griffe R.", "Oeuf Chance", "Baie Sitrus", "Booster", "Amulette"};
        gfx_text(8, y, "Objets :", COL(255, 235, 150));
        hb[0] = 0;
        for (i = 0; i < HI_COUNT; i++)
            if (m->held[i]) {
                char t[24];
                snprintf(t, sizeof(t), "%s x%d ", hn[i], m->held[i]);
                if (strlen(hb) + strlen(t) < sizeof(hb)) strcat(hb, t);
            }
        if (!hb[0]) strcpy(hb, "aucun");
        gfx_text(56, y, hb, WHITE);
        gfx_printf(8, 172, COL(190, 200, 230), "PV %d/%d  EXP %d", m->hp, m->stat[0], m->exp);
    }
}
static void card_bot(void)
{
    gfx_gradient(0, SCR_H, COL(26, 34, 70), COL(12, 16, 36));
    gfx_text_center(128, 90, "Touche l'ecran ou appuie sur B", COL(220, 230, 255));
}

void ui_info_card(const Mon *m)
{
    cardMon = m;
    ui_set_custom(card_top, card_bot);
    for (;;) {
        frame();
        if ((kDown & (KEY_A | KEY_B)) || touchNew) break;
    }
    ui_set_custom(0, 0);
}

void ui_party_screen(void)
{
    for (;;) {
        int pick = ui_pick_party("Equipe (touche pour voir)", 1, 0, 0);
        char labels[3][28] = {"Résumé", "Passer en tête", "Retour"};
        int sub;
        if (pick < 0) break;
        sub = ui_choose(BM_LIST, species[run.party[pick].species].name, 3, (const char (*)[28])labels, 0, 0, 0, 1, 0);
        if (sub == 0) ui_info_card(&run.party[pick]);
        else if (sub == 1 && pick > 0) {
            Mon t = run.party[0];
            run.party[0] = run.party[pick];
            run.party[pick] = t;
        }
    }
    bottom.icon[0] = 0;
}

/* ------------------------------------------------------------ écran titre et fin */
static int titleMode;   /* 0 = titre, 1 = fin */
static int endWon;

static void title_top(void)
{
    int i;
    static const int dex[3] = {1, 4, 7};
    scene.biome = 0;
    if (bgBiome != 0) build_bg(0);
    gfx_copy_from(bgCache);
    gfx_gradient(0, 74, COL(40, 60, 140), COL(250, 170, 120));
    for (i = 0; i < 20; i++) gfx_rect((i * 71 + frameCount / 2) % 256, (i * 23) % 60, 2, 2, COL(255, 255, 220));
    gfx_ellipse(186, 90, 58, 10, COL(70, 140, 70));
    gfx_ellipse(62, 153, 72, 13, COL(70, 140, 70));
    for (i = 0; i < 3; i++) {
        int bob = ((frameCount / 8 + i * 3) % 6 < 3) ? 0 : 2;
        sprite_draw(dex[i], 0, 0, 44 + i * 82, 118 + bob, 2, 0);
    }
    gfx_box(14, 14, 228, 54, COL(20, 24, 60), COL(255, 215, 90));
    if (titleMode == 0) {
        gfx_text_big(30, 22, "POKEROGUE", COL(255, 220, 80), 3);
        gfx_text_center(128, 52, "version Nintendo DS", COL(180, 220, 255));
    } else {
        gfx_text_big(24, 22, endWon ? "VICTOIRE !" : "GAME OVER", endWon ? COL(120, 255, 140) : COL(255, 110, 110), 3);
        gfx_printf(30, 52, WHITE, "Vague %d  Captures %d  K.O. %d", run.wave, run.caught, run.kills);
    }
    gfx_text_center(128, 176, "Usage personnel - projet fan", COL(220, 220, 230));
}

void ui_title_screen_draw(void)
{
    titleMode = 0;
    ui_set_custom(title_top, 0);
}

void ui_draw_gameover(void)
{
    titleMode = 1;
    ui_set_custom(title_top, 0);
}
void ui_set_end_won(int w) { endWon = w; }
