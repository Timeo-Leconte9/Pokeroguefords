/* sprites.c : sprites 32x32 générés par code (aucun asset protégé).
 * Chaque espèce reçoit un archétype (corps), les couleurs de ses types et des détails propres à ses types.
 * Les espèces d'une même famille partagent leur graine : l'évolution reste reconnaissable. */
#include "game.h"

/* identifiants de région dans le canevas */
enum { R_NONE = 0, R_BASE, R_ACC, R_BELLY, R_WING, R_DARK, R_FLAME, R_FLAMEY, R_WHITE, R_COUNT };

static u8 cv[SPR][SPR];       /* régions */
static u16 outp[SPR][SPR];    /* image finale */
static int sc;                /* échelle du corps en % */
static u32 rs;                /* graine locale */
static int hx, hy, hrx, hry;  /* tête */
static int bx, by, brx, bry;  /* corps */
static int tx, ty;            /* bout de queue */
static int stageN;
static const Species *SP;
static int dexNo;

#define G 30
#define S(n) (((n) * sc + 50) / 100)

static int vr(int n)
{
    rs = rs * 1664525u + 1013904223u;
    return (int)((rs >> 16) % (u32)n);
}

static void P(int x, int y, int id)
{
    if ((unsigned)x < SPR && (unsigned)y < SPR) cv[y][x] = (u8)id;
}
static void E(int cx, int cy, int rx, int ry, int id)
{
    int x, y;
    if (rx < 1) rx = 1;
    if (ry < 1) ry = 1;
    for (y = -ry; y <= ry; y++)
        for (x = -rx; x <= rx; x++)
            if (x * x * ry * ry + y * y * rx * rx <= rx * rx * ry * ry + (rx * ry) / 2) P(cx + x, cy + y, id);
}
static void R(int x, int y, int w, int h, int id)
{
    int i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) P(x + i, y + j, id);
}
static int edge(int x0, int y0, int x1, int y1, int px, int py) { return (px - x0) * (y1 - y0) - (py - y0) * (x1 - x0); }
static void T(int x0, int y0, int x1, int y1, int x2, int y2, int id)
{
    int minx = x0, maxx = x0, miny = y0, maxy = y0, x, y;
    if (x1 < minx) minx = x1;
    if (x2 < minx) minx = x2;
    if (x1 > maxx) maxx = x1;
    if (x2 > maxx) maxx = x2;
    if (y1 < miny) miny = y1;
    if (y2 < miny) miny = y2;
    if (y1 > maxy) maxy = y1;
    if (y2 > maxy) maxy = y2;
    for (y = miny; y <= maxy; y++)
        for (x = minx; x <= maxx; x++) {
            int a = edge(x0, y0, x1, y1, x, y), b = edge(x1, y1, x2, y2, x, y), c = edge(x2, y2, x0, y0, x, y);
            if ((a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0)) P(x, y, id);
        }
}
static void L(int x0, int y0, int x1, int y1, int th, int id)
{
    int n = abs(x1 - x0) > abs(y1 - y0) ? abs(x1 - x0) : abs(y1 - y0);
    int i;
    if (n == 0) n = 1;
    for (i = 0; i <= n; i++) {
        int x = x0 + (x1 - x0) * i / n, y = y0 + (y1 - y0) * i / n;
        E(x, y, th / 2 > 0 ? th / 2 : 0, th / 2 > 0 ? th / 2 : 0, id);
        if (th <= 1) P(x, y, id);
    }
}

/* ------------------------------------------------------------ archétypes */
static void ears(int kind, int ex, int ey, int rx)
{
    /* kind : 0 rien, 1 pointues, 2 rondes, 3 longues */
    if (kind == 1) {
        T(ex - rx, ey, ex - rx - S(1), ey - S(6), ex - rx + S(4), ey - S(1), R_BASE);
        T(ex + rx, ey, ex + rx + S(1), ey - S(6), ex + rx - S(4), ey - S(1), R_BASE);
        P(ex - rx - S(1), ey - S(6), R_ACC);
        P(ex + rx + S(1), ey - S(6), R_ACC);
    } else if (kind == 2) {
        E(ex - rx + 1, ey - S(1), S(3), S(3), R_BASE);
        E(ex + rx - 1, ey - S(1), S(3), S(3), R_BASE);
        E(ex - rx + 1, ey - S(1), S(1), S(1), R_ACC);
        E(ex + rx - 1, ey - S(1), S(1), S(1), R_ACC);
    } else if (kind == 3) {
        T(ex - rx + S(1), ey, ex - rx - S(2), ey - S(10), ex - rx + S(4), ey - S(1), R_BASE);
        T(ex + rx - S(1), ey, ex + rx + S(2), ey - S(10), ex + rx - S(4), ey - S(1), R_BASE);
        T(ex - rx - S(2), ey - S(10), ex - rx - S(1), ey - S(6), ex - rx + S(1), ey - S(7), R_ACC);
        T(ex + rx + S(2), ey - S(10), ex + rx + S(1), ey - S(6), ex + rx - S(1), ey - S(7), R_ACC);
    }
}

static void arch_biped(int wings, int tall)
{
    int legH = S(tall ? 8 : 5), bry2 = S(tall ? 7 : 7), brx2 = S(tall ? 5 : 6) + vr(2), hry2 = S(tall ? 4 : 5) + 1, hrx2 = S(tall ? 5 : 6) + vr(2);
    int earKind = vr(4);
    int tailKind = vr(3);
    int bcy = G - legH - bry2 + 2;
    int hcy = bcy - bry2 - hry2 + 3;
    if (wings) {
        T(16 - brx2, bcy - bry2 + 1, 16 - brx2 - S(10), bcy - bry2 - S(7), 16 - brx2 - S(2), bcy + S(3), R_WING);
        T(16 + brx2, bcy - bry2 + 1, 16 + brx2 + S(10), bcy - bry2 - S(7), 16 + brx2 + S(2), bcy + S(3), R_WING);
    }
    /* queue */
    if (tailKind > 0 || SP->t1 == 1 || wings) {
        int tx0 = 16 + brx2 - 1, ty0 = G - legH - S(2);
        L(tx0, ty0, tx0 + S(5), ty0 + S(2), 3, R_BASE);
        L(tx0 + S(5), ty0 + S(2), tx0 + S(9), ty0 - S(4), 2, R_BASE);
        tx = tx0 + S(9);
        ty = ty0 - S(4);
    } else {
        tx = 16 + brx2 + 2;
        ty = bcy;
    }
    R(16 - S(5), G - legH, S(4) + 1, legH + 1, R_BASE);
    R(16 + S(1), G - legH, S(4) + 1, legH + 1, R_BASE);
    E(16 - S(4), G, S(3), 1, R_BASE);
    E(16 + S(4), G, S(3), 1, R_BASE);
    E(16 - brx2 - 1, bcy + 1, S(2), S(4) + (tall ? S(2) : 0), R_BASE);
    E(16 + brx2 + 1, bcy + 1, S(2), S(4) + (tall ? S(2) : 0), R_BASE);
    E(16, bcy, brx2, bry2, R_BASE);
    E(16, bcy + 1, brx2 - S(2), bry2 - S(2), R_BELLY);
    if (earKind) ears(earKind, 16, hcy - hry2 + 2, hrx2 - S(2));
    E(16, hcy, hrx2, hry2, R_BASE);
    hx = 16; hy = hcy; hrx = hrx2; hry = hry2;
    bx = 16; by = bcy; brx = brx2; bry = bry2;
}

static void arch_quad(void)
{
    int bodyCy = G - S(9), headCy = G - S(13);
    int earKind = vr(3) + 1;
    int legH = S(6);
    /* queue */
    L(16 + S(10), bodyCy - S(1), 16 + S(15), bodyCy - S(6), 3, R_BASE);
    tx = 16 + S(15); ty = bodyCy - S(6);
    E(tx, ty, S(2), S(2), R_ACC);
    R(16 - S(7), G - legH, S(3) + 1, legH + 1, R_BASE);
    R(16 - S(3), G - legH, S(3) + 1, legH + 1, R_DARK);
    R(16 + S(4), G - legH, S(3) + 1, legH + 1, R_DARK);
    R(16 + S(8), G - legH, S(3) + 1, legH + 1, R_BASE);
    E(16 + 2, bodyCy, S(10), S(6), R_BASE);
    E(16 + 2, bodyCy + S(2), S(7), S(3), R_BELLY);
    if (earKind) ears(earKind, 16 - S(8), headCy - S(4), S(2));
    E(16 - S(8), headCy, S(6), S(5), R_BASE);
    E(16 - S(12), headCy + S(2), S(3), S(2), R_BELLY);
    P(16 - S(15), headCy + S(1), R_DARK);
    hx = 16 - S(8); hy = headCy; hrx = S(6); hry = S(5);
    bx = 16 + 2; by = bodyCy; brx = S(10); bry = S(6);
}

static void arch_blob(void)
{
    int rx = S(10), ry = S(9);
    int earKind = vr(3);
    int cy = G - ry;
    E(16 - S(5), G, S(3), 1, R_BASE);
    E(16 + S(5), G, S(3), 1, R_BASE);
    if (earKind) ears(earKind, 16, cy - ry + S(3), S(5));
    E(16 - rx, cy + S(1), S(2), S(3), R_BASE);
    E(16 + rx, cy + S(1), S(2), S(3), R_BASE);
    E(16, cy, rx, ry, R_BASE);
    E(16, cy + S(3), S(6), S(5), R_BELLY);
    hx = 16; hy = cy - S(2); hrx = rx; hry = ry - S(2);
    bx = 16; by = cy; brx = rx; bry = ry;
    tx = 16 + rx; ty = cy;
    if (vr(2)) { T(16 - S(2), cy - ry, 16, cy - ry - S(5), 16 + S(2), cy - ry, R_ACC); }
}

static void arch_bird(void)
{
    int bcy = G - S(11);
    int crest = vr(2);
    /* queue (plumes) */
    T(16 + S(7), bcy - S(2), 16 + S(16), bcy - S(7), 16 + S(15), bcy + S(1), R_ACC);
    T(16 + S(7), bcy, 16 + S(15), bcy + S(3), 16 + S(8), bcy + S(4), R_WING);
    tx = 16 + S(15); ty = bcy - S(7);
    /* pattes */
    L(16 - S(1), bcy + S(5), 16 - S(1), G, 2, R_ACC);
    L(16 + S(3), bcy + S(5), 16 + S(3), G, 2, R_ACC);
    E(16 - S(3), G, S(2), 1, R_ACC);
    E(16 + S(1), G, S(2), 1, R_ACC);
    E(16 + 1, bcy, S(9), S(6), R_BASE);
    E(16 - S(2), bcy + S(3), S(5), S(3), R_BELLY);
    E(16 + S(3), bcy - S(1), S(6), S(3), R_WING);
    T(16 + S(8), bcy - S(1), 16 + S(11), bcy - S(4), 16 + S(7), bcy + S(3), R_WING);
    if (crest) T(16 - S(8), bcy - S(11), 16 - S(5), bcy - S(15), 16 - S(5), bcy - S(9), R_ACC);
    E(16 - S(8), bcy - S(5), S(5), S(5), R_BASE);
    T(16 - S(12), bcy - S(6), 16 - S(17), bcy - S(4), 16 - S(12), bcy - S(2), R_FLAMEY);
    hx = 16 - S(8); hy = bcy - S(5); hrx = S(5); hry = S(5);
    bx = 16; by = bcy; brx = S(9); bry = S(6);
}

static void arch_fish(void)
{
    int cy = G - S(12);
    T(16 + S(9), cy, 16 + S(16), cy - S(8), 16 + S(16), cy + S(8), R_ACC);
    T(16 - S(2), cy - S(6), 16 + S(5), cy - S(12), 16 + S(7), cy - S(5), R_ACC);
    T(16 - S(1), cy + S(5), 16 + S(4), cy + S(10), 16 + S(5), cy + S(5), R_ACC);
    E(16, cy, S(11), S(7), R_BASE);
    E(16 - S(2), cy + S(3), S(8), S(3), R_BELLY);
    E(16 + S(1), cy + S(1), S(3), S(2), R_WING);
    hx = 16 - S(6); hy = cy - S(1); hrx = S(5); hry = S(5);
    bx = 16; by = cy; brx = S(11); bry = S(7);
    tx = 16 + S(15); ty = cy;
}

static void arch_serpent(int big)
{
    static const int px[6] = {-7, -8, -3, 4, 11, 14};
    static const int py[6] = {17, 10, 5, 5, 6, 11};
    int i;
    int rr = big ? 5 : 4;
    for (i = 0; i < 5; i++) {
        int x0 = 16 + S(px[i]), y0 = G - S(py[i]), x1 = 16 + S(px[i + 1]), y1 = G - S(py[i + 1]);
        int th = (rr * 2 - i * (big ? 2 : 1)) * sc / 100;
        if (th < 2) th = 2;
        L(x0, y0, x1, y1, th + 1, R_BASE);
    }
    for (i = 1; i < 5; i++) {
        int x = 16 + S(px[i]), y = G - S(py[i]);
        E(x, y + 1, S(1), S(1), R_BELLY);
    }
    E(16 + S(px[0]) - 1, G - S(py[0]), S(6), S(5), R_BASE);
    if (vr(2)) { T(16 + S(px[0]) - S(5), G - S(py[0]) - S(3), 16 + S(px[0]) - S(7), G - S(py[0]) - S(8), 16 + S(px[0]), G - S(py[0]) - S(4), R_ACC); }
    hx = 16 + S(px[0]) - 1; hy = G - S(py[0]); hrx = S(6); hry = S(5);
    bx = 16 - S(3); by = G - S(6); brx = S(8); bry = S(4);
    tx = 16 + S(14); ty = G - S(11);
}

static void arch_plant(void)
{
    int cy = G - S(8);
    int ry = S(7), rx = S(7);
    /* pieds */
    R(16 - S(5), G - S(3), S(3) + 1, S(4), R_DARK);
    R(16 + S(2), G - S(3), S(3) + 1, S(4), R_DARK);
    /* feuilles */
    E(16 - S(9), cy - S(3), S(5), S(2), R_ACC);
    E(16 + S(9), cy - S(3), S(5), S(2), R_ACC);
    E(16, cy, rx, ry, R_BASE);
    E(16, cy + S(2), S(4), S(4), R_BELLY);
    /* fleur / bulbe */
    {
        int fy = cy - ry - S(2);
        int k;
        for (k = 0; k < 5; k++) {
            static const int ox[5] = {-5, -3, 0, 3, 5}, oy[5] = {0, -3, -4, -3, 0};
            E(16 + S(ox[k]), fy + S(oy[k]) + S(1), S(3), S(3), R_ACC);
        }
        E(16, fy, S(3), S(3), R_FLAMEY);
    }
    hx = 16; hy = cy; hrx = rx; hry = ry;
    bx = 16; by = cy; brx = rx; bry = ry;
    tx = 16 + S(8); ty = cy;
}

static void arch_insect(void)
{
    int cy = G - S(9);
    int i;
    /* pattes */
    for (i = 0; i < 3; i++) {
        L(16 - S(3) + i * S(4), cy + S(3), 16 - S(5) + i * S(5), G, 2, R_DARK);
    }
    /* ailes */
    E(16 - S(1), cy - S(8), S(5), S(5), R_WING);
    E(16 + S(4), cy - S(7), S(5), S(4), R_WING);
    E(16 + S(6), cy, S(8), S(5), R_BASE);
    E(16 + S(6), cy + S(2), S(5), S(2), R_ACC);
    E(16 - S(2), cy - S(1), S(5), S(5), R_BASE);
    E(16 - S(8), cy - S(2), S(4), S(4), R_BASE);
    L(16 - S(9), cy - S(5), 16 - S(12), cy - S(11), 1, R_DARK);
    L(16 - S(7), cy - S(5), 16 - S(8), cy - S(11), 1, R_DARK);
    hx = 16 - S(8); hy = cy - S(2); hrx = S(4); hry = S(4);
    bx = 16 + S(3); by = cy; brx = S(8); bry = S(5);
    tx = 16 + S(14); ty = cy;
}

static void arch_crab(void)
{
    int cy = G - S(8);
    int i;
    for (i = 0; i < 3; i++) {
        L(16 - S(4) + i * S(4), cy + S(3), 16 - S(8) + i * S(7), G, 2, R_DARK);
    }
    E(16, cy, S(8), S(6), R_BASE);
    E(16 - S(11), cy - S(5), S(4), S(4), R_ACC);
    E(16 + S(11), cy - S(5), S(4), S(4), R_ACC);
    T(16 - S(15), cy - S(8), 16 - S(11), cy - S(5), 16 - S(15), cy - S(2), R_NONE);
    T(16 + S(15), cy - S(8), 16 + S(11), cy - S(5), 16 + S(15), cy - S(2), R_NONE);
    L(16 - S(6), cy - S(3), 16 - S(9), cy - S(5), 3, R_BASE);
    L(16 + S(6), cy - S(3), 16 + S(9), cy - S(5), 3, R_BASE);
    hx = 16; hy = cy - S(1); hrx = S(7); hry = S(5);
    bx = 16; by = cy; brx = S(8); bry = S(6);
    tx = 16 + S(10); ty = cy;
}

static void arch_rock(void)
{
    int cy = G - S(8), i;
    E(16 - S(11), cy + S(1), S(4), S(5), R_BASE);
    E(16 + S(11), cy + S(1), S(4), S(5), R_BASE);
    E(16, cy, S(9), S(8), R_BASE);
    E(16, cy + S(5), S(6), S(3), R_DARK);
    for (i = 0; i < 4; i++) E(16 - S(6) + i * S(4), cy - S(7) + (i & 1), S(2), S(2), R_BASE);
    hx = 16; hy = cy - S(2); hrx = S(8); hry = S(6);
    bx = 16; by = cy; brx = S(9); bry = S(8);
    tx = 16 + S(10); ty = cy;
}

static void arch_ghost(void)
{
    int cy = G - S(14);
    int i;
    /* traînée de gaz */
    E(16 - S(5), cy + S(9), S(3), S(3), R_ACC);
    E(16 + S(5), cy + S(10), S(3), S(2), R_ACC);
    E(16, cy + S(11), S(4), S(2), R_ACC);
    for (i = 0; i < 3; i++) E(16 - S(8) + i * S(8), cy - S(8) + (i == 1 ? -2 : 1), S(3), S(3), R_ACC);
    E(16, cy, S(9), S(9), R_BASE);
    E(16 - S(11), cy + S(4), S(3), S(3), R_BASE);
    E(16 + S(11), cy + S(4), S(3), S(3), R_BASE);
    hx = 16; hy = cy; hrx = S(9); hry = S(9);
    bx = 16; by = cy; brx = S(9); bry = S(9);
    tx = 16 + S(11); ty = cy + S(4);
}

static void arch_ball(void)
{
    int cy = G - S(10);
    int variant = vr(3);
    E(16, cy, S(10), S(10), R_BASE);
    if (SP->t1 == 3 || variant == 0) E(16, cy + S(5), S(8), S(5), R_BELLY);
    if (dexNo == 81 || dexNo == 82) {   /* Magnéti : aimants latéraux */
        R(16 - S(14), cy - S(3), S(4), S(6), R_ACC);
        R(16 + S(10), cy - S(3), S(4), S(6), R_ACC);
    }
    hx = 16; hy = cy; hrx = S(10); hry = S(10);
    bx = 16; by = cy; brx = S(10); bry = S(10);
    tx = 16 + S(10); ty = cy;
}

static void arch_star(void)
{
    int cy = G - S(12), k;
    static const int ax[5] = {0, 11, 7, -7, -11}, ay[5] = {-12, -4, 10, 10, -4};
    for (k = 0; k < 5; k++) {
        int ex = 16 + S(ax[k]), ey = cy + S(ay[k]);
        T(16 - S(4), cy - S(3), ex, ey, 16 + S(4), cy + S(3), R_BASE);
        T(16 + S(4), cy - S(3), ex, ey, 16 - S(4), cy + S(3), R_BASE);
    }
    E(16, cy, S(5), S(5), R_BASE);
    E(16, cy, S(3), S(3), R_ACC);
    hx = 16; hy = cy; hrx = S(5); hry = S(5);
    bx = 16; by = cy; brx = S(8); bry = S(8);
    tx = 16 + S(11); ty = cy - S(4);
}

static void arch_shell(void)
{
    int cy = G - S(9);
    int i;
    for (i = 0; i < 4; i++) L(16 - S(9) + i * S(4), cy + S(5), 16 - S(10) + i * S(5), G, 2, R_BELLY);
    E(16 + S(2), cy - S(1), S(10), S(9), R_BASE);
    for (i = 0; i < 3; i++) L(16 + S(2) - S(6) + i * S(6), cy - S(7), 16 + S(2) - S(5) + i * S(6), cy + S(3), 1, R_ACC);
    E(16 - S(7), cy + S(3), S(5), S(5), R_BELLY);
    hx = 16 - S(7); hy = cy + S(3); hrx = S(5); hry = S(5);
    bx = 16 + S(2); by = cy - S(1); brx = S(10); bry = S(9);
    tx = 16 + S(11); ty = cy;
}

static void arch_jelly(void)
{
    int cy = G - S(17), i;
    for (i = 0; i < 5; i++) {
        int x = 16 - S(8) + i * S(4);
        L(x, cy + S(4), x + ((i & 1) ? 1 : -1), G, 3, R_ACC);
    }
    E(16, cy, S(10), S(8), R_BASE);
    E(16 - S(5), cy - S(1), S(2), S(2), R_FLAME);
    E(16 + S(5), cy - S(1), S(2), S(2), R_FLAME);
    E(16, cy - S(5), S(2), S(2), R_FLAME);
    hx = 16; hy = cy + S(1); hrx = S(9); hry = S(6);
    bx = 16; by = cy; brx = S(10); bry = S(8);
    tx = 16 + S(10); ty = cy;
}

/* ------------------------------------------------------------ détails de type */
static void decorate(int t, int strong)
{
    int i;
    switch (t) {
    case 1: /* feu : flamme au bout de la queue / sur la tête */
        E(tx, ty - S(3), S(3), S(3), R_FLAME);
        T(tx - S(2), ty - S(3), tx, ty - S(9), tx + S(2), ty - S(3), R_FLAME);
        E(tx, ty - S(2), S(1), S(2), R_FLAMEY);
        break;
    case 2: /* eau : nageoire sur la tête */
        T(hx - S(2), hy - hry + 1, hx, hy - hry - S(5), hx + S(3), hy - hry + 1, R_ACC);
        break;
    case 3: /* électrik : queue en zigzag */
        if (strong) {
            int x = tx, y = ty;
            L(x, y, x + S(3), y - S(4), 2, R_FLAMEY);
            L(x + S(3), y - S(4), x + S(1), y - S(5), 2, R_FLAMEY);
            L(x + S(1), y - S(5), x + S(5), y - S(10), 2, R_FLAMEY);
        }
        break;
    case 4: /* plante : feuille sur la tête */
        if (SP->arch != 'P' && SP->arch != 'Q') {
            T(hx - S(1), hy - hry + 1, hx + S(5), hy - hry - S(6), hx + S(7), hy - hry + 1, R_ACC);
        } else if (SP->arch == 'Q') {
            E(bx, by - bry - S(2), S(5), S(4), R_ACC);
            E(bx, by - bry - S(2), S(2), S(2), R_BELLY);
        }
        break;
    case 5: /* glace : cristaux */
        T(hx - S(3), hy - hry + 2, hx - S(2), hy - hry - S(5), hx - S(1), hy - hry + 2, R_WHITE);
        T(hx + S(1), hy - hry + 2, hx + S(2), hy - hry - S(6), hx + S(3), hy - hry + 2, R_WHITE);
        break;
    case 6: /* combat : bandeau */
        if (strong) R(hx - hrx + 1, hy - hry / 2, hrx * 2 - 1, 2, R_FLAME);
        break;
    case 7: /* poison : épines violettes dans le dos */
        for (i = 0; i < 3; i++) T(bx - brx + S(3) + i * S(4), by - bry + 1, bx - brx + S(5) + i * S(4), by - bry - S(4), bx - brx + S(7) + i * S(4), by - bry + 1, R_ACC);
        break;
    case 8: /* sol : bosses de terre */
        E(bx + S(3), by + S(2), S(2), S(1), R_DARK);
        E(bx - S(4), by + S(3), S(1), S(1), R_DARK);
        break;
    case 9: /* vol : petite aile */
        if (SP->arch != 'W') {
            E(bx + brx - S(1), by - S(2), S(2), S(5), R_WING);
            E(bx - brx + S(1), by - S(2), S(2), S(5), R_WING);
        }
        break;
    case 10: /* psy : gemme */
        E(hx, hy - hry + S(2), S(1), S(2), R_WHITE);
        break;
    case 11: /* insecte : antennes */
        if (SP->arch != 'I') {
            L(hx - S(2), hy - hry + 1, hx - S(4), hy - hry - S(5), 1, R_DARK);
            L(hx + S(2), hy - hry + 1, hx + S(4), hy - hry - S(5), 1, R_DARK);
        }
        break;
    case 12: /* roche : bosses */
        E(bx - S(3), by - bry + S(2), S(2), S(2), R_DARK);
        E(bx + S(3), by - bry + S(3), S(2), S(2), R_DARK);
        break;
    case 14: /* dragon : cornes */
        T(hx - S(4), hy - hry + 2, hx - S(5), hy - hry - S(6), hx - S(2), hy - hry + 1, R_WHITE);
        T(hx + S(2), hy - hry + 1, hx + S(5), hy - hry - S(6), hx + S(4), hy - hry + 2, R_WHITE);
        break;
    case 15: /* ténèbres : pointes sombres */
        T(hx - S(4), hy - hry + 2, hx - S(3), hy - hry - S(5), hx - S(1), hy - hry + 2, R_DARK);
        T(hx + S(1), hy - hry + 2, hx + S(3), hy - hry - S(5), hx + S(4), hy - hry + 2, R_DARK);
        break;
    case 16: /* acier : plaque */
        R(bx - S(3), by - S(1), S(6), S(3), R_WHITE);
        break;
    case 17: /* fée : ruban */
        E(hx + S(5), hy - hry + S(1), S(2), S(2), R_ACC);
        E(hx + S(8), hy - hry + S(2), S(2), S(1), R_ACC);
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------ composition */
static u16 makeRgb(int r, int g, int b) { return COL(r, g, b); }

static void compose(u16 *pal, int back)
{
    int x, y;
    u16 outline = gfx_darken(pal[R_BASE], 190);
    (void)back;
    for (y = 0; y < SPR; y++)
        for (x = 0; x < SPR; x++) {
            int id = cv[y][x];
            u16 c;
            if (!id) { outp[y][x] = 0; continue; }
            c = pal[id];
            if (id != R_WHITE && id != R_FLAMEY && id != R_FLAME) {
                int top = (y == 0 || cv[y - 1][x] != id) || (x == 0 || cv[y][x - 1] != id);
                int bot = (y == SPR - 1 || cv[y + 1][x] != id) || (x == SPR - 1 || cv[y][x + 1] != id);
                if (top && !bot) c = gfx_lighten(c, 55);
                else if (bot && !top) c = gfx_darken(c, 60);
                else if (top && bot) c = gfx_mix(c, pal[R_BASE], 64);
                else if (y > 14 && x > 16) c = gfx_darken(c, 22);
            }
            outp[y][x] = c;
        }
    for (y = 0; y < SPR; y++)
        for (x = 0; x < SPR; x++) {
            if (cv[y][x]) continue;
            if ((x > 0 && cv[y][x - 1]) || (x < SPR - 1 && cv[y][x + 1]) || (y > 0 && cv[y - 1][x]) || (y < SPR - 1 && cv[y + 1][x]))
                outp[y][x] = outline;
        }
}

static void ov(int x, int y, u16 c)
{
    if ((unsigned)x < SPR && (unsigned)y < SPR && cv[y][x]) outp[y][x] = c;
}
static void ovr(int x, int y, int w, int h, u16 c)
{
    int i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) ov(x + i, y + j, c);
}

static void face(int back)
{
    char a = SP->arch;
    u16 white = COL(255, 255, 255), black = COL(24, 24, 40);
    int side = (a == 'Q' || a == 'W' || a == 'F' || a == 'S');
    int angry = (SP->t1 == 6 || SP->t1 == 15 || SP->t1 == 7 || SP->t1 == 14 || SP->t1 == 13 || SP->t1 == 12);
    int ey = hy - 1 + (a == 'Q' ? 0 : 0);
    if (back) return;
    if (a == 'T' || a == 'E' || (a == 'L' && (dexNo == 81 || dexNo == 82))) {
        /* un seul grand oeil central */
        int cx = hx, cy = hy;
        if (a == 'T') { ovr(cx - 1, cy - 1, 3, 3, white); ov(cx, cy, black); return; }
        ovr(cx - 2, cy - 2, 5, 5, white);
        ovr(cx - 1, cy - 1, 3, 3, black);
        ov(cx - 1, cy - 1, white);
        return;
    }
    if (a == 'G') {
        /* gros yeux + bouche */
        ovr(hx - 5, hy - 3, 4, 3, white);
        ovr(hx + 2, hy - 3, 4, 3, white);
        ovr(hx - 4, hy - 2, 2, 2, COL(220, 40, 60));
        ovr(hx + 3, hy - 2, 2, 2, COL(220, 40, 60));
        ovr(hx - 4, hy + 2, 9, 3, black);
        ovr(hx - 3, hy + 3, 7, 1, white);
        return;
    }
    if (side) {
        int ex = hx - hrx / 3 - 1;
        ovr(ex, ey, 2, 3, white);
        ovr(ex, ey + 1, 2, 2, black);
        ov(ex, ey + 1, white);
        if (angry) { ovr(ex - 1, ey - 1, 4, 1, black); }
        if (a == 'Q' || a == 'W') ovr(hx - hrx / 2 - 3, hy + hry / 2, 3, 1, black);
        if (a == 'F' || a == 'S') ovr(hx - hrx + 1, hy + hry / 2, 4, 1, black);
        return;
    }
    {
        int dx = hrx / 2;
        if (dx < 2) dx = 2;
        if (dx > 6) dx = 6;
        ovr(hx - dx - 1, ey, 3, 3, white);
        ovr(hx + dx - 1, ey, 3, 3, white);
        ovr(hx - dx, ey + 1, 2, 2, black);
        ovr(hx + dx - 1, ey + 1, 2, 2, black);
        ov(hx - dx, ey + 1, white);
        ov(hx + dx - 1, ey + 1, white);
        if (angry) {
            ovr(hx - dx - 2, ey - 1, 4, 1, black);
            ovr(hx + dx - 1, ey - 1, 4, 1, black);
        }
        ovr(hx - 1, hy + hry / 2 + 1, 3, 1, black);
        if (SP->t1 == 3 || SP->t2 == 3 || SP->t1 == 17) {
            ovr(hx - dx - 3, ey + 3, 2, 2, COL(240, 70, 70));
            ovr(hx + dx + 2, ey + 3, 2, 2, COL(240, 70, 70));
        }
    }
}

static int stage_of(int d)
{
    int s = 0, k, found = 1;
    while (found) {
        found = 0;
        for (k = 1; k <= SPECIES_COUNT; k++)
            if (species[k].evoTo == d) { d = k; s++; found = 1; break; }
    }
    return s;
}
static int root_of(int d)
{
    int k, found = 1;
    while (found) {
        found = 0;
        for (k = 1; k <= SPECIES_COUNT; k++)
            if (species[k].evoTo == d) { d = k; found = 1; break; }
    }
    return d;
}

static int bst_of(const Species *s)
{
    return s->st[0] + s->st[1] + s->st[2] + s->st[3] + s->st[4] + s->st[5];
}

static void generate(int d, int back, int shiny, u16 *dst)
{
    u16 pal[R_COUNT];
    u16 base, acc;
    int bst, i, j;
    SP = &species[d];
    dexNo = d;
    memset(cv, 0, sizeof(cv));
    memset(outp, 0, sizeof(outp));
    bst = bst_of(SP);
    sc = 60 + ((bst - 200) * 40) / 330;
    if (sc < 56) sc = 56;
    if (sc > 100) sc = 100;
    if (SP->arch == 'F' || SP->arch == 'S') { if (d == 130) sc = 100; }
    stageN = stage_of(d);
    rs = (u32)root_of(d) * 2654435761u + 12345u;
    vr(7);

    base = SP->color ? SP->color : typeColors[SP->t1];
    if (shiny) base = (u16)(RGB15((base >> 10) & 31, base & 31, (base >> 5) & 31) | BIT(15));
    acc = (SP->t2 != NO_TYPE) ? typeColors[SP->t2] : gfx_darken(base, 80);
    if (SP->t2 != NO_TYPE && SP->color && SP->t2 == SP->t1) acc = gfx_darken(base, 80);
    if (shiny) acc = (u16)(RGB15((acc >> 10) & 31, acc & 31, (acc >> 5) & 31) | BIT(15));
    pal[R_NONE] = 0;
    pal[R_BASE] = base;
    pal[R_ACC] = acc;
    pal[R_BELLY] = back ? gfx_darken(base, 30) : gfx_mix(base, rgb(250, 238, 200), SP->t1 == 0 ? 200 : 150);
    pal[R_WING] = gfx_lighten(SP->t2 != NO_TYPE ? acc : base, 70);
    pal[R_DARK] = gfx_darken(base, 100);
    pal[R_FLAME] = rgb(240, 110, 30);
    pal[R_FLAMEY] = rgb(255, 220, 70);
    pal[R_WHITE] = rgb(240, 248, 255);
    if (SP->t1 == 2 || SP->t1 == 5) pal[R_FLAME] = rgb(120, 190, 255);
    if (SP->t1 == 6) pal[R_FLAME] = rgb(210, 40, 40);
    if (SP->t1 == 4) pal[R_FLAMEY] = rgb(250, 220, 90);
    if (SP->t1 == 1) pal[R_FLAMEY] = rgb(255, 230, 90);
    if (SP->t1 == 10) pal[R_FLAMEY] = rgb(250, 160, 200);

    switch (SP->arch) {
    case 'B': arch_biped(0, 0); break;
    case 'D': arch_biped(1, 0); break;
    case 'H': arch_biped(0, 1); break;
    case 'Q': arch_quad(); break;
    case 'O': arch_blob(); break;
    case 'W': arch_bird(); break;
    case 'F': arch_fish(); break;
    case 'S': arch_serpent(d == 130 || d == 95 || d == 149 || d == 148); break;
    case 'P': arch_plant(); break;
    case 'I': if (d == 98 || d == 99) arch_crab(); else arch_insect(); break;
    case 'R': arch_rock(); break;
    case 'G': arch_ghost(); break;
    case 'L': arch_ball(); break;
    case 'T': arch_star(); break;
    case 'E': arch_shell(); break;
    case 'J': arch_jelly(); break;
    default: arch_blob(); break;
    }
    decorate(SP->t1, 1);
    if (SP->t2 != NO_TYPE && SP->t2 != 9 && vr(2)) decorate(SP->t2, 0);
    if (SP->t2 == 9 && SP->arch != 'W') decorate(9, 0);

    /* motif : rayures ou taches */
    {
        int pat = vr(5);
        if (pat == 0 && SP->arch != 'T') {
            for (i = 0; i < 3; i++)
                for (j = 0; j < SPR; j++)
                    if (cv[j][bx - brx / 2 + i * 4] == R_BASE && j > by - bry + 1 && j < by + bry - 3 && (j & 3) < 2) cv[j][bx - brx / 2 + i * 4] = R_ACC;
        } else if (pat == 1) {
            E(bx - brx / 2, by - 1, 1, 1, R_DARK);
            E(bx + brx / 3, by + 1, 1, 1, R_DARK);
            E(bx, by - bry / 2, 1, 1, R_DARK);
        }
    }
    compose(pal, back);
    face(back);
    for (j = 0; j < SPR; j++)
        for (i = 0; i < SPR; i++) dst[j * SPR + i] = outp[j][i];
}

/* ------------------------------------------------------------ cache */
static u16 *cache[2][2][SPECIES_COUNT + 1];

const u16 *sprite_get(int d, int back, int shiny)
{
    u16 **slot;
    if (d < 1 || d > SPECIES_COUNT) d = 1;
    back = back ? 1 : 0;
    shiny = shiny ? 1 : 0;
    slot = &cache[back][shiny][d];
    if (!*slot) {
        *slot = (u16 *)malloc(SPR * SPR * 2);
        if (!*slot) {
            static u16 empty[SPR * SPR];
            return empty;
        }
        generate(d, back, shiny, *slot);
    }
    return *slot;
}

void sprite_draw(int d, int back, int shiny, int cx, int baseY, int scale, int white)
{
    gfx_blit(sprite_get(d, back, shiny), SPR, SPR, cx - 16 * scale, baseY - 31 * scale, scale, back ? 1 : 0, white);
}

void sprite_draw_icon(int d, int x, int y) { gfx_blit(sprite_get(d, 0, 0), SPR, SPR, x, y, 1, 0, 0); }

/* dresseur : silhouette simple */
void draw_trainer(int cx, int baseY, int scale, int seed)
{
    static u16 buf[SPR * SPR];
    u16 skin = COL(240, 200, 160);
    u16 hair[4] = {COL(60, 40, 30), COL(200, 150, 60), COL(40, 40, 70), COL(180, 60, 40)};
    u16 shirt[4] = {COL(220, 60, 60), COL(60, 120, 220), COL(80, 180, 90), COL(160, 90, 200)};
    u16 pants = COL(50, 60, 100);
    int x, y;
    memset(buf, 0, sizeof(buf));
#define PIX(px, py, c) do { if ((unsigned)(px) < SPR && (unsigned)(py) < SPR) buf[(py) * SPR + (px)] = (c); } while (0)
    for (y = 0; y < 31; y++)
        for (x = 0; x < SPR; x++) {
            int dx = x - 16;
            u16 c = 0;
            if (y >= 3 && y <= 11 && dx * dx + (y - 7) * (y - 7) * 2 <= 40) c = skin;           /* tête */
            if (y >= 2 && y <= 6 && dx * dx + (y - 6) * (y - 6) * 3 <= 44) c = hair[seed & 3];    /* cheveux */
            if (y >= 12 && y <= 22 && dx >= -5 && dx <= 5) c = shirt[(seed >> 2) & 3];           /* torse */
            if (y >= 13 && y <= 21 && (dx == -7 || dx == -6 || dx == 6 || dx == 7)) c = shirt[(seed >> 2) & 3];
            if (y >= 20 && y <= 22 && (dx == -7 || dx == 7)) c = skin;
            if (y >= 23 && y <= 30 && ((dx >= -5 && dx <= -1) || (dx >= 1 && dx <= 5))) c = pants;
            if (c) PIX(x, y, c | BIT(15));
        }
    PIX(14, 7, COL(20, 20, 30) | BIT(15));
    PIX(18, 7, COL(20, 20, 30) | BIT(15));
    if (seed & 8)
        for (x = 11; x <= 21; x++) PIX(x, 3, COL(220, 40, 40) | BIT(15));
#undef PIX
    gfx_blit(buf, SPR, SPR, cx - 16 * scale, baseY - 31 * scale, scale, 0, 0);
}
