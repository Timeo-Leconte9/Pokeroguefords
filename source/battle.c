
/* battle.c : Pokémon (stats, attaques apprises), formules de combat, statuts, capture, expérience, évolutions */
#include "game.h"

#define EXP_MULT 3
#define MAX_LEVEL 250

static const u8 natStat[5] = {STAT_ATK, STAT_DEF, STAT_SPE, STAT_SPA, STAT_SPD};
static const char *const BALL_NAMES[5] = {"Poké Ball", "Super Ball", "Hyper Ball", "Rogue Ball", "Master Ball"};

/* ------------------------------------------------------------ talents (simplifiés, déduits du type) */
enum { AB_NONE, AB_BLAZE, AB_TORRENT, AB_OVERGROW, AB_INTIMIDATE, AB_LEVITATE, AB_STATIC, AB_FLAMEBODY,
       AB_POISONPOINT, AB_STURDY, AB_GUTS, AB_COUNT };
static const char *const abilityNames[AB_COUNT] = {"-", "Brasier", "Torrent", "Engrais", "Intimidation", "Lévitation", "Statik",
                                                   "Corps Ardent", "Point Poison", "Fermeté", "Cran"};
const char *ability_name(int ab) { return abilityNames[ab < 0 || ab >= AB_COUNT ? 0 : ab]; }

int mon_ability(const Mon *m)
{
    const Species *s = &species[m->species];
    int d = m->species;
    if (d == 77 || d == 78 || d == 126) return AB_FLAMEBODY;
    if (d == 92 || d == 93 || d == 94 || d == 109 || d == 110 || d == 81 || d == 82) return AB_LEVITATE;
    if (s->legend) return AB_INTIMIDATE;
    switch (s->t1) {
    case 1: return AB_BLAZE;
    case 2: return AB_TORRENT;
    case 4: return AB_OVERGROW;
    case 3: return AB_STATIC;
    case 13: return AB_LEVITATE;
    case 7: return AB_POISONPOINT;
    case 8: case 12: case 16: return AB_STURDY;
    case 0: case 6: return AB_GUTS;
    case 14: case 15: return AB_INTIMIDATE;
    default: return AB_NONE;
    }
}

/* ------------------------------------------------------------ Méga / Dynamax / Téra */
static int mt1(const Mon *m) { return m->tOv ? m->tmpT1 : species[m->species].t1; }
static int mt2(const Mon *m) { return m->tOv ? m->tmpT2 : species[m->species].t2; }

typedef struct { u8 dex; const char *name; u8 t1, t2; u8 st[6]; } MegaDef;
/* types : 0 Normal 1 Feu 2 Eau 3 Élec 4 Plante 5 Glace 6 Combat 7 Poison 8 Sol 9 Vol
   10 Psy 11 Insecte 12 Roche 13 Spectre 14 Dragon 15 Ténèbres 16 Acier 17 Fée (index de typeNames) */
static const MegaDef megaDefs[] = {
    {3,   "Méga-Florizarre", 4, 7, {80, 100, 123, 122, 120, 80}},
    {6,   "Méga-Dracaufeu X", 1, 14, {78, 130, 111, 130, 85, 100}},
    {6,   "Méga-Dracaufeu Y", 1, 9, {78, 104, 78, 159, 115, 100}},
    {9,   "Méga-Tortank", 2, NO_TYPE, {79, 103, 120, 135, 115, 78}},
    {15,  "Méga-Dardargnan", 11, 7, {65, 150, 40, 15, 80, 145}},
    {18,  "Méga-Roucarnage", 0, 9, {83, 80, 80, 135, 80, 121}},
    {65,  "Méga-Alakazam", 10, NO_TYPE, {55, 50, 65, 175, 105, 150}},
    {80,  "Méga-Flagadoss", 2, 10, {95, 75, 180, 130, 80, 30}},
    {94,  "Méga-Ectoplasma", 13, 7, {60, 65, 80, 170, 95, 130}},
    {115, "Méga-Kangourex", 0, NO_TYPE, {105, 125, 100, 60, 100, 100}},
    {127, "Méga-Scarabrute", 11, 9, {65, 155, 120, 65, 90, 105}},
    {130, "Méga-Léviator", 2, 15, {95, 155, 109, 70, 130, 81}},
    {142, "Méga-Ptéra", 12, 9, {80, 135, 85, 70, 95, 150}},
    {150, "Méga-Mewtwo X", 10, 6, {106, 190, 100, 154, 100, 130}},
    {150, "Méga-Mewtwo Y", 10, NO_TYPE, {106, 150, 70, 194, 120, 140}},
};
#define MEGA_COUNT ((int)(sizeof(megaDefs) / sizeof(megaDefs[0])))
static u8 gimUsed[3];
static const MegaDef *megaPick;

static const MegaDef *mega_for(const Mon *m)
{
    int i, best = -1;
    for (i = 0; i < MEGA_COUNT; i++) {
        if (megaDefs[i].dex != m->species) continue;
        /* deux formes : choisit celle qui a la meilleure attaque offensive */
        if (best < 0) best = i;
        else {
            int a = m->stat[STAT_ATK] >= m->stat[STAT_SPA];
            if ((a && megaDefs[i].st[1] > megaDefs[best].st[1]) || (!a && megaDefs[i].st[3] > megaDefs[best].st[3])) best = i;
        }
    }
    return best < 0 ? 0 : &megaDefs[best];
}

static int calc_stat_base(int idx, int b, const Mon *m);

static void gim_revert(Mon *m)
{
    int i;
    if (!m->gim) return;
    if (m->gim == 1) { for (i = 1; i < 6; i++) m->stat[i] = m->bakStat[i]; }
    else if (m->gim == 2) {
        int dbl = m->stat[0];
        m->stat[0] = m->bakStat[0];
        if (m->hp > 0) { m->hp = (m->hp * m->stat[0] + dbl - 1) / dbl; if (m->hp < 1) m->hp = 1; }
        if (m->hp > m->stat[0]) m->hp = m->stat[0];
    }
    m->tOv = 0; m->gim = 0; m->dynaTurns = 0;
}

static void gim_apply(Mon *m, int g, int teraType)
{
    int i;
    for (i = 0; i < 6; i++) m->bakStat[i] = m->stat[i];
    m->gim = (u8)(g + 1);
    if (g == 0) {
        const MegaDef *d = megaPick;
        for (i = 1; i < 6; i++) m->stat[i] = calc_stat_base(i, d->st[i], m);
        m->tOv = 1; m->tmpT1 = d->t1; m->tmpT2 = d->t2;
    } else if (g == 1) {
        m->bakHp = m->hp;
        m->stat[0] *= 2; m->hp *= 2;
        m->dynaTurns = 3;
    } else {
        m->tOv = 1; m->tmpT1 = (u8)teraType; m->tmpT2 = NO_TYPE;
    }
}

/* ------------------------------------------------------------ utilitaires */
static u32 isqrt64(unsigned long long v)
{
    unsigned long long r = 0, b = 1ULL << 62;
    while (b > v) b >>= 2;
    while (b) {
        if (v >= r + b) { v -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
        b >>= 2;
    }
    return (u32)r;
}

const char *mon_name(const Mon *m) { return species[m->species].name; }
int mon_alive(const Mon *m) { return m->hp > 0; }

static int calc_stat_base(int idx, int b, const Mon *m)
{
    int iv = m->iv[idx], L = m->level;
    int v;
    if (idx == STAT_HP) return ((2 * b + iv) * L) / 100 + L + 10;
    v = ((2 * b + iv) * L) / 100 + 5;
    {
        int up = m->nature / 5, down = m->nature % 5;
        if (up != down) {
            if (natStat[up] == idx) v = v * 110 / 100;
            else if (natStat[down] == idx) v = v * 90 / 100;
        }
    }
    return v;
}

int mon_calc_stat(int idx, const Species *sp, const Mon *m) { return calc_stat_base(idx, sp->st[idx], m); }

void mon_recalc(Mon *m)
{
    const Species *sp = &species[m->species];
    int i, oldMax;
    if (m->gim) gim_revert(m);
    oldMax = m->stat[0];
    for (i = 0; i < 6; i++) m->stat[i] = mon_calc_stat(i, sp, m);
    if (oldMax > 0 && m->hp > 0) m->hp += m->stat[0] - oldMax;
    if (m->hp > m->stat[0]) m->hp = m->stat[0];
    if (m->hp < 0) m->hp = 0;
}

int mon_exp_for(int level, int sp)
{
    int n3;
    if (level <= 0) return 0;
    n3 = level * level * level;
    if (sp >= 1 && sp <= SPECIES_COUNT && species[sp].legend) n3 = n3 * 5 / 4;
    return n3;
}

/* ------------------------------------------------------------ apprentissage */
typedef struct { u8 mv, lv; } LE;

static void add_le(LE *le, int *n, int mv, int lv)
{
    int i, j;
    if (*n >= 28) return;
    for (i = 0; i < *n; i++)
        if (le[i].mv == mv) return;
    for (i = 0; i < *n && le[i].lv <= lv; i++) { }
    for (j = *n; j > i; j--) le[j] = le[j - 1];
    le[i].mv = (u8)mv;
    le[i].lv = (u8)lv;
    (*n)++;
}

static int is_type(const Species *s, int t) { return s->t1 == t || s->t2 == t; }

static int build_learnset(int sp, LE *le)
{
    const Species *s = &species[sp];
    int n = 0, ci = (s->st[1] >= s->st[3]) ? 0 : 1, k, c;
    static const u8 covers[10] = {0, 5, 6, 8, 10, 12, 13, 3, 1, 2};
    int phys = (ci == 0);
#define TM(t, tier) tierMoves[t][tier][ci]
    if (sp == 129) { add_le(le, &n, M_TREMPETTE, 1); add_le(le, &n, M_CHARGE, 15); return n; }
    if (sp == 132) { add_le(le, &n, M_CHARGE, 1); return n; }
    add_le(le, &n, (sp & 1) ? M_CHARGE : M_GRIFFE, 1);
    add_le(le, &n, TM(s->t1, 0), 1);
    add_le(le, &n, TM(s->t1, 1), 16);
    add_le(le, &n, TM(s->t1, 2), 34);
    add_le(le, &n, TM(s->t1, 3), 52);
    if (s->t2 != NO_TYPE) {
        add_le(le, &n, TM(s->t2, 0), 6);
        add_le(le, &n, TM(s->t2, 1), 21);
        add_le(le, &n, TM(s->t2, 2), 38);
        add_le(le, &n, TM(s->t2, 3), 56);
    }
    /* couverture */
    c = covers[(sp * 7 + 3) % 10];
    for (k = 0; k < 10 && (c == s->t1 || c == s->t2); k++) c = covers[(sp * 7 + 4 + k) % 10];
    add_le(le, &n, TM(c, 1), 24);
    add_le(le, &n, TM(c, 2), 44);
    add_le(le, &n, tierMoves[0][1][ci], 28);
    /* soutien selon types et stats */
    if (is_type(s, 4)) { add_le(le, &n, M_POUDRE_DODO, 10); add_le(le, &n, M_SPORE, 30); }
    if (is_type(s, 3)) add_le(le, &n, M_CAGE_ECLAIR, 12);
    if (is_type(s, 7)) add_le(le, &n, M_TOXIK, 14);
    if (is_type(s, 1)) add_le(le, &n, M_FEU_FOLLET, 14);
    if (is_type(s, 10)) { add_le(le, &n, M_HYPNOSE, 18); add_le(le, &n, M_REPOS, 27); }
    if (is_type(s, 13)) { add_le(le, &n, M_HYPNOSE, 20); add_le(le, &n, M_TENEBRES, 26);
    if (is_type(s, 17)) add_le(le, &n, M_BERCEUSE, 14);
    if (is_type(s, 14)) add_le(le, &n, M_DANSE_DRACO, 40);
    if (is_type(s, 16)) add_le(le, &n, M_MUR_DE_FER, 30);
    if (is_type(s, 15)) add_le(le, &n, M_MACHINATION, 36);
    if (is_type(s, 11)) add_le(le, &n, M_SECRETION, 4);
    if (is_type(s, 6)) add_le(le, &n, M_DANSE_LAMES, 32);
    if (is_type(s, 9) && s->st[5] >= 80) add_le(le, &n, M_HATE, 33);
    if (phys && s->st[1] >= 100) add_le(le, &n, M_DANSE_LAMES, 30);
    if (!phys && s->st[3] >= 100) add_le(le, &n, M_CALME_ESPRIT, 30);
    if (s->st[5] >= 105) add_le(le, &n, M_HATE, 34);
    if (s->st[0] >= 95) add_le(le, &n, M_SOIN, 32);
    add_le(le, &n, M_RUGISSEMENT, 8);
    add_le(le, &n, M_MIMI_QUEUE, 11);
    if (sp == 100 || sp == 101 || sp == 110) add_le(le, &n, M_EXPLOSION, 40);
    if (sp == 74 || sp == 75 || sp == 76 || sp == 109) add_le(le, &n, M_DESTRUCTION, 32);
    if (sp == 21 || sp == 22 || sp == 16) add_le(le, &n, M_VIVE_ATTAQUE, 5);
    if (sp == 19 || sp == 20 || sp == 52 || sp == 53) add_le(le, &n, M_VIVE_ATTAQUE, 7);
    if (sp == 39 || sp == 40) add_le(le, &n, M_BERCEUSE, 1);
    if (sp == 41 || sp == 42) add_le(le, &n, M_ULTRASON, 10);
    if (sp == 133 || sp == 134 || sp == 135 || sp == 136) add_le(le, &n, M_VIVE_ATTAQUE, 8);
#undef TM
    return n;
}

void mon_fill_moves(Mon *m)
{
    LE le[28];
    int n = build_learnset(m->species, le), i, cnt = 0, sel[28];
    int dam = 0;
    for (i = 0; i < n; i++)
        if (le[i].lv <= m->level) sel[cnt++] = le[i].mv;
    for (i = 0; i < 4; i++) m->moves[i] = 255;
    {
        int start = cnt > 4 ? cnt - 4 : 0;
        for (i = start; i < cnt; i++) m->moves[i - start] = (u8)sel[i];
    }
    for (i = 0; i < 4; i++)
        if (m->moves[i] != 255 && moves[m->moves[i]].cat != CAT_STAT) dam = 1;
    if (!dam) m->moves[0] = M_CHARGE;
    for (i = 0; i < 4; i++)
        if (m->moves[i] != 255 && m->pp[i] == 0) m->pp[i] = moves[m->moves[i]].pp;
}
