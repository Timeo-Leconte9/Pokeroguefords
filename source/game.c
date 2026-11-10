/* game.c : boucle de partie (vagues, biomes, boss, récompenses, boutique, sauvegarde, sélection des starters) */
#include "game.h"
#include <time.h>
#include <fat.h>

Run run;
int uiScreen;

#define SAVE_FILE "PokeRogueDS.sav"
#define SAVE_MAGIC 0x50525334u   /* 'PRS4' */
#define START_POINTS 10
#define WHITE COL(255, 255, 255)
#define BLACK COL(16, 16, 24)
#define POST_WAVE_HEAL_PCT 12

/* ------------------------------------------------------------ objets */
const ItemDef itemDefs[IT_COUNT] = {
    [IT_POTION] = {"Potion", "Rend 20 PV", 0, 100, 0},
    [IT_SUPERPOTION] = {"Super Potion", "Rend 50 PV", 1, 250, 0},
    [IT_HYPERPOTION] = {"Hyper Potion", "Rend 200 PV", 2, 500, 0},
    [IT_FULLRESTORE] = {"Guérison Totale", "PV + statut", 3, 1200, 0},
    [IT_REVIVE] = {"Rappel", "Réanime (50% PV)", 1, 500, 0},
    [IT_MAXREVIVE] = {"Rappel Max", "Réanime (100% PV)", 3, 1500, 0},
    [IT_ETHER] = {"Élixir", "Tous les PP", 1, 400, 0},
    [IT_FULLHEAL] = {"Total Soin", "Guérit le statut", 0, 200, 0},
    [IT_RARECANDY] = {"Super Bonbon", "+1 niveau", 2, 1000, 3},
    [IT_BALL_POKE] = {"Poké Ball x5", "Capture de base", 0, 100, 1},
    [IT_BALL_GREAT] = {"Super Ball x5", "Capture x1.5", 1, 300, 1},
    [IT_BALL_ULTRA] = {"Hyper Ball x5", "Capture x2", 2, 600, 1},
    [IT_BALL_MASTER] = {"Master Ball", "Capture sûre", 4, 6000, 1},
    [IT_BALL_ROGUE] = {"Rogue Ball x5", "Capture x2.5", 3, 1200, 1},
    [IT_LEFTOVERS] = {"Restes", "1/16 PV par tour", 2, 0, 2},
    [IT_LIFEORB] = {"Orbe Vie", "+30% dégâts", 3, 0, 2},
    [IT_SCOPE] = {"Lentille Scope", "Critiques ++", 1, 0, 2},
    [IT_QUICKCLAW] = {"Griffe Rapide", "20% d'agir en 1er", 1, 0, 2},
    [IT_LUCKYEGG] = {"Oeuf Chance", "+50% Exp.", 2, 0, 2},
    [IT_SITRUS] = {"Baie Sitrus", "Soigne 25% (1 fois)", 0, 0, 2},
    [IT_TYPEBOOST] = {"Booster de type", "+20% (un type)", 2, 0, 2},
    [IT_AMULET] = {"Amulette Pièce", "+20% d'argent", 3, 0, 2},
};
static const u8 heldIndex[IT_COUNT] = {[IT_LEFTOVERS] = HI_LEFTOVERS, [IT_LIFEORB] = HI_LIFEORB, [IT_SCOPE] = HI_SCOPE,
                                      [IT_QUICKCLAW] = HI_QUICKCLAW, [IT_LUCKYEGG] = HI_LUCKYEGG, [IT_SITRUS] = HI_SITRUS,
                                      [IT_TYPEBOOST] = HI_TYPEBOOST, [IT_AMULET] = HI_AMULET};
static const char *const boostNames[TYPE_COUNT] = {"Mouchoir Soie", "Charbon", "Eau Mystique", "Aimant", "Graine Miracle", "Glaçon Éternel",
                                                   "Ceinture Noire", "Pic Venin", "Sable Doux", "Bec Pointu", "Cuiller Tordue",
                                                   "Poudre Argent", "Pierre Dure", "Rune Sort", "Dent Dragon", "Lunettes Noires",
                                                   "Peau Métal", "Pierre Fée"};

static u16 tier_color(int t)
{
    static const u16 c[5] = {COL(90, 120, 170), COL(70, 170, 100), COL(100, 100, 220), COL(220, 170, 40), COL(200, 80, 200)};
    return c[t < 0 ? 0 : t > 4 ? 4 : t];
}

static int item_price(int item)
{
    static const int tierPrice[5] = {300, 600, 1200, 2500, 6000};
    int p = itemDefs[item].price;
    if (p <= 0) p = tierPrice[itemDefs[item].tier > 4 ? 4 : itemDefs[item].tier];
    return p * (100 + run.wave * 2) / 100;
}

/* applique un objet à un Pokémon ; renvoie 1 si utilisé */
static int use_on(int item, Mon *m)
{
    switch (item) {
    case IT_POTION: case IT_SUPERPOTION: case IT_HYPERPOTION: {
        int amt = item == IT_POTION ? 20 : item == IT_SUPERPOTION ? 50 : 200;
        if (m->hp <= 0 || m->hp >= m->stat[0]) { ui_msg_nowait("Cela n'aurait aucun effet."); ui_wait(30); return 0; }
        heal_mon(m, amt);
        audio_sfx(SFX_HEAL);
        ui_msg("%s récupère des PV !", mon_name(m));
        return 1;
    }
    case IT_FULLRESTORE:
        if (m->hp <= 0 || (m->hp >= m->stat[0] && !m->status)) { ui_msg_nowait("Cela n'aurait aucun effet."); ui_wait(30); return 0; }
        m->hp = m->stat[0];
        m->status = 0;
        audio_sfx(SFX_HEAL);
        ui_msg("%s est en pleine forme !", mon_name(m));
        return 1;
    case IT_REVIVE: case IT_MAXREVIVE:
        if (m->hp > 0) { ui_msg_nowait("Cela n'aurait aucun effet."); ui_wait(30); return 0; }
        m->hp = item == IT_REVIVE ? m->stat[0] / 2 : m->stat[0];
        if (m->hp < 1) m->hp = 1;
        m->status = 0;
        audio_sfx(SFX_HEAL);
        ui_msg("%s est réanimé !", mon_name(m));
        return 1;
    case IT_ETHER: {
        int i;
        for (i = 0; i < 4; i++) m->pp[i] = m->ppmax[i];
        audio_sfx(SFX_HEAL);
        ui_msg("Les PP de %s sont restaurés !", mon_name(m));
        return 1;
    }
    case IT_FULLHEAL:
        if (!m->status) { ui_msg_nowait("Cela n'aurait aucun effet."); ui_wait(30); return 0; }
        m->status = 0;
        audio_sfx(SFX_HEAL);
        ui_msg("%s est guéri !", mon_name(m));
        return 1;
    case IT_RARECANDY: {
        int need = mon_exp_for(m->level + 1, m->species) - m->exp;
        if (m->hp <= 0) { ui_msg_nowait("Cela n'aurait aucun effet."); ui_wait(30); return 0; }
        apply_exp(m, need > 0 ? need : 1);
        return 1;
    }
    default: {
        int hi = heldIndex[item];
        if (itemDefs[item].kind != 2) return 0;
        if (m->held[hi] >= 5) { ui_msg("%s en porte déjà assez !", mon_name(m)); return 0; }
        if (item == IT_TYPEBOOST) {
            const Species *sp = &species[m->species];
            int t = sp->t1;
            if (m->held[hi] && m->boostType != t) { ui_msg("%s a déjà un booster d'un autre type.", mon_name(m)); return 0; }
            m->boostType = (u8)t;
        }
        m->held[hi]++;
        audio_sfx(SFX_HEAL);
        if (item == IT_TYPEBOOST) ui_msg("%s reçoit : %s !", mon_name(m), boostNames[m->boostType]);
        else ui_msg("%s reçoit : %s !", mon_name(m), itemDefs[item].name);
        return 1;
    }
    }
}

int item_pick_target(int item)
{
    int filtA = 0, filtF = 0;
    if (item == IT_REVIVE || item == IT_MAXREVIVE) filtF = 1;
    else if (itemDefs[item].kind != 1) filtA = (item != IT_ETHER && item != IT_FULLHEAL && itemDefs[item].kind != 2) ? 1 : 0;
    for (;;) {
        int r = ui_pick_party("Sur quel Pokémon ?", 1, filtA, filtF);
        if (r < 0) return 0;
        if (use_on(item, &run.party[r])) return 1;
    }
}

void give_item(int item, int tier)
{
    (void)tier;
    switch (item) {
    case IT_BALL_POKE: run.balls[0] += 5; ui_msg("Vous obtenez 5 Poké Balls !"); break;
    case IT_BALL_GREAT: run.balls[1] += 5; ui_msg("Vous obtenez 5 Super Balls !"); break;
    case IT_BALL_ULTRA: run.balls[2] += 5; ui_msg("Vous obtenez 5 Hyper Balls !"); break;
    case IT_BALL_ROGUE: run.balls[3] += 5; ui_msg("Vous obtenez 5 Rogue Balls !"); break;
    case IT_BALL_MASTER: run.balls[4] += 1; ui_msg("Vous obtenez une Master Ball !"); break;
    default: break;
    }
}

/* ------------------------------------------------------------ récompenses */
static int roll_item(int wave)
{
    int tier, r = rnd(1000), c0, c1, c2, c3, k, pool[12], n = 0, i;
    c0 = 520 - wave * 2;
    if (c0 < 150) c0 = 150;
    c1 = c0 + 300;
    c2 = c1 + 130 + wave / 2;
    c3 = c2 + 40 + wave / 4;
    if (r < c0) tier = 0;
    else if (r < c1) tier = 1;
    else if (r < c2) tier = 2;
    else if (r < c3) tier = 3;
    else tier = 4;
    if (wave < 10 && tier > 2) tier = 2;
    if (tier == 4 && rnd(3)) tier = 3;
    for (k = 0; k < IT_COUNT; k++) {
        if (itemDefs[k].tier == tier) pool[n++] = k;
        if (n >= 12) break;
    }
    if (!n) return IT_POTION;
    i = rnd(n);
    return pool[i];
}

static void roll_rewards(int *out, int wave)
{
    int i, j, tries = 0;
    int needHeal = 0;
    for (i = 0; i < run.partyCount; i++)
        if (run.party[i].hp * 2 < run.party[i].stat[0]) needHeal = 1;
    for (i = 0; i < 3; i++) {
        int it;
        do {
            it = roll_item(wave);
            for (j = 0; j < i; j++)
                if (out[j] == it) break;
            tries++;
        } while (j < i && tries < 40);
        out[i] = it;
    }
    if (needHeal && rnd(100) < 70) {
        int h = wave < 15 ? IT_POTION : wave < 40 ? IT_SUPERPOTION : IT_HYPERPOTION;
        out[0] = h;
    }
}

static void item_label(int item, char *dst, char *sub)
{
    if (item == IT_TYPEBOOST) snprintf(dst, 28, "Booster de type");
    else snprintf(dst, 28, "%s", itemDefs[item].name);
    snprintf(sub, 28, "%s", itemDefs[item].desc);
}

/* Boutique : 3 objets aléatoires (selon la rareté, balles comprises) + 3 objets fixes */
static int shopItems[3], shopSold[3], shopWave = -1;
static const u8 shopFixed[3] = {IT_POTION, IT_REVIVE, IT_ETHER};

static void roll_shop(int wave)
{
    int i, j, tries = 0;
    shopWave = wave;
    for (i = 0; i < 3; i++) {
        int it;
        do {
            it = roll_item(wave);
            for (j = 0; j < i; j++)
                if (shopItems[j] == it) break;
            tries++;
        } while ((j < i || it == IT_POTION || it == IT_REVIVE || it == IT_ETHER) && tries < 80);
        shopItems[i] = it;
        shopSold[i] = 0;
    }
}

void shop_screen(void)
{
    if (shopWave != run.wave) roll_shop(run.wave);
    for (;;) {
        char labels[6][28], subs[6][28];
        u16 cols[6];
        u8 en[6];
        int i, r, it;
        for (i = 0; i < 6; i++) {
            int rnd_slot = (i % 2 == 0);
            it = rnd_slot ? shopItems[i / 2] : shopFixed[i / 2];
            if (rnd_slot && shopSold[i / 2]) {
                snprintf(labels[i], 28, "Vendu");
                subs[i][0] = 0;
                cols[i] = COL(90, 90, 100);
                en[i] = 0;
                continue;
            }
            item_label(it, labels[i], subs[i]);
            snprintf(subs[i], 28, "%d G", item_price(it));
            cols[i] = tier_color(itemDefs[it].tier);
            en[i] = run.money >= item_price(it);
        }
        ui_msg_nowait("Boutique : %d G. A gauche : stock du jour. A droite : toujours dispo.", run.money);
        r = ui_choose(BM_LIST, "Boutique", 6, (const char (*)[28])labels, (const char (*)[28])subs, cols, en, 2, 1);
        if (r < 0) return;
        it = (r % 2 == 0) ? shopItems[r / 2] : shopFixed[r / 2];
        if (itemDefs[it].kind == 1) {
            run.money -= item_price(it);
            give_item(it, 0);
            if (r % 2 == 0) shopSold[r / 2] = 1;
        } else if (item_pick_target(it)) {
            run.money -= item_price(it);
            if (r % 2 == 0) shopSold[r / 2] = 1;
        }
    }
}

void reward_screen(int wave)
{
    int items[3];
    int rerollCost;
    roll_rewards(items, wave);
    roll_shop(wave);
    run.rerolls = 0;
    ui_reset_scene();
    scene.showParty = 1;
    for (;;) {
        char labels[8][28], subs[8][28];
        u16 cols[8];
        u8 en[8];
        int i, r, n = 0;
        rerollCost = 100 + wave * 4 + run.rerolls * 100;
        ui_msg_nowait("Choisis une récompense ! (Argent : %d G)", run.money);
        for (i = 0; i < 3; i++) {
            item_label(items[i], labels[n], subs[n]);
            cols[n] = tier_color(itemDefs[items[i]].tier);
            en[n++] = 1;
        }
        snprintf(labels[n], 28, "Equipe");
        snprintf(subs[n], 28, "Voir / réordonner");
        cols[n] = COL(70, 140, 100);
        en[n++] = 1;
        snprintf(labels[n], 28, "Relancer");
        snprintf(subs[n], 28, "%d G", rerollCost);
        cols[n] = COL(150, 110, 60);
        en[n] = run.money >= rerollCost;
        n++;
        snprintf(labels[n], 28, "Boutique");
        snprintf(subs[n], 28, "Acheter des objets");
        cols[n] = COL(180, 120, 60);
        en[n++] = 1;
        snprintf(labels[n], 28, "Passer");
        snprintf(subs[n], 28, "Aucune récompense");
        cols[n] = COL(120, 60, 60);
        en[n++] = 1;
        r = ui_choose(BM_LIST, "Récompenses", n, (const char (*)[28])labels, (const char (*)[28])subs, cols, en, 2, 0);
        if (r >= 0 && r < 3) {
            int it = items[r];
            if (itemDefs[it].kind == 1) { give_item(it, 0); break; }
            if (item_pick_target(it)) break;
        } else if (r == 3) ui_party_screen();
        else if (r == 4) {
            run.money -= rerollCost;
            run.rerolls++;
            roll_rewards(items, wave);
        } else if (r == 5) shop_screen();
        else if (r == 6) break;
    }
    scene.showParty = 0;
}

/* ------------------------------------------------------------ sauvegarde */
static int fatOk;

int save_available(void)
{
    FILE *f;
    u32 magic = 0;
    if (!fatOk) return 0;
    f = fopen(SAVE_FILE, "rb");
    if (!f) return 0;
    if (fread(&magic, 4, 1, f) != 1) magic = 0;
    fclose(f);
    return magic == SAVE_MAGIC;
}

void save_run(void)
{
    FILE *f;
    if (!fatOk) return;
    run.magic = SAVE_MAGIC;
    f = fopen(SAVE_FILE, "wb");
    if (!f) return;
    fwrite(&run, sizeof(Run), 1, f);
    fclose(f);
}

int load_run(void)
{
    FILE *f;
    Run tmp;
    if (!fatOk) return 0;
    f = fopen(SAVE_FILE, "rb");
    if (!f) return 0;
    if (fread(&tmp, sizeof(Run), 1, f) != 1) { fclose(f); return 0; }
    fclose(f);
    if (tmp.magic != SAVE_MAGIC || tmp.partyCount < 1 || tmp.partyCount > MAX_PARTY || tmp.wave < 1 || tmp.wave > MAX_WAVE || tmp.biome < 0 || tmp.biome >= BIOME_COUNT)
        return 0;
    run = tmp;
    return 1;
}

void save_delete(void)
{
    if (fatOk) remove(SAVE_FILE);
}

/* ------------------------------------------------------------ sélection des starters */
static int startList[SPECIES_COUNT], startN;
static int stCursor, stPage, stSel[MAX_PARTY], stSelN, stPoints;
#define ST_COLS 5
#define ST_ROWS 4
#define ST_PER_PAGE (ST_COLS * ST_ROWS)

static int st_pages(void) { return (startN + ST_PER_PAGE - 1) / ST_PER_PAGE; }
static int st_selected(int sp)
{
    int i;
    for (i = 0; i < stSelN; i++)
        if (stSel[i] == sp) return 1;
    return 0;
}

static void st_toggle(int sp)
{
    int i;
    if (st_selected(sp)) {
        for (i = 0; i < stSelN; i++)
            if (stSel[i] == sp) {
                stPoints += starterCost[sp];
                for (; i < stSelN - 1; i++) stSel[i] = stSel[i + 1];
                stSelN--;
                break;
            }
        audio_sfx(SFX_CANCEL);
    } else if (stSelN < MAX_PARTY && starterCost[sp] <= stPoints) {
        stSel[stSelN++] = sp;
        stPoints -= starterCost[sp];
        audio_sfx(SFX_SELECT);
    } else audio_sfx(SFX_WEAK);
}

static void st_top(void)
{
    int sp = startList[stCursor], i;
    const Species *s = &species[sp];
    static const char *const sn[6] = {"PV", "Atq", "Déf", "AtqSp", "DéfSp", "Vit"};
    gfx_gradient(0, SCR_H, COL(60, 110, 200), COL(190, 225, 245));
    gfx_gradient(110, SCR_H, COL(100, 190, 90), COL(60, 140, 70));
    gfx_ellipse(60, 112, 50, 9, COL(80, 160, 70));
    sprite_draw(sp, 0, 0, 60, 112, 2, 0);
    gfx_box(116, 6, 134, 104, COL(24, 32, 64), COL(225, 228, 240));
    gfx_text_shadow(122, 10, s->name, WHITE, BLACK);
    gfx_printf(122, 22, COL(255, 235, 140), "N°%d  Coût : %d", sp, starterCost[sp]);
    ui_type_badge(122, 34, s->t1);
    if (s->t2 != NO_TYPE) ui_type_badge(146, 34, s->t2);
    for (i = 0; i < 6; i++) {
        int y = 46 + i * 10, w = s->st[i] * 60 / 160;
        gfx_text(122, y, sn[i], COL(190, 210, 255));
        gfx_rect(154, y + 1, 60, 5, COL(30, 36, 60));
        gfx_rect(154, y + 1, w > 60 ? 60 : w, 5, i == 0 ? COL(80, 210, 110) : COL(240, 190, 70));
        gfx_printf(218, y, WHITE, "%d", s->st[i]);
    }
    gfx_box(4, 118, 248, 70, COL(24, 32, 64), COL(225, 228, 240));
    gfx_printf(10, 122, COL(255, 235, 140), "Equipe de départ  -  Points restants : %d/%d", stPoints, START_POINTS);
    for (i = 0; i < stSelN; i++) {
        int x = 10 + i * 40;
        sprite_draw_icon(stSel[i], x, 138);
        gfx_printf(x + 12, 172, COL(255, 235, 140), "%d", starterCost[stSel[i]]);
    }
    if (!stSelN) gfx_text(10, 148, "Touche un Pokémon pour l'ajouter.", COL(200, 210, 240));
}

static void st_bottom(void)
{
    int i, base = stPage * ST_PER_PAGE;
    gfx_gradient(0, SCR_H, COL(26, 34, 70), COL(12, 16, 36));
    ui_draw_button(4, 2, 34, 15, COL(80, 100, 180), 0, 1);
    gfx_text_center(21, 5, "<", WHITE);
    ui_draw_button(218, 2, 34, 15, COL(80, 100, 180), 0, 1);
    gfx_text_center(235, 5, ">", WHITE);
    gfx_printf(78, 5, WHITE, "Page %d/%d  (L/R)", stPage + 1, st_pages());
    for (i = 0; i < ST_PER_PAGE; i++) {
        int idx = base + i, x, y;
        if (idx >= startN) break;
        x = 8 + (i % ST_COLS) * 48;
        y = 20 + (i / ST_COLS) * 38;
        {
            int sp = startList[idx];
            int sel = st_selected(sp), cur = (idx == stCursor), afford = starterCost[sp] <= stPoints || sel;
            gfx_box(x, y, 46, 36, sel ? COL(40, 110, 60) : afford ? COL(36, 48, 100) : COL(40, 40, 48), cur ? WHITE : COL(90, 100, 150));
            sprite_draw_icon(sp, x + 7, y + 1);
            gfx_printf(x + 3, y + 26, afford ? COL(255, 235, 140) : COL(140, 140, 150), "%d", starterCost[sp]);
            if (sel) gfx_text(x + 36, y + 26, "v", COL(140, 255, 160));
        }
    }
    ui_draw_button(60, 173, 136, 17, stSelN ? COL(60, 170, 90) : COL(90, 90, 100), 0, stSelN > 0);
    gfx_text_center(128, 177, "COMMENCER (START)", WHITE);
}

int starter_select(void)
{
    int i, result = 0;
    startN = 0;
    for (i = 1; i <= SPECIES_COUNT; i++)
        if (starterCost[i] > 0) startList[startN++] = i;
    stCursor = 0;
    stPage = 0;
    stSelN = 0;
    stPoints = START_POINTS;
    ui_reset_scene();
    ui_set_custom(st_top, st_bottom);
    uiScreen = 1;
    ui_fade_in();
    for (;;) {
        frame();
        if (touchNew) {
            int tx = touchX, ty = touchY;
            if (ty >= 2 && ty < 17 && tx >= 4 && tx < 38) { stPage = (stPage + st_pages() - 1) % st_pages(); stCursor = stPage * ST_PER_PAGE; audio_sfx(SFX_SELECT); }
            else if (ty >= 2 && ty < 17 && tx >= 218 && tx < 252) { stPage = (stPage + 1) % st_pages(); stCursor = stPage * ST_PER_PAGE; audio_sfx(SFX_SELECT); }
            else if (ty >= 173 && ty < 190 && tx >= 60 && tx < 196) { if (stSelN) { result = 1; break; } }
            else if (ty >= 20 && ty < 172 && tx >= 8 && tx < 248) {
                int c = (tx - 8) / 48, r = (ty - 20) / 38, idx = stPage * ST_PER_PAGE + r * ST_COLS + c;
                if (idx < startN) {
                    stCursor = idx;
                    st_toggle(startList[idx]);
                }
            }
        }
        if (kDown & KEY_RIGHT) { if (stCursor + 1 < startN) stCursor++; }
        if (kDown & KEY_LEFT) { if (stCursor > 0) stCursor--; }
        if (kDown & KEY_DOWN) { if (stCursor + ST_COLS < startN) stCursor += ST_COLS; }
        if (kDown & KEY_UP) { if (stCursor - ST_COLS >= 0) stCursor -= ST_COLS; }
        if (kDown & KEY_R) { stCursor += ST_PER_PAGE; if (stCursor >= startN) stCursor = startN - 1; }
        if (kDown & KEY_L) { stCursor -= ST_PER_PAGE; if (stCursor < 0) stCursor = 0; }
        stPage = stCursor / ST_PER_PAGE;
        if (kDown & KEY_A) st_toggle(startList[stCursor]);
        if ((kDown & KEY_START) && stSelN) { result = 1; break; }
        if (kDown & KEY_B) { result = 0; break; }
    }
    uiScreen = 0;
    ui_set_custom(0, 0);
    return result;
}

/* ------------------------------------------------------------ déroulement */
static int wave_level(int wave) { return 3 + (wave - 1) * 19 / 20; }

void new_run_setup(void)
{
    int i;
    memset(&run, 0, sizeof(run));
    run.wave = 1;
    run.biome = 0;
    run.money = 300;
    run.balls[0] = 5;
    run.partyCount = stSelN;
    for (i = 0; i < stSelN; i++) mon_init(&run.party[i], stSel[i], 5);
}

int biome_choose(void)
{
    char labels[3][28], subs[3][28];
    u16 cols[3];
    int i, r;
    for (i = 0; i < 3; i++) {
        const Biome *b = &biomes[biomes[run.biome].links[i]];
        snprintf(labels[i], 28, "%s", b->name);
        snprintf(subs[i], 28, "%s / %s", typeNames[b->types[0]], typeNames[b->types[1]]);
        cols[i] = COL(60 + i * 40, 110, 190 - i * 30);
    }
    ui_reset_scene();
    ui_msg_nowait("Un nouveau biome t'attend. Où aller ?");
    r = ui_choose(BM_LIST, "Choisis ta destination", 3, (const char (*)[28])labels, (const char (*)[28])subs, cols, 0, 1, 0);
    if (r < 0) r = 0;
    run.biome = biomes[run.biome].links[r];
    return run.biome;
}

static void build_wave(BattleSetup *bs, int wave)
{
    int wl = wave_level(wave), i;
    memset(bs, 0, sizeof(*bs));
    bs->wave = wave;
    bs->canCatch = 1;
    bs->isBoss = (wave % 10 == 0);
    bs->isTrainer = (!bs->isBoss && wave % 5 == 0 && wave > 5);
    if (bs->isBoss) {
        int sp, lvl = wl + 2 + wave / 25;
        if (wave >= 200) { sp = 150; lvl = wl + 20; }
        else if (wave == 150) sp = 151;
        else if (wave == 100) sp = 149;
        else if (wave == 50) sp = 144 + rnd(3);
        else sp = pick_wild_species(run.biome, lvl, 1);
        bs->enemyCount = 1;
        mon_init(&bs->enemies[0], sp, lvl);
        bs->enemies[0].stat[0] = bs->enemies[0].stat[0] * (wave < 30 ? 3 : 4) / 2;
        bs->enemies[0].hp = bs->enemies[0].stat[0];
        bs->enemies[0].shiny = 0;
        bs->canCatch = 0;
        bs->moneyReward = 60 + wave * 12;
    } else if (bs->isTrainer) {
        int cnt = wave < 30 ? 2 : 3;
        bs->enemyCount = cnt;
        for (i = 0; i < cnt; i++) mon_init(&bs->enemies[i], pick_wild_species(run.biome, wl, 0), wl > 1 ? wl - 1 + i / 2 : 1);
        bs->canCatch = 0;
        bs->moneyReward = 40 + wave * 8;
    } else {
        int lvl = wl + rnd(3) - 1;
        if (lvl < 1) lvl = 1;
        bs->enemyCount = 1;
        mon_init(&bs->enemies[0], pick_wild_species(run.biome, lvl, 0), lvl);
        bs->moneyReward = 15 + wave * 5;
    }
    {
        int amulet = 0;
        for (i = 0; i < run.partyCount; i++) amulet += run.party[i].held[HI_AMULET];
        bs->moneyReward = bs->moneyReward * (100 + 20 * amulet) / 100;
    }
}

void game_over(int won)
{
    ui_set_end_won(won);
    audio_music(won ? MUS_VICTORY : MUS_TITLE);
    ui_draw_gameover();
    ui_reset_scene();
    ui_draw_gameover();
    ui_fade_in();
    {
        char labels[1][28] = {"Retour au titre"};
        ui_choose(BM_LIST, won ? "Tu as terminé la partie !" : "Ton équipe est K.O.", 1, (const char (*)[28])labels, 0, 0, 0, 1, 0);
    }
    ui_set_custom(0, 0);
    save_delete();
}

static void run_loop(void)
{
    for (;;) {
        BattleSetup bs;
        int res, i;
        if (run.wave > MAX_WAVE) {
            game_over(1);
            return;
        }
        if (run.wave > 1 && (run.wave - 1) % 10 == 0 && run.wave != run.bestWave + 0 && 0) { }
        save_run();
        build_wave(&bs, run.wave);
        ui_fade_out();
        ui_reset_scene();
        res = battle_run(&bs);
        battle_clear();
        if (res == BR_LOSE) {
            ui_fade_out();
            game_over(0);
            return;
        }
        if (res == BR_WIN || res == BR_CAUGHT) {
            run.money += bs.moneyReward;
            for (i = 0; i < run.partyCount; i++)
                if (run.party[i].hp > 0) heal_mon(&run.party[i], run.party[i].stat[0] * POST_WAVE_HEAL_PCT / 100 + 1);
            for (i = 0; i < run.partyCount; i++) {
                run.party[i].status = 0;
            }
            if (run.wave > run.bestWave) run.bestWave = run.wave;
            if (res == BR_WIN) {
                audio_music(MUS_VICTORY);
                ui_msg("Récompense : %d G !", bs.moneyReward);
            }
            ui_fade_out();
            audio_music(MUS_SHOP);
            ui_reset_scene();
            ui_fade_in();
            if (run.wave < MAX_WAVE) reward_screen(run.wave);
        } else if (res == BR_FLED) {
            if (run.wave > run.bestWave) run.bestWave = run.wave;
        }
        if (run.wave % 10 == 0 && run.wave < MAX_WAVE) {
            ui_fade_out();
            biome_choose();
        }
        run.wave++;
    }
}

#define CFG_FILE "PokeRogueDS.cfg"
static void cfg_save(void)
{
    FILE *f = fopen(CFG_FILE, "wb");
    if (f) { fputc(gameSpeed, f); fputc(battleMusicSetting, f); fputc(musicOn, f); fclose(f); }
}
static void cfg_load(void)
{
    FILE *f = fopen(CFG_FILE, "rb");
    if (f) {
        int v = fgetc(f), m = fgetc(f), o = fgetc(f);
        fclose(f);
        if (v >= 1 && v <= 4) gameSpeed = v;
        if (m >= 0 && m <= 4) battleMusicSetting = m;
        if (o == 0 || o == 1) musicOn = o;
    }
}

static void options_screen(void)
{
    static const char *const musNames[5] = {"Auto", "Classique", "Route", "Epique", "Perso (SD)"};
    for (;;) {
        char labels[4][28], subs[4][28];
        int r;
        snprintf(labels[0], 28, "Vitesse du jeu");
        snprintf(subs[0], 28, "x%d  (x1 a x4)", gameSpeed);
        snprintf(labels[1], 28, "Musique de combat");
        snprintf(subs[1], 28, "%s", musNames[battleMusicSetting]);
        snprintf(labels[2], 28, "Musique");
        snprintf(subs[2], 28, "%s", musicOn ? "Activee" : "Coupee");
        snprintf(labels[3], 28, "Retour");
        subs[3][0] = 0;
        r = ui_choose(BM_LIST, "OPTIONS", 4, (const char (*)[28])labels, (const char (*)[28])subs, 0, 0, 1, 0);
        if (r == 0) gameSpeed = gameSpeed % 4 + 1;
        else if (r == 1) { battleMusicSetting = (battleMusicSetting + 1) % 5; audio_music(audio_battle_track(0, 0, 0)); }
        else if (r == 2) { musicOn = !musicOn; audio_music(MUS_TITLE); }
        else break;
        cfg_save();
    }
    audio_music(MUS_TITLE);
}

static void help_screen(void)
{
    ui_reset_scene();
    ui_msg_nowait("Tout se fait au stylet ou avec les touches : A valide, B annule, D-pad déplace.");
    ui_wait_key();
    ui_msg_nowait("Capture des Pokémon, combats chaque vague, boss tous les 10 niveaux. 200 vagues !");
    ui_wait_key();
    ui_msg_nowait("Choisis une récompense après chaque combat. Si ton équipe tombe, la partie est perdue.");
    ui_wait_key();
    ui_msg_nowait("En combat : touche MÉGA, DYNAMAX ou TÉRA (ou X) puis choisis l'attaque. Une fois par combat chacun.");
    ui_wait_key();
    ui_msg_nowait("Menu titre > Options : vitesse x1 à x4 et choix de la musique de combat.");
    ui_wait_key();
}

void game_main_loop(void)
{
    cfg_load();
    for (;;) {
        char labels[4][28];
        int n = 0, hasSave, r, contIdx = -1, newIdx, helpIdx, speedIdx;
        ui_reset_scene();
        audio_music(MUS_TITLE);
        ui_title_screen_draw();
        hasSave = save_available();
        if (hasSave) { snprintf(labels[n], 28, "Continuer"); contIdx = n++; }
        snprintf(labels[n], 28, "Nouvelle partie");
        newIdx = n++;
        snprintf(labels[n], 28, "Aide");
        helpIdx = n++;
        snprintf(labels[n], 28, "Options");
        speedIdx = n++;
        ui_fade_in();
        r = ui_choose(BM_LIST, "POKEROGUE DS", n, (const char (*)[28])labels, 0, 0, 0, 1, 0);
        rng_seed((u32)time(NULL) * 2654435761u ^ (frameCount * 40503u) ^ 0xA5A5u);
        ui_set_custom(0, 0);
        if (r == helpIdx) { help_screen(); continue; }
        if (r == speedIdx) { options_screen(); continue; }
        if (r == contIdx && contIdx >= 0) {
            if (!load_run()) continue;
            audio_music(MUS_BATTLE);
            run_loop();
        } else if (r == newIdx) {
            ui_fade_out();
            if (!starter_select()) continue;
            new_run_setup();
            run_loop();
        }
    }
}

void game_init_fat(void)
{
    fatOk = fatInitDefault() ? 1 : 0;
}
