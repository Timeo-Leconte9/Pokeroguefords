/* gfx.c : moteur graphique logiciel (bitmap 16 bits sur les deux écrans), police, entrées */
#include "game.h"
#include <stdarg.h>

static u16 backbuf[2][SCR_W * SCR_H];
static u16 *cur = backbuf[0];
static int curScreen = 0;
static u16 *vram[2];

int gfxClipY1 = SCR_H;      /* clipping bas (animation de K.O.) */
int clipX0 = 0, clipY0 = 0, clipX1 = SCR_W, clipY1 = SCR_H;   /* zone de rendu courante (rectangles sales) */

#define MAX_DIRTY 12
typedef struct { int x, y, w, h; } DRect;
static DRect dirty[2][MAX_DIRTY];
static int dirtyN[2];
unsigned long gfxPixelCount;   /* statistiques pour les tests PC */

void gfx_set_clip(int x, int y, int w, int h)
{
    clipX0 = x < 0 ? 0 : x;
    clipY0 = y < 0 ? 0 : y;
    clipX1 = x + w > SCR_W ? SCR_W : x + w;
    clipY1 = y + h > SCR_H ? SCR_H : y + h;
}
void gfx_reset_clip(void) { clipX0 = 0; clipY0 = 0; clipX1 = SCR_W; clipY1 = SCR_H; }

void gfx_mark(int screen, int x, int y, int w, int h)
{
    int i, x1, y1;
    DRect *d = dirty[screen ? 1 : 0];
    int *n = &dirtyN[screen ? 1 : 0];
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCR_W) w = SCR_W - x;
    if (y + h > SCR_H) h = SCR_H - y;
    if (w <= 0 || h <= 0) return;
    x1 = x + w; y1 = y + h;
    for (i = 0; i < *n; i++) {   /* fusion si chevauchement */
        if (x < d[i].x + d[i].w && x1 > d[i].x && y < d[i].y + d[i].h && y1 > d[i].y) {
            int nx = x < d[i].x ? x : d[i].x, ny = y < d[i].y ? y : d[i].y;
            int mx = x1 > d[i].x + d[i].w ? x1 : d[i].x + d[i].w, my = y1 > d[i].y + d[i].h ? y1 : d[i].y + d[i].h;
            d[i].x = nx; d[i].y = ny; d[i].w = mx - nx; d[i].h = my - ny;
            return;
        }
    }
    if (*n >= MAX_DIRTY) {   /* trop de zones : tout l'écran */
        d[0].x = 0; d[0].y = 0; d[0].w = SCR_W; d[0].h = SCR_H;
        *n = 1;
        return;
    }
    d[*n].x = x; d[*n].y = y; d[*n].w = w; d[*n].h = h;
    (*n)++;
}
void gfx_mark_all(int screen) { dirtyN[screen ? 1 : 0] = 0; gfx_mark(screen, 0, 0, SCR_W, SCR_H); }
u32 kDown, kHeld;
int touchNew, touchHeld, touchX, touchY;
u32 frameCount;

/* ------------------------------------------------------------ aléatoire */
u32 rngState = 0x1234567u;
void rng_seed(u32 s) { rngState = s ? s : 0x1234567u; }
u32 rng(void)
{
    /* xorshift32 */
    u32 x = rngState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rngState = x;
    return x;
}
int rnd(int n) { return n <= 1 ? 0 : (int)((rng() >> 4) % (u32)n); }

/* ------------------------------------------------------------ init */
void gfx_init(void)
{
    int bg0, bg1;
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    bg0 = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    bg1 = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    vram[0] = bgGetGfxPtr(bg0);
    vram[1] = bgGetGfxPtr(bg1);
    memset(backbuf, 0, sizeof(backbuf));
}

void gfx_target(int screen)
{
    curScreen = screen ? 1 : 0;
    cur = backbuf[curScreen];
}

void gfx_present(void)
{
    int i, k, y;
    swiWaitForVBlank();
    for (i = 0; i < 2; i++) {
        for (k = 0; k < dirtyN[i]; k++) {
            DRect *d = &dirty[i][k];
            if (d->w >= SCR_W) {
                DC_FlushRange(backbuf[i] + d->y * SCR_W, (u32)(d->h * SCR_W * 2));
                dmaCopy(backbuf[i] + d->y * SCR_W, vram[i] + d->y * SCR_W, (u32)(d->h * SCR_W * 2));
            } else {
                for (y = d->y; y < d->y + d->h; y++) {
                    DC_FlushRange(backbuf[i] + y * SCR_W + d->x, (u32)(d->w * 2));
                    dmaCopy(backbuf[i] + y * SCR_W + d->x, vram[i] + y * SCR_W + d->x, (u32)(d->w * 2));
                }
            }
        }
        dirtyN[i] = 0;
    }
}

void input_poll(void)
{
    touchPosition t;
    scanKeys();
    kDown = keysDown();
    kHeld = keysHeld();
    touchNew = (kDown & KEY_TOUCH) ? 1 : 0;
    touchHeld = (kHeld & KEY_TOUCH) ? 1 : 0;
    if (touchHeld) {
        touchRead(&t);
        touchX = t.px;
        touchY = t.py;
    }
}

/* ------------------------------------------------------------ couleurs */
u16 gfx_mix(u16 a, u16 b, int k)
{
    int ar = a & 31, ag = (a >> 5) & 31, ab = (a >> 10) & 31;
    int br = b & 31, bg = (b >> 5) & 31, bb = (b >> 10) & 31;
    int r = ar + (((br - ar) * k) >> 8);
    int g = ag + (((bg - ag) * k) >> 8);
    int bl = ab + (((bb - ab) * k) >> 8);
    return (u16)(RGB15(r, g, bl) | BIT(15));
}
u16 gfx_lighten(u16 c, int k) { return gfx_mix(c, COL(255, 255, 255), k); }
u16 gfx_darken(u16 c, int k) { return gfx_mix(c, COL(0, 0, 0), k); }

/* ------------------------------------------------------------ primitives */
void gfx_clear(u16 c)
{
    int i;
    u32 v = c | ((u32)c << 16);
    u32 *p = (u32 *)cur;
    for (i = 0; i < SCR_W * SCR_H / 2; i++) p[i] = v;
}

void gfx_pixel(int x, int y, u16 c)
{
    if (x >= clipX0 && x < clipX1 && y >= clipY0 && y < clipY1 && y < gfxClipY1) cur[y * SCR_W + x] = c;
}

void gfx_rect(int x, int y, int w, int h, u16 c)
{
    int xx, yy, x1 = x + w, y1 = y + h;
    u32 v = c | ((u32)c << 16);
    if (x < clipX0) x = clipX0;
    if (y < clipY0) y = clipY0;
    if (x1 > clipX1) x1 = clipX1;
    if (y1 > clipY1) y1 = clipY1;
    if (y1 > gfxClipY1) y1 = gfxClipY1;
    if (x1 <= x || y1 <= y) return;
    for (yy = y; yy < y1; yy++) {
        u16 *p = cur + yy * SCR_W;
        xx = x;
        if (xx & 1) p[xx++] = c;
        for (; xx + 1 < x1; xx += 2) *(u32 *)(p + xx) = v;
        if (xx < x1) p[xx] = c;
    }
}

void gfx_hline(int x, int y, int w, u16 c) { gfx_rect(x, y, w, 1, c); }

void gfx_shade(int x, int y, int w, int h, int amount)
{
    int xx, yy, x1 = x + w, y1 = y + h;
    if (x < clipX0) x = clipX0;
    if (y < clipY0) y = clipY0;
    if (x1 > clipX1) x1 = clipX1;
    if (y1 > clipY1) y1 = clipY1;
    for (yy = y; yy < y1; yy++) {
        u16 *p = cur + yy * SCR_W;
        for (xx = x; xx < x1; xx++) {
            u16 c = p[xx];
            int k = 256 - amount;
            int r = ((c & 31) * k) >> 8, g = (((c >> 5) & 31) * k) >> 8, b = (((c >> 10) & 31) * k) >> 8;
            p[xx] = (u16)(RGB15(r, g, b) | BIT(15));
        }
    }
}

void gfx_tint(int x, int y, int w, int h, u16 col, int amount)
{
    int xx, yy, x1 = x + w, y1 = y + h;
    int cr = col & 31, cg = (col >> 5) & 31, cb = (col >> 10) & 31;
    if (x < clipX0) x = clipX0;
    if (y < clipY0) y = clipY0;
    if (x1 > clipX1) x1 = clipX1;
    if (y1 > clipY1) y1 = clipY1;
    for (yy = y; yy < y1; yy++) {
        u16 *p = cur + yy * SCR_W;
        for (xx = x; xx < x1; xx++) {
            u16 c = p[xx];
            int r = (c & 31) + (((cr - (c & 31)) * amount) >> 8);
            int g = ((c >> 5) & 31) + (((cg - ((c >> 5) & 31)) * amount) >> 8);
            int b = ((c >> 10) & 31) + (((cb - ((c >> 10) & 31)) * amount) >> 8);
            p[xx] = (u16)(RGB15(r, g, b) | BIT(15));
        }
    }
}

/* boîte aux coins arrondis, avec liseré */
void gfx_box(int x, int y, int w, int h, u16 fill, u16 border)
{
    gfx_rect(x + 1, y, w - 2, h, border);
    gfx_rect(x, y + 1, w, h - 2, border);
    gfx_rect(x + 2, y + 1, w - 4, h - 2, fill);
    gfx_rect(x + 1, y + 2, w - 2, h - 4, fill);
}

static int isqrt_i(int v)
{
    int r = 0, b = 1 << 14;
    if (v <= 0) return 0;
    while (b > v) b >>= 2;
    while (b) {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
        b >>= 2;
    }
    return r;
}

void gfx_ellipse(int cx, int cy, int rx, int ry, u16 c)
{
    int y;
    if (rx <= 0 || ry <= 0) return;
    for (y = -ry; y <= ry; y++) {
        int dx = isqrt_i(rx * rx * (ry * ry - y * y) / (ry * ry));
        gfx_rect(cx - dx, cy + y, dx * 2 + 1, 1, c);
    }
}

void gfx_gradient(int y0, int y1, u16 c0, u16 c1)
{
    int y;
    for (y = y0; y < y1; y++) {
        int k;
        if (y < clipY0 || y >= clipY1) continue;
        k = (y1 - y0) > 1 ? ((y - y0) * 256) / (y1 - y0 - 1) : 0;
        gfx_rect(0, y, SCR_W, 1, gfx_mix(c0, c1, k));
    }
}

void gfx_copy_from(const u16 *src)
{
    int y;
    if (clipX0 == 0 && clipX1 == SCR_W) {
        if (clipY1 > clipY0) memcpy(cur + clipY0 * SCR_W, src + clipY0 * SCR_W, (size_t)(clipY1 - clipY0) * SCR_W * 2);
        return;
    }
    for (y = clipY0; y < clipY1; y++) memcpy(cur + y * SCR_W + clipX0, src + y * SCR_W + clipX0, (size_t)(clipX1 - clipX0) * 2);
}
void gfx_save_to(u16 *dst) { memcpy(dst, cur, SCR_W * SCR_H * 2); }

/* ------------------------------------------------------------ sprites */
void gfx_blit(const u16 *src, int sw, int sh, int x, int y, int scale, int flip, int white)
{
    int sx, sy, i, j;
    int ylim = clipY1 < gfxClipY1 ? clipY1 : gfxClipY1;
    if (x >= clipX1 || y >= ylim || x + sw * scale <= clipX0 || y + sh * scale <= clipY0) return;
    for (sy = 0; sy < sh; sy++) {
        int dy = y + sy * scale;
        if (dy >= ylim || dy + scale <= clipY0) continue;
        for (sx = 0; sx < sw; sx++) {
            int dx = x + sx * scale;
            u16 c;
            if (dx >= clipX1 || dx + scale <= clipX0) continue;
            c = src[sy * sw + (flip ? sw - 1 - sx : sx)];
            if (!(c & BIT(15))) continue;
            if (white > 0) c = gfx_mix(c, COL(255, 255, 255), white);
            for (j = 0; j < scale; j++) {
                int yy = dy + j;
                u16 *row;
                if (yy < clipY0 || yy >= ylim) continue;
                row = cur + yy * SCR_W;
                for (i = 0; i < scale; i++) {
                    int xx = dx + i;
                    if (xx >= clipX0 && xx < clipX1) row[xx] = c;
                }
            }
        }
    }
}

/* ------------------------------------------------------------ police 5x7 (colonnes, bit0 = haut) */
static const u8 font5x7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x56, 0x20, 0x50}, {0x00, 0x08, 0x07, 0x03, 0x00}, {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00}, {0x2A, 0x1C, 0x7F, 0x1C, 0x2A}, {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x80, 0x70, 0x30, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x00, 0x60, 0x60, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02}, {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x72, 0x49, 0x49, 0x49, 0x46}, {0x21, 0x41, 0x49, 0x4D, 0x33}, {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x31}, {0x41, 0x21, 0x11, 0x09, 0x07},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x46, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x00, 0x14, 0x00, 0x00},
    {0x00, 0x40, 0x34, 0x00, 0x00}, {0x00, 0x08, 0x14, 0x22, 0x41}, {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x59, 0x09, 0x06}, {0x3E, 0x41, 0x5D, 0x59, 0x4E},
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x41, 0x51, 0x73}, {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x1C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x26, 0x49, 0x49, 0x49, 0x32}, {0x03, 0x01, 0x7F, 0x01, 0x03}, {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x03, 0x04, 0x78, 0x04, 0x03}, {0x61, 0x59, 0x49, 0x4D, 0x43}, {0x00, 0x7F, 0x41, 0x41, 0x41},
    {0x02, 0x04, 0x08, 0x10, 0x20}, {0x00, 0x41, 0x41, 0x41, 0x7F}, {0x04, 0x02, 0x01, 0x02, 0x04},
    {0x40, 0x40, 0x40, 0x40, 0x40}, {0x00, 0x03, 0x07, 0x08, 0x00}, {0x20, 0x54, 0x54, 0x78, 0x40},
    {0x7F, 0x28, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x28}, {0x38, 0x44, 0x44, 0x28, 0x7F},
    {0x38, 0x54, 0x54, 0x54, 0x18}, {0x00, 0x08, 0x7E, 0x09, 0x02}, {0x18, 0xA4, 0xA4, 0x9C, 0x78},
    {0x7F, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7D, 0x40, 0x00}, {0x20, 0x40, 0x40, 0x3D, 0x00},
    {0x7F, 0x10, 0x28, 0x44, 0x00}, {0x00, 0x41, 0x7F, 0x40, 0x00}, {0x7C, 0x04, 0x78, 0x04, 0x78},
    {0x7C, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38}, {0xFC, 0x18, 0x24, 0x24, 0x18},
    {0x18, 0x24, 0x24, 0x18, 0xFC}, {0x7C, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x24},
    {0x04, 0x04, 0x3F, 0x44, 0x24}, {0x3C, 0x40, 0x40, 0x20, 0x7C}, {0x1C, 0x20, 0x40, 0x20, 0x1C},
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, {0x44, 0x28, 0x10, 0x28, 0x44}, {0x4C, 0x90, 0x90, 0x90, 0x7C},
    {0x44, 0x64, 0x54, 0x4C, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00}, {0x00, 0x00, 0x77, 0x00, 0x00},
    {0x00, 0x41, 0x36, 0x08, 0x00}, {0x02, 0x01, 0x02, 0x04, 0x02}};

/* symboles ♀ ♂ ♥ (5 colonnes) */
static const u8 glyphFemale[5] = {0x06, 0x29, 0x79, 0x29, 0x06};
static const u8 glyphMale[5] = {0x30, 0x48, 0x4D, 0x4B, 0x37};
static const u8 glyphHeart[5] = {0x0C, 0x1E, 0x3C, 0x1E, 0x0C};

enum { AC_NONE = 0, AC_ACUTE, AC_GRAVE, AC_CIRC, AC_DIAER, AC_CEDIL };

/* décode un caractère UTF-8 ; renvoie le nombre d'octets lus ; sort : base ASCII, accent, glyphe spécial */
static int decode_char(const char *s, int *base, int *accent, const u8 **special, int *second)
{
    unsigned char c = (unsigned char)s[0];
    *accent = AC_NONE;
    *special = 0;
    *second = 0;
    if (c < 0x80) {
        *base = c;
        return 1;
    }
    if (c == 0xC3) {
        unsigned char d = (unsigned char)s[1];
        int b = '?', a = AC_NONE;
        switch (d) {
        case 0xA9: b = 'e'; a = AC_ACUTE; break;
        case 0xA8: b = 'e'; a = AC_GRAVE; break;
        case 0xAA: b = 'e'; a = AC_CIRC; break;
        case 0xAB: b = 'e'; a = AC_DIAER; break;
        case 0xA0: b = 'a'; a = AC_GRAVE; break;
        case 0xA2: b = 'a'; a = AC_CIRC; break;
        case 0xA4: b = 'a'; a = AC_DIAER; break;
        case 0xAE: b = 'i'; a = AC_CIRC; break;
        case 0xAF: b = 'i'; a = AC_DIAER; break;
        case 0xB4: b = 'o'; a = AC_CIRC; break;
        case 0xB6: b = 'o'; a = AC_DIAER; break;
        case 0xB9: b = 'u'; a = AC_GRAVE; break;
        case 0xBB: b = 'u'; a = AC_CIRC; break;
        case 0xBC: b = 'u'; a = AC_DIAER; break;
        case 0xA7: b = 'c'; a = AC_CEDIL; break;
        case 0x89: b = 'E'; a = AC_ACUTE; break;
        case 0x88: b = 'E'; a = AC_GRAVE; break;
        case 0x8A: b = 'E'; a = AC_CIRC; break;
        case 0x80: b = 'A'; a = AC_GRAVE; break;
        case 0x87: b = 'C'; a = AC_CEDIL; break;
        case 0x94: b = 'O'; a = AC_CIRC; break;
        case 0x8E: b = 'I'; a = AC_CIRC; break;
        default: break;
        }
        *base = b;
        *accent = a;
        return 2;
    }
    if (c == 0xC5 && (unsigned char)s[1] == 0x93) { *base = 'o'; *second = 'e'; return 2; }   /* œ */
    if (c == 0xC5 && (unsigned char)s[1] == 0x92) { *base = 'O'; *second = 'E'; return 2; }   /* Œ */
    if (c == 0xE2 && (unsigned char)s[1] == 0x99) {
        unsigned char d = (unsigned char)s[2];
        if (d == 0x80) { *special = glyphFemale; return 3; }
        if (d == 0x82) { *special = glyphMale; return 3; }
        if (d == 0xA5) { *special = glyphHeart; return 3; }
    }
    if (c == 0xE2 && (unsigned char)s[1] == 0x80) {
        unsigned char d = (unsigned char)s[2];
        if (d == 0x99) { *base = '\''; return 3; }
        if (d == 0xA6) { *base = '.'; *second = '.'; return 3; }
    }
    /* octets inconnus : on saute la séquence */
    *base = '?';
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static void draw_glyph(int x, int y, const u8 *cols, int accent, int skipDot, int upper, u16 c, int scale)
{
    int cx, r, i, j;
    for (cx = 0; cx < 5; cx++) {
        u8 b = cols[cx];
        for (r = 0; r < 8; r++) {
            if (!(b & (1 << r))) continue;
            if (skipDot && r == 0) continue;
            for (j = 0; j < scale; j++)
                for (i = 0; i < scale; i++) gfx_pixel(x + cx * scale + i, y + (1 + r) * scale + j, c);
        }
    }
#define PX(ax, ay) do { for (j = 0; j < scale; j++) for (i = 0; i < scale; i++) gfx_pixel(x + (ax) * scale + i, y + (ay) * scale + j, c); } while (0)
    if (accent == AC_CEDIL) {
        PX(2, 8);
        PX(1, 9);
    } else if (accent != AC_NONE) {
        if (upper) {
            if (accent == AC_ACUTE) { PX(2, 0); PX(3, 0); }
            else if (accent == AC_GRAVE) { PX(1, 0); PX(2, 0); }
            else { PX(1, 0); PX(2, 0); PX(3, 0); }
        } else {
            if (accent == AC_ACUTE) { PX(3, 1); PX(2, 2); }
            else if (accent == AC_GRAVE) { PX(1, 1); PX(2, 2); }
            else if (accent == AC_CIRC) { PX(2, 1); PX(1, 2); PX(3, 2); }
            else { PX(1, 2); PX(3, 2); }
        }
    }
#undef PX
}

static int text_draw(int x, int y, const char *s, u16 c, int scale)
{
    int px = x;
    while (*s) {
        int base, accent, second;
        const u8 *special;
        int n = decode_char(s, &base, &accent, &special, &second);
        if (special) {
            draw_glyph(px, y, special, AC_NONE, 0, 1, c, scale);
            px += 6 * scale;
        } else {
            int b = base;
            if (b < 32 || b > 126) b = '?';
            draw_glyph(px, y, font5x7[b - 32], accent, (accent != AC_NONE && base == 'i'), (base >= 'A' && base <= 'Z'), c, scale);
            px += 6 * scale;
            if (second) {
                draw_glyph(px, y, font5x7[second - 32], AC_NONE, 0, 0, c, scale);
                px += 6 * scale;
            }
        }
        s += n;
    }
    return px - x;
}

int gfx_text_width(const char *s)
{
    int w = 0;
    while (*s) {
        int base, accent, second;
        const u8 *special;
        int n = decode_char(s, &base, &accent, &special, &second);
        w += 6;
        if (second) w += 6;
        s += n;
    }
    return w;
}

void gfx_text(int x, int y, const char *s, u16 c) { text_draw(x, y, s, c, 1); }
void gfx_text_big(int x, int y, const char *s, u16 c, int scale) { text_draw(x, y, s, c, scale); }
void gfx_text_shadow(int x, int y, const char *s, u16 c, u16 sh)
{
    text_draw(x + 1, y + 1, s, sh, 1);
    text_draw(x, y, s, c, 1);
}
void gfx_text_center(int cx, int y, const char *s, u16 c) { text_draw(cx - gfx_text_width(s) / 2, y, s, c, 1); }
void gfx_text_right(int rx, int y, const char *s, u16 c) { text_draw(rx - gfx_text_width(s), y, s, c, 1); }

void gfx_printf(int x, int y, u16 c, const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    text_draw(x, y, buf, c, 1);
}
