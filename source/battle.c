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
    if (is_type(s, 13)) { add_le(le, &n, M_HYPNOSE, 20); add_le(le, &n, M_TENEBRES, 26); }
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
    for (i = 0; i < 4; i++) {
        if (m->moves[i] == 255) { m->pp[i] = m->ppmax[i] = 0; continue; }
        m->pp[i] = m->ppmax[i] = moves[m->moves[i]].pp;
    }
}

int mon_can_learn_at(int sp, int level, int *mv)
{
    LE le[28];
    int n = build_learnset(sp, le), i;
    for (i = 0; i < n; i++)
        if (le[i].lv == level) { *mv = le[i].mv; return 1; }
    return 0;
}

void mon_init(Mon *m, int sp, int level)
{
    int i;
    memset(m, 0, sizeof(*m));
    m->species = (u8)sp;
    m->level = level;
    m->nature = (u8)rnd(25);
    for (i = 0; i < 6; i++) m->iv[i] = (u8)rnd(32);
    m->shiny = (rnd(128) == 0);
    m->exp = mon_exp_for(level, sp);
    m->hp = 0;
    mon_recalc(m);
    m->hp = m->stat[0];
    mon_fill_moves(m);
}

/* ------------------------------------------------------------ types / espèces */
int type_mult4(int mt, int t1, int t2)
{
    int a = typeChart[mt][t1];
    int b = (t2 == NO_TYPE) ? 4 : typeChart[mt][t2];
    return a * b / 4;
}

static u8 prevOf[SPECIES_COUNT + 1];
static int prevInit;
static void init_prev(void)
{
    int i;
    if (prevInit) return;
    for (i = 1; i <= SPECIES_COUNT; i++)
        if (species[i].evoTo) prevOf[species[i].evoTo] = (u8)i;
    prevInit = 1;
}

int evo_stage_level(int sp)
{
    init_prev();
    return prevOf[sp] ? species[prevOf[sp]].evoLvl : 1;
}

static int family_matches(int base, const u8 *types)
{
    int s = base, k;
    while (s) {
        for (k = 0; k < 4; k++)
            if (species[s].t1 == types[k] || species[s].t2 == types[k]) return 1;
        s = species[s].evoTo;
    }
    return 0;
}

int pick_wild_species(int biome, int level, int boss)
{
    int cand[SPECIES_COUNT], wts[SPECIES_COUNT], n = 0, i, total = 0, r, s;
    int anyBiome = rnd(100) < 12;
    init_prev();
    for (i = 1; i <= SPECIES_COUNT; i++) {
        int w;
        if (prevOf[i] || species[i].legend) continue;
        if (!anyBiome && !family_matches(i, biomes[biome].types)) continue;
        w = 12 - starterCost[i];
        if (w < 2) w = 2;
        if (boss) w = starterCost[i] >= 3 ? starterCost[i] : 0;
        if (!w) continue;
        cand[n] = i;
        wts[n] = w;
        total += w;
        n++;
    }
    if (n == 0) return 1 + rnd(SPECIES_COUNT);
    r = rnd(total);
    for (i = 0; i < n; i++) {
        if (r < wts[i]) break;
        r -= wts[i];
    }
    if (i >= n) i = n - 1;
    s = cand[i];
    for (;;) {
        const Species *sp = &species[s];
        if (!sp->evoTo) break;
        if (s == 133) {
            if (level >= 30) s = 134 + rnd(3);
            break;
        }
        if (level >= sp->evoLvl) s = sp->evoTo;
        else break;
    }
    return s;
}

/* ------------------------------------------------------------ soins */
int heal_mon(Mon *m, int amount)
{
    int before = m->hp;
    if (m->hp <= 0) return 0;
    m->hp += amount;
    if (m->hp > m->stat[0]) m->hp = m->stat[0];
    return m->hp - before;
}

/* ============================================================ COMBAT */
static BattleSetup *B;
static Fighter P, F;
static int activeIdx;
static int fleeTries;

static const char *const STAT_NAMES[5] = {"L'Attaque", "La Défense", "L'Atq. Spé.", "La Déf. Spé.", "La Vitesse"};

static const char *fname(const Fighter *f)
{
    static char buf[2][40];
    static int k;
    k ^= 1;
    if (f->side) snprintf(buf[k], 40, "%s ennemi", mon_name(f->mon));
    else snprintf(buf[k], 40, "%s", mon_name(f->mon));
    return buf[k];
}

static void init_fighter(Fighter *f, Mon *m, int side)
{
    memset(f, 0, sizeof(*f));
    f->mon = m;
    f->side = (u8)side;
    f->critBoost = m->held[HI_SCOPE];
}

static int apply_stage(int v, int s)
{
    if (s >= 0) return v * (2 + s) / 2;
    return v * 2 / (2 - s);
}

static int eff_speed(const Fighter *f)
{
    int v = apply_stage(f->mon->stat[STAT_SPE], f->stage[4]);
    if (f->mon->status == ST_PAR) v /= 2;
    return v;
}

static int status_immune(const Mon *m, int st)
{
    int t1 = mt1(m), t2 = mt2(m);
    if (st == ST_PAR && (t1 == 3 || t2 == 3)) return 1;
    if (st == ST_BRN && (t1 == 1 || t2 == 1)) return 1;
    if ((st == ST_PSN || st == ST_TOX) && (t1 == 7 || t2 == 7 || t1 == 16 || t2 == 16)) return 1;
    if (st == ST_FRZ && (t1 == 5 || t2 == 5)) return 1;
    return 0;
}

static void sync_all(void)
{
    ui_sync_hp();
}

static void show_hp(void)
{
    ui_animate_hp();
}

static int try_status(Fighter *t, int st, int quiet)
{
    static const char *const msgs[] = {"", "est brûlé !", "est paralysé ! Il aura du mal à attaquer !", "est empoisonné !", "est gravement empoisonné !", "s'endort !", "est gelé !", "est confus !"};
    if (st == ST_CNF) {
        if (t->confused) return 0;
        t->confused = (u8)(2 + rnd(4));
        ui_msg("%s est confus !", fname(t));
        return 1;
    }
    if (t->mon->status || t->mon->hp <= 0) {
        if (!quiet) ui_msg("Mais cela échoue !");
        return 0;
    }
    if (status_immune(t->mon, st)) {
        if (!quiet) ui_msg("Cela n'affecte pas %s...", fname(t));
        return 0;
    }
    t->mon->status = (u8)st;
    t->mon->statusTurns = (u8)(st == ST_SLP ? 1 + rnd(3) : 0);
    t->toxic = 0;
    sync_all();
    ui_msg("%s %s", fname(t), msgs[st]);
    return 1;
}

static void change_stage(Fighter *t, int stat, int amount)
{
    int old = t->stage[stat], nw = old + amount;
    if (nw > 6) nw = 6;
    if (nw < -6) nw = -6;
    if (nw == old) {
        ui_msg("%s de %s ne peut plus %s !", STAT_NAMES[stat], fname(t), amount > 0 ? "monter" : "baisser");
        return;
    }
    t->stage[stat] = (s8)nw;
    ui_msg("%s de %s %s%s !", STAT_NAMES[stat], fname(t), amount > 0 ? "monte" : "baisse", (amount >= 2 || amount <= -2) ? " beaucoup" : "");
}

static int calc_damage(Fighter *a, Fighter *d, const Move *mv, int *crit, int *mult4)
{
    Mon *am = a->mon, *dm = d->mon;
    int phys = (mv->cat == CAT_PHYS);
    int A = am->stat[phys ? STAT_ATK : STAT_SPA], D = dm->stat[phys ? STAT_DEF : STAT_SPD];
    int sa = a->stage[phys ? 0 : 2], sd = d->stage[phys ? 1 : 3];
    int cst, dmg;
    static const int chances[4] = {24, 8, 2, 1};
    *mult4 = type_mult4(mv->type, mt1(dm), mt2(dm));
    if (mv->type == 8 && mon_ability(dm) == AB_LEVITATE) *mult4 = 0;
    if (*mult4 == 0) { *crit = 0; return 0; }
    cst = (mv->eff == EF_HICRIT ? 1 : 0) + a->critBoost;
    if (cst > 3) cst = 3;
    *crit = (rnd(chances[cst]) == 0);
    if (*crit) { if (sa < 0) sa = 0; if (sd > 0) sd = 0; }
    A = apply_stage(A, sa);
    D = apply_stage(D, sd);
    if (phys && am->status && mon_ability(am) == AB_GUTS) A = A * 3 / 2;
    if (D < 1) D = 1;
    dmg = ((2 * am->level / 5 + 2) * mv->power * A / D) / 50 + 2;
    if (*crit) dmg = dmg * 3 / 2;
    dmg = dmg * (85 + rnd(16)) / 100;
    if (am->gim == 3) {
        const Species *os = &species[am->species];
        int own = (mv->type == os->t1 || mv->type == os->t2);
        if (mv->type == am->tmpT1) dmg = own ? dmg * 2 : dmg * 3 / 2;
        else if (own) dmg = dmg * 3 / 2;
    } else if (mv->type == mt1(am) || mv->type == mt2(am)) dmg = dmg * 3 / 2;
    dmg = dmg * *mult4 / 4;
    if (phys && am->status == ST_BRN && mon_ability(am) != AB_GUTS) dmg /= 2;
    {
        int ab = mon_ability(am);
        if (am->hp * 3 <= am->stat[0] && ((ab == AB_BLAZE && mv->type == 1) || (ab == AB_TORRENT && mv->type == 2) || (ab == AB_OVERGROW && mv->type == 4)))
            dmg = dmg * 3 / 2;
    }
    if (am->held[HI_LIFEORB]) dmg = dmg * 13 / 10;
    if (am->held[HI_TYPEBOOST] && am->boostType == mv->type) dmg = dmg * (100 + 20 * am->held[HI_TYPEBOOST]) / 100;
    if (dmg < 1) dmg = 1;
    return dmg;
}

static int can_act(Fighter *a)
{
    Mon *m = a->mon;
    if (a->flinch) {
        a->flinch = 0;
        ui_msg("%s a la trouille !", fname(a));
        return 0;
    }
    if (m->status == ST_SLP) {
        if (m->statusTurns == 0) {
            m->status = ST_NONE;
            sync_all();
            ui_msg("%s se réveille !", fname(a));
        } else {
            m->statusTurns--;
            ui_msg("%s dort profondément.", fname(a));
            return 0;
        }
    }
    if (m->status == ST_FRZ) {
        if (rnd(100) < 20) {
            m->status = ST_NONE;
            sync_all();
            ui_msg("%s dégèle !", fname(a));
        } else {
            ui_msg("%s est gelé !", fname(a));
            return 0;
        }
    }
    if (m->status == ST_PAR && rnd(100) < 25) {
        ui_msg("%s est paralysé ! Il ne peut pas attaquer !", fname(a));
        return 0;
    }
    if (a->confused) {
        ui_msg("%s est confus !", fname(a));
        a->confused--;
        if (a->confused == 0) ui_msg("%s n'est plus confus !", fname(a));
        else if (rnd(100) < 34) {
            int dmg = ((2 * m->level / 5 + 2) * 40 * m->stat[STAT_ATK] / (m->stat[STAT_DEF] ? m->stat[STAT_DEF] : 1)) / 50 + 2;
            m->hp -= dmg;
            if (m->hp < 0) m->hp = 0;
            ui_msg("%s se blesse dans sa confusion !", fname(a));
            ui_shake(8);
            show_hp();
            return 0;
        }
    }
    return 1;
}

static int move_targets_self(const Move *mv)
{
    return mv->eff == EF_STAT_SELF || mv->eff == EF_HEAL || mv->eff == EF_REST || mv->eff == EF_NOTHING;
}

/* exécute une attaque ; renvoie 1 si le combattant adverse est K.O. */
static void do_move(Fighter *a, Fighter *d, int slot)
{
    Mon *am = a->mon;
    int mid = (slot < 0) ? M_LUTTE : am->moves[slot];
    const Move *mv = &moves[mid];
    Move maxMv;
    char maxName[40];
    int hits = 1, total = 0, h, crit = 0, mult4 = 4, anyCrit = 0, lastMult = 4;
    if (!can_act(a)) return;
    if (slot >= 0) am->pp[slot]--;
    if (am->gim == 2 && mv->cat != CAT_STAT && mid != M_LUTTE) {
        int pw = mv->power, np;
        np = pw <= 40 ? 90 : pw <= 50 ? 100 : pw <= 60 ? 110 : pw <= 70 ? 120 : pw <= 100 ? 130 : pw <= 140 ? 140 : 150;
        if (mv->type == 6 || mv->type == 7) np -= 20;   /* Combat / Poison : puissance Dynamax réduite */
        maxMv = *mv;
        maxMv.power = (u8)np;
        maxMv.eff = EF_NONE; maxMv.chance = 0; maxMv.acc = 0;   /* ne rate jamais */
        snprintf(maxName, sizeof(maxName), "Dyna-%s", mv->name);
        maxMv.name = maxName;
        mv = &maxMv;
    }
    ui_msg("%s utilise %s !", fname(a), mv->name);
    ui_lunge(a->side);
    if (!move_targets_self(mv) && mv->acc && rnd(100) >= mv->acc) {
        ui_msg("L'attaque de %s échoue !", fname(a));
        return;
    }
    if (mv->cat == CAT_STAT) {
        switch (mv->eff) {
        case EF_STATUS: try_status(d, mv->p1, 0); break;
        case EF_STAT_FOE: change_stage(d, mv->p1, mv->p2); break;
        case EF_STAT_SELF:
            audio_sfx(SFX_HEAL);
            change_stage(a, mv->p1, mv->p2);
            if (mv->p3 != 255) change_stage(a, mv->p3, mv->p2);
            break;
        case EF_HEAL: {
            int got = heal_mon(am, am->stat[0] / 2);
            audio_sfx(SFX_HEAL);
            sync_all();
            show_hp();
            if (got > 0) ui_msg("%s récupère des PV !", fname(a));
            else ui_msg("Mais cela échoue !");
            break;
        }
        case EF_REST:
            am->hp = am->stat[0];
            am->status = ST_SLP;
            am->statusTurns = 2;
            audio_sfx(SFX_HEAL);
            sync_all();
            show_hp();
            ui_msg("%s s'endort et récupère tous ses PV !", fname(a));
            break;
        default: ui_msg("Mais rien ne se passe !"); break;
        }
        return;
    }
    if (mv->eff == EF_MULTI) hits = mv->p1 == 2 ? 2 : ((int[]){2, 2, 3, 3, 4, 5})[rnd(6)];
    for (h = 0; h < hits; h++) {
        int dmg;
        if (d->mon->hp <= 0) break;
        if (mv->eff == EF_FIXED) {
            mult4 = type_mult4(mv->type, mt1(d->mon), mt2(d->mon));
            dmg = mult4 ? mv->p1 : 0;
            crit = 0;
        } else if (mv->eff == EF_LEVELDMG) {
            mult4 = type_mult4(mv->type, mt1(d->mon), mt2(d->mon));
            dmg = mult4 ? am->level : 0;
            crit = 0;
        } else dmg = calc_damage(a, d, mv, &crit, &mult4);
        lastMult = mult4;
        if (mult4 == 0) {
            ui_msg("Cela n'affecte pas %s...", fname(d));
            return;
        }
        if (dmg >= d->mon->hp && d->mon->hp == d->mon->stat[0] && d->mon->hp > 1 && mon_ability(d->mon) == AB_STURDY) {
            dmg = d->mon->hp - 1;
            ui_msg("%s tient bon grâce à Fermeté !", fname(d));
        }
        if (dmg > d->mon->hp) dmg = d->mon->hp;
        d->mon->hp -= dmg;
        total += dmg;
        if (crit) anyCrit = 1;
        scene.fxTarget = d->side == 1;
        audio_sfx(mult4 > 4 ? SFX_SUPER : mult4 < 4 ? SFX_WEAK : SFX_HIT);
        ui_attack_fx(mv->type, d->side == 1);
        scene.fxTarget = d->side == 1;
        scene.foe.white = scene.ally.white = 0;
        if (d->side) scene.foe.white = 180; else scene.ally.white = 180;
        ui_shake(8);
        scene.foe.white = scene.ally.white = 0;
        show_hp();
    }
    if (total > 0 && mv->cat == CAT_PHYS && am->hp > 0 && d->mon->hp > 0 && rnd(100) < 30) {
        int ab = mon_ability(d->mon);
        if (ab == AB_STATIC) try_status(a, ST_PAR, 1);
        else if (ab == AB_FLAMEBODY) try_status(a, ST_BRN, 1);
        else if (ab == AB_POISONPOINT) try_status(a, ST_PSN, 1);
    }
    if (hits > 1) ui_msg("Touché %d fois !", hits);
    if (anyCrit) ui_msg("Coup critique !");
    if (lastMult > 4) ui_msg("C'est super efficace !");
    else if (lastMult < 4) ui_msg("Ce n'est pas très efficace...");
    if (total > 0) {
        if (am->held[HI_LIFEORB] && am->hp > 0) {
            int rec = am->stat[0] / 10;
            if (rec < 1) rec = 1;
            am->hp -= rec;
            if (am->hp < 0) am->hp = 0;
            sync_all();
            show_hp();
            ui_msg("%s perd des PV à cause de l'Orbe Vie !", fname(a));
        }
        if (mv->eff == EF_DRAIN) {
            int got = heal_mon(am, total / 2 > 0 ? total / 2 : 1);
            if (got > 0) { show_hp(); ui_msg("%s absorbe de l'énergie !", fname(d)); }
        }
        if (mv->eff == EF_RECOIL) {
            int rec = mid == M_LUTTE ? am->stat[0] / 4 : total / (mv->p1 ? mv->p1 : 3);
            if (rec < 1) rec = 1;
            am->hp -= rec;
            if (am->hp < 0) am->hp = 0;
            show_hp();
            ui_msg("%s est blessé par le contrecoup !", fname(a));
        }
    }
    if (mv->eff == EF_SUICIDE) {
        am->hp = 0;
        show_hp();
    }
    if (d->mon->hp > 0) {
        if (mv->eff == EF_STATUS && rnd(100) < mv->chance) try_status(d, mv->p1, 1);
        else if (mv->eff == EF_FLINCH && rnd(100) < mv->chance) d->flinch = 1;
        else if (mv->eff == EF_STAT_FOE && rnd(100) < mv->chance) change_stage(d, mv->p1, mv->p2);
    }
    if (am->hp > 0 && mv->eff == EF_STAT_SELF) {
        change_stage(a, mv->p1, mv->p2);
        if (mv->p3 != 255) change_stage(a, mv->p3, mv->p2);
    }
}

/* ------------------------------------------------------------ IA ennemie */
static int ai_choose(void)
{
    Mon *m = F.mon;
    int i, best = -1, bestScore = -1, usable = 0, order[4], uc = 0;
    for (i = 0; i < 4; i++) {
        int sc;
        const Move *mv;
        if (m->moves[i] == 255 || m->pp[i] <= 0) continue;
        mv = &moves[m->moves[i]];
        usable++;
        order[uc++] = i;
        if (mv->cat == CAT_STAT) {
            sc = 18;
            if (mv->eff == EF_STATUS && !P.mon->status) sc = 45;
            if (mv->eff == EF_HEAL && m->hp * 2 < m->stat[0]) sc = 70;
            if (mv->eff == EF_STAT_SELF && F.stage[mv->p1] >= 2) sc = 0;
            if (mv->eff == EF_REST && m->hp * 3 < m->stat[0]) sc = 60;
        } else {
            int mult = type_mult4(mv->type, mt1(P.mon), mt2(P.mon));
            int pw = mv->eff == EF_FIXED ? 40 : mv->eff == EF_LEVELDMG ? m->level : mv->power;
            sc = pw * mult / 4;
            if (mv->type == mt1(m) || mv->type == mt2(m)) sc = sc * 3 / 2;
            if (mv->acc && mv->acc < 100) sc = sc * mv->acc / 100;
            if (mv->eff == EF_SUICIDE) sc /= 3;
        }
        sc += rnd(20);
        if (sc > bestScore) { bestScore = sc; best = i; }
    }
    if (!usable) return -1;
    if (rnd(100) < 20) return order[rnd(uc)];
    return best;
}

/* ------------------------------------------------------------ expérience / niveaux / évolutions */
static void learn_move(Mon *m, int mv)
{
    int i;
    for (i = 0; i < 4; i++)
        if (m->moves[i] == mv) return;
    for (i = 0; i < 4; i++)
        if (m->moves[i] == 255) {
            m->moves[i] = (u8)mv;
            m->pp[i] = m->ppmax[i] = moves[mv].pp;
            ui_msg("%s apprend %s !", mon_name(m), moves[mv].name);
            return;
        }
    ui_msg("%s veut apprendre %s, mais connaît déjà 4 attaques.", mon_name(m), moves[mv].name);
    {
        char labels[5][28];
        u16 cols[5];
        u8 en[5];
        int k, r;
        char title[40];
        for (k = 0; k < 4; k++) {
            snprintf(labels[k], 28, "%s", moves[m->moves[k]].name);
            cols[k] = typeColors[moves[m->moves[k]].type];
            en[k] = 1;
        }
        snprintf(labels[4], 28, "Ne pas apprendre");
        cols[4] = COL(120, 60, 60);
        en[4] = 1;
        snprintf(title, sizeof(title), "Oublier quelle attaque ?");
        r = ui_choose(BM_LIST, title, 5, (const char (*)[28])labels, 0, cols, en, 1, 0);
        if (r >= 0 && r < 4) {
            ui_msg("%s oublie %s et apprend %s !", mon_name(m), moves[m->moves[r]].name, moves[mv].name);
            m->moves[r] = (u8)mv;
            m->pp[r] = m->ppmax[r] = moves[mv].pp;
        } else ui_msg("%s n'apprend pas %s.", mon_name(m), moves[mv].name);
    }
}

void learn_moves_after_level(Mon *m, int oldL, int newL)
{
    int l, mv;
    for (l = oldL + 1; l <= newL; l++)
        if (mon_can_learn_at(m->species, l, &mv)) learn_move(m, mv);
}

int check_evolution(Mon *m, int oldL)
{
    const Species *sp = &species[m->species];
    int to, isActive;
    (void)oldL;
    if (!sp->evoTo || m->level < sp->evoLvl) return 0;
    to = sp->evoTo;
    if (m->species == 133) to = 134 + rnd(3);
    isActive = (B && P.mon == m);
    ui_msg("Quoi ? %s évolue !", mon_name(m));
    ui_flash(14);
    {
        char old[32];
        snprintf(old, sizeof(old), "%s", mon_name(m));
        m->species = (u8)to;
        mon_recalc(m);
        if (isActive) ui_set_ally(m);
        ui_flash(14);
        audio_sfx(SFX_LEVELUP);
        ui_msg("%s a évolué en %s !", old, mon_name(m));
    }
    return 1;
}

void apply_exp(Mon *m, int amount)
{
    int old = m->level;
    m->exp += amount;
    while (m->level < MAX_LEVEL && m->exp >= mon_exp_for(m->level + 1, m->species)) m->level++;
    if (m->level > old) {
        int gain = m->level;
        mon_recalc(m);
        sync_all();
        audio_sfx(SFX_LEVELUP);
        ui_msg("%s monte au niveau %d !", mon_name(m), gain);
        learn_moves_after_level(m, old, m->level);
        check_evolution(m, old);
        sync_all();
    }
}

static void award_exp(void)
{
    const Species *fs = &species[F.mon->species];
    int fl = F.mon->level, i;
    int base = fs->baseExp * fl / 5;
    if (B->isTrainer) base = base * 3 / 2;
    if (B->isBoss) base = base * 2;
    for (i = 0; i < run.partyCount; i++) {
        Mon *m = &run.party[i];
        int r256, r2, sq, gain;
        if (m->hp <= 0 || m->level >= MAX_LEVEL) continue;
        r256 = (2 * fl + 10) * 256 / (fl + m->level + 10);
        r2 = (r256 * r256) >> 8;
        sq = (int)isqrt64((unsigned long long)r256 << 8);
        gain = (int)((long long)base * ((r2 * sq) >> 8) / 256) + 1;
        gain *= EXP_MULT;
        if (i != activeIdx) gain /= 2;
        if (m->held[HI_LUCKYEGG]) gain = gain * (100 + 50 * m->held[HI_LUCKYEGG]) / 100;
        if (gain < 1) gain = 1;
        if (i == activeIdx) ui_msg("%s gagne %d points d'Exp.", mon_name(m), gain);
        apply_exp(m, gain);
    }
}

/* ------------------------------------------------------------ envoi / K.O. */
static void intimidate(Fighter *src, Fighter *tgt)
{
    if (!src->mon || !tgt->mon || src->mon->hp <= 0 || tgt->mon->hp <= 0) return;
    if (mon_ability(src->mon) != AB_INTIMIDATE) return;
    ui_msg("Intimidation de %s !", fname(src));
    change_stage(tgt, 0, -1);
}

static void send_ally(int idx, int announce)
{
    if (P.mon && P.mon != &run.party[idx]) gim_revert(P.mon);
    activeIdx = idx;
    init_fighter(&P, &run.party[idx], 0);
    ui_set_ally(&run.party[idx]);
    scene.ally.fade = 256;
    scene.ally.white = 200;
    if (announce) ui_msg("Go ! %s !", mon_name(P.mon));
    {
        int i;
        for (i = 0; i < 12; i++) {
            scene.ally.fade = 256 - i * 256 / 12;
            scene.ally.white = 200 - i * 200 / 12;
            anim_frame();
        }
        scene.ally.fade = 0;
        scene.ally.white = 0;
    }
    if (F.mon && F.mon->hp > 0) intimidate(&P, &F);
}

static void send_foe(int idx, int announce)
{
    int i;
    B->enemyIdx = idx;
    init_fighter(&F, &B->enemies[idx], 1);
    ui_set_foe(&B->enemies[idx], B->isBoss);
    scene.foe.dx = 90;
    for (i = 0; i < 14; i++) {
        scene.foe.dx = 90 - i * 90 / 14;
        anim_frame();
    }
    scene.foe.dx = 0;
    if (announce) {
        if (B->isTrainer) ui_msg("Le dresseur envoie %s !", mon_name(F.mon));
        else if (B->isBoss) ui_msg("Un %s surpuissant se dresse devant vous !", mon_name(F.mon));
        else ui_msg("Un %s sauvage apparaît !", mon_name(F.mon));
    }
    if (P.mon) intimidate(&F, &P);
}

static void faint_anim(Actor *a, Fighter *f)
{
    int i;
    audio_sfx(SFX_FAINT);
    for (i = 0; i <= 16; i++) {
        a->fade = i * 256 / 16;
        anim_frame();
    }
    a->hidden = 1;
    a->fade = 0;
    ui_msg("%s est K.O. !", fname(f));
}

static int living_count(void)
{
    int i, n = 0;
    for (i = 0; i < run.partyCount; i++)
        if (run.party[i].hp > 0) n++;
    return n;
}

/* renvoie 1 si le combat est terminé (victoire) ; -1 si défaite ; 0 sinon */
static int handle_faints(void)
{
    if (F.mon->hp <= 0 && !scene.foe.hidden) {
        faint_anim(&scene.foe, &F);
        run.kills++;
        award_exp();
        if (B->enemyIdx + 1 < B->enemyCount) {
            send_foe(B->enemyIdx + 1, 1);
            return 0;
        }
        return 1;
    }
    if (P.mon->hp <= 0 && !scene.ally.hidden) {
        faint_anim(&scene.ally, &P);
        gim_revert(P.mon);
        if (living_count() == 0) return -1;
        for (;;) {
            int r = ui_pick_party("Envoyer quel Pokémon ?", 0, 1, 0);
            if (r >= 0 && run.party[r].hp > 0) {
                scene.ally.hidden = 0;
                send_ally(r, 1);
                break;
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------------ fin de tour */
static void end_turn_effects(Fighter *f)
{
    Mon *m = f->mon;
    int dmg;
    if (m->hp <= 0) return;
    f->flinch = 0;
    if (m->gim == 2 && m->dynaTurns > 0 && --m->dynaTurns == 0) {
        ui_msg("%s n'est plus Dynamax.", fname(f));
        gim_revert(m);
        if (!f->side) ui_set_ally(m);
    }
    if (m->status == ST_BRN) {
        dmg = m->stat[0] / 16;
        if (dmg < 1) dmg = 1;
        m->hp -= dmg;
        if (m->hp < 0) m->hp = 0;
        ui_msg("%s souffre de sa brûlure !", fname(f));
        show_hp();
    } else if (m->status == ST_PSN || m->status == ST_TOX) {
        if (m->status == ST_TOX) { if (f->toxic < 15) f->toxic++; dmg = m->stat[0] * f->toxic / 16; }
        else dmg = m->stat[0] / 8;
        if (dmg < 1) dmg = 1;
        m->hp -= dmg;
        if (m->hp < 0) m->hp = 0;
        ui_msg("%s souffre du poison !", fname(f));
        show_hp();
    }
    if (m->hp > 0 && m->held[HI_LEFTOVERS] && m->hp < m->stat[0]) {
        int h = m->stat[0] * m->held[HI_LEFTOVERS] / 16;
        heal_mon(m, h < 1 ? 1 : h);
        show_hp();
        ui_msg("%s récupère des PV grâce aux Restes.", fname(f));
    }
    if (m->hp > 0 && m->held[HI_SITRUS] && m->hp * 2 <= m->stat[0]) {
        m->held[HI_SITRUS]--;
        heal_mon(m, m->stat[0] / 4);
        audio_sfx(SFX_HEAL);
        show_hp();
        ui_msg("%s mange une Baie Sitrus et récupère des PV !", fname(f));
    }
}

/* ------------------------------------------------------------ capture */
static int try_capture(int ball)
{
    int a, mx = F.mon->stat[0], hp = F.mon->hp, shakes = 0, caught = 0, i;
    int mult10[5] = {10, 15, 20, 25, 2550};
    long long num;
    u32 b;
    const Species *sp = &species[F.mon->species];
    int rate = sp->catchRate;
    if (!B->canCatch) {
        ui_msg("Impossible de capturer le Pokémon d'un dresseur ou un boss !");
        return 0;
    }
    if (run.balls[ball] <= 0) {
        ui_msg("Vous n'avez plus de %s !", BALL_NAMES[ball]);
        return 0;
    }
    run.balls[ball]--;
    ui_msg("Vous lancez une %s !", BALL_NAMES[ball]);
    audio_sfx(SFX_BALL);
    /* trajectoire */
    scene.ballShow = 1;
    for (i = 0; i <= 18; i++) {
        scene.ballX = 70 + (186 - 70) * i / 18;
        scene.ballY = 120 + (60 - 120) * i / 18 - (i * (18 - i)) / 3;
        anim_frame();
    }
    scene.foe.white = 256;
    for (i = 0; i <= 10; i++) { scene.foe.fade = i * 256 / 10; scene.foe.white = 256; anim_frame(); }
    scene.foe.hidden = 1;
    scene.foe.white = 0;
    scene.foe.fade = 0;
    for (i = 0; i <= 12; i++) {
        scene.ballY = 60 + (80 - 60) * i / 12;
        anim_frame();
    }
    num = (long long)(3 * mx - 2 * hp) * rate * mult10[ball];
    a = (int)(num / (3LL * mx * 10));
    if (F.mon->status == ST_SLP || F.mon->status == ST_FRZ) a = a * 2;
    else if (F.mon->status) a = a * 3 / 2;
    if (a < 1) a = 1;
    if (a >= 255) { a = 255; shakes = 4; caught = 1; }
    else {
        unsigned long long x = ((unsigned long long)a << 16) / 255;
        b = isqrt64(x << 16);
        b = isqrt64((unsigned long long)b << 16);
        for (i = 0; i < 4; i++) {
            if ((u32)rnd(65536) >= b) break;
            shakes++;
        }
        caught = (shakes == 4);
    }
    for (i = 0; i < (shakes < 3 ? shakes + 1 : 3); i++) {
        int k;
        if (i >= shakes && caught) break;
        audio_sfx(SFX_BALL);
        for (k = 0; k < 8; k++) { scene.ballAngle = (k < 2 || k > 5) ? 0 : (k < 4 ? -3 : 3); anim_frame(); }
        scene.ballAngle = 0;
        ui_wait(10);
        if (i >= shakes) break;
    }
    if (!caught) {
        scene.ballShow = 0;
        scene.foe.hidden = 0;
        scene.foe.white = 200;
        for (i = 0; i < 8; i++) { scene.foe.white = 200 - i * 25; anim_frame(); }
        scene.foe.white = 0;
        ui_msg(shakes == 0 ? "Oh non ! Le Pokémon s'est libéré !" : shakes < 3 ? "Zut ! Il était presque attrapé !" : "Argh ! Presque !");
        return 1;
    }
    audio_sfx(SFX_CATCH);
    ui_msg("Gotcha ! %s est capturé !", mon_name(F.mon));
    run.caught++;
    {
        Mon c = *F.mon;
        c.status = 0;
        c.hp = c.stat[0];
        for (i = 0; i < 4; i++) c.pp[i] = c.ppmax[i];
        scene.ballShow = 0;
        if (run.partyCount < MAX_PARTY) run.party[run.partyCount++] = c;
        else {
            int r;
            ui_msg("Votre équipe est pleine !");
            r = ui_pick_party("Remplacer qui ? (B = relâcher)", 1, 0, 0);
            if (r >= 0) {
                ui_msg("%s est relâché.", mon_name(&run.party[r]));
                run.party[r] = c;
            } else ui_msg("%s est relâché.", mon_name(&c));
        }
    }
    return 2;
}

void battle_clear(void) { B = 0; }

/* ------------------------------------------------------------ boucle principale */
static int pendingGim = -1, pendingMove = -1;
static int pick_move_slot(void)
{
    char labels[4][28], subs[4][28];
    u16 cols[4];
    u8 en[4];
    int i, anyPP = 0;
    Mon *m = P.mon;
    for (i = 0; i < 4; i++) {
        if (m->moves[i] == 255) {
            labels[i][0] = 0;
            subs[i][0] = 0;
            cols[i] = COL(80, 80, 90);
            en[i] = 0;
            bottom.moveId[i] = 0;
            continue;
        }
        snprintf(labels[i], 28, "%s", moves[m->moves[i]].name);
        snprintf(subs[i], 28, "PP %d/%d", m->pp[i], m->ppmax[i]);
        cols[i] = typeColors[moves[m->moves[i]].type];
        en[i] = m->pp[i] > 0;
        bottom.moveId[i] = m->moves[i];
        if (en[i]) anyPP = 1;
    }
    if (!anyPP) return -2;   /* Lutte */
    bottom.gimShow = (m->gim == 0);
    bottom.gimAvail[0] = (m->gim == 0 && !gimUsed[0] && mega_for(m) != 0);
    bottom.gimAvail[1] = (m->gim == 0 && !gimUsed[1]);
    bottom.gimAvail[2] = (m->gim == 0 && !gimUsed[2]);
    if (!(bottom.gimAvail[0] | bottom.gimAvail[1] | bottom.gimAvail[2])) bottom.gimShow = 0;
    bottom.gimSel = -1;
    {
        int r = ui_choose(BM_MOVES, "Choisis une attaque", 4, (const char (*)[28])labels, (const char (*)[28])subs, cols, en, 2, 1);
        pendingGim = (r >= 0 && bottom.gimShow) ? bottom.gimSel : -1;
        pendingMove = r;
        bottom.gimShow = 0; bottom.gimSel = -1;
        memset(bottom.gimAvail, 0, sizeof(bottom.gimAvail));
        return r;
    }
}

static int pick_ball(void)
{
    char labels[5][28];
    u8 en[5];
    u16 cols[5] = {COL(210, 60, 60), COL(60, 110, 220), COL(230, 190, 40), COL(190, 40, 90), COL(150, 60, 190)};
    char subs[5][28];
    int i;
    for (i = 0; i < 5; i++) {
        snprintf(labels[i], 28, "%s", BALL_NAMES[i]);
        snprintf(subs[i], 28, "x%d", run.balls[i]);
        en[i] = run.balls[i] > 0;
    }
    return ui_choose(BM_LIST, "Quelle balle ?", 5, (const char (*)[28])labels, (const char (*)[28])subs, cols, en, 1, 1);
}

/* action du joueur : renvoie 1 si le combat se poursuit normalement */
int battle_run(BattleSetup *bs)
{
    int result = -99;
    B = bs;
    fleeTries = 0;
    memset(gimUsed, 0, sizeof(gimUsed));
    pendingGim = -1;
    P.mon = 0;
    ui_reset_scene();
    scene.biome = run.biome;
    scene.wave = bs->wave;
    audio_music(audio_battle_track(bs->isBoss, bs->isTrainer, run.biome));
    {
        int i, first = 0;
        for (i = 0; i < run.partyCount; i++)
            if (run.party[i].hp > 0) { first = i; break; }
        activeIdx = first;
        init_fighter(&P, &run.party[first], 0);
        init_fighter(&F, &bs->enemies[0], 1);
        bs->enemyIdx = 0;
        scene.ally.visible = 0;
        if (bs->isTrainer) {
            scene.trainer = 1;
            scene.trainerSeed = bs->wave * 7 + 3;
        } else {
            ui_set_foe(F.mon, bs->isBoss);
            scene.foe.dx = 90;
        }
        ui_msg_nowait(" ");
        ui_fade_in();
        if (bs->isTrainer) {
            ui_msg("Un dresseur vous défie !");
            send_foe(0, 1);
        } else {
            int k;
            for (k = 0; k < 14; k++) { scene.foe.dx = 90 - k * 90 / 14; anim_frame(); }
            scene.foe.dx = 0;
            if (bs->isBoss) ui_msg("Un %s surpuissant se dresse devant vous !", mon_name(F.mon));
            else ui_msg("Un %s sauvage apparaît !", mon_name(F.mon));
        }
        send_ally(first, 1);
        intimidate(&F, &P);
    }
    while (result == -99) {
        int act, pSlot = -1, pAction = 0 /*0 attaque 1 changer 2 balle*/, pArg = 0, eSlot;
        int pFirst, r;
        ui_bottom_info();
        ui_sync_hp();
        /* menu du joueur */
        for (;;) {
            act = ui_main_menu();
            if (act == 0) {
                pSlot = pick_move_slot();
                if (pSlot == -1) continue;
                pAction = 0;
                if (pSlot == -2) { pSlot = -1; pendingGim = -1; }   /* Lutte */
                break;
            } else if (act == 1) {
                int bsel;
                if (!B->canCatch) { ui_msg("Impossible de capturer ce Pokémon !"); continue; }
                bsel = pick_ball();
                if (bsel < 0) continue;
                pAction = 2;
                pArg = bsel;
                break;
            } else if (act == 2) {
                int sel = ui_pick_party("Envoyer quel Pokémon ?", 1, 1, 0);
                if (sel < 0) continue;
                if (sel == activeIdx) { ui_msg("%s est déjà au combat !", mon_name(P.mon)); continue; }
                pAction = 1;
                pArg = sel;
                break;
            } else if (act == 3) {
                if (B->isBoss || B->isTrainer) { ui_msg("Impossible de fuir ce combat !"); continue; }
                fleeTries++;
                if (eff_speed(&P) >= eff_speed(&F) || rnd(100) < 40 + fleeTries * 20) {
                    ui_msg("Vous prenez la fuite !");
                    result = BR_FLED;
                    break;
                }
                ui_msg("Impossible de s'échapper !");
                pAction = 3;
                break;
            }
        }
        if (result != -99) break;
        ui_bottom_info();
        /* actions non-attaque : exécutées en premier */
        if (pAction == 1) {
            ui_msg("Reviens, %s !", mon_name(P.mon));
            scene.ally.white = 200;
            { int i; for (i = 0; i <= 10; i++) { scene.ally.fade = i * 256 / 10; anim_frame(); } }
            scene.ally.fade = 0;
            scene.ally.white = 0;
            send_ally(pArg, 1);
        } else if (pAction == 2) {
            r = try_capture(pArg);
            if (r == 2) { result = BR_CAUGHT; break; }
            if (r == 0) continue;   /* pas de tour perdu */
        }
        /* Méga-évolution / Dynamax / Téracristallisation (avant les attaques) */
        if (pAction == 0 && pendingGim >= 0 && P.mon->hp > 0) {
            int g = pendingGim, tt = mt1(P.mon);
            pendingGim = -1;
            if (g == 0) megaPick = mega_for(P.mon);
            if (g == 2) {
                if (pSlot >= 0 && moves[P.mon->moves[pSlot]].cat != CAT_STAT) tt = moves[P.mon->moves[pSlot]].type;
            }
            if (g == 0) ui_msg("%s est entouré d'une énergie Méga !", mon_name(P.mon));
            else if (g == 1) ui_msg("%s est englouti par l'énergie Dynamax !", mon_name(P.mon));
            else ui_msg("%s se téracristallise en type %s !", mon_name(P.mon), typeNames[tt]);
            gimUsed[g] = 1;
            ui_flash(18);
            audio_sfx(SFX_LEVELUP);
            gim_apply(P.mon, g, tt);
            ui_set_ally(P.mon);
            if (g == 0) ui_msg("%s a mégaévolué en %s !", mon_name(P.mon), megaPick->name);
            ui_sync_hp();
        }
        /* choix ennemi */
        eSlot = ai_choose();
        pFirst = 1;
        if (pAction == 0) {
            int pp = (pSlot >= 0 ? moves[P.mon->moves[pSlot]].prio : 0);
            int ep = (eSlot >= 0 ? moves[F.mon->moves[eSlot]].prio : 0);
            if (P.mon->held[HI_QUICKCLAW] && rnd(100) < 20 * P.mon->held[HI_QUICKCLAW]) pp += 1;
            if (ep > pp) pFirst = 0;
            else if (ep == pp) {
                int sp1 = eff_speed(&P), sp2 = eff_speed(&F);
                if (sp2 > sp1) pFirst = 0;
                else if (sp2 == sp1) pFirst = rnd(2);
            }
        } else pFirst = 0;
        {
            int step, fainted;
            for (step = 0; step < 2; step++) {
                int playerTurn = (step == 0) == (pFirst == 1);
                if (playerTurn) {
                    if (pAction != 0) continue;
                    if (P.mon->hp <= 0 || F.mon->hp <= 0) continue;
                    do_move(&P, &F, pSlot);
                } else {
                    if (P.mon->hp <= 0 || F.mon->hp <= 0) continue;
                    do_move(&F, &P, eSlot);
                }
                sync_all();
                fainted = (F.mon->hp <= 0 || P.mon->hp <= 0);
                r = handle_faints();
                if (r == 1) { result = BR_WIN; break; }
                if (r == -1) { result = BR_LOSE; break; }
                if (fainted) break;
            }
        }
        if (result != -99) break;
        if (P.mon->hp > 0 && F.mon->hp > 0) {
            end_turn_effects(&P);
            end_turn_effects(&F);
            sync_all();
            r = handle_faints();
            if (r == 1) { result = BR_WIN; break; }
            if (r == -1) { result = BR_LOSE; break; }
            if (P.mon->hp > 0 && F.mon->hp > 0) { /* prochain tour */ }
        }
        /* si le Pokémon ennemi suivant vient d'arriver, rien de plus */
        if (P.mon->hp <= 0 || F.mon->hp <= 0) {
            r = handle_faints();
            if (r == 1) { result = BR_WIN; break; }
            if (r == -1) { result = BR_LOSE; break; }
        }
    }
    {
        int i;
        for (i = 0; i < run.partyCount; i++) gim_revert(&run.party[i]);
    }
    if (result == BR_WIN) {
        if (B->isTrainer) ui_msg("Vous avez battu le dresseur !");
        audio_sfx(SFX_LEVELUP);
    }
    return result;
}
