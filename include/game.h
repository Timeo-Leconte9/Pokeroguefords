#ifndef GAME_H
#define GAME_H

#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPECIES_COUNT 151
#define TYPE_COUNT 18
#define MAX_PARTY 6
#define MAX_WAVE 200
#define NO_TYPE 255

#include "data_ids.h"

/* ---------------------------------------------------------------- données */
typedef struct {
    const char *name;
    u8 t1, t2;
    u8 st[6];            /* PV, Atq, Déf, AtqSpé, DéfSpé, Vit */
    u8 evoTo, evoLvl;
    u8 catchRate;
    u16 baseExp;
    char arch;           /* archétype de sprite */
    u8 legend;
    u16 color;           /* couleur forcée du sprite (0 = couleur du type) */
} Species;

typedef struct {
    const char *name;
    u8 type, cat, power, acc, pp;
    s8 prio;
    u8 eff, chance;
    u8 p1;
    s8 p2;
    u8 p3;
} Move;

typedef struct {
    const char *name;
    u8 types[4];
    u8 links[3];
} Biome;

extern const char *const typeNames[TYPE_COUNT];
extern const u16 typeColors[TYPE_COUNT];
extern const u8 typeChart[TYPE_COUNT][TYPE_COUNT];
extern const char *const natureNames[25];
extern const Species species[SPECIES_COUNT + 1];
extern const u8 starterCost[SPECIES_COUNT + 1];
extern const Move moves[MOVE_TOTAL];
extern const u8 tierMoves[TYPE_COUNT][4][2];
extern const Biome biomes[BIOME_COUNT];

enum { CAT_PHYS = 0, CAT_SPEC = 1, CAT_STAT = 2 };
enum { STAT_HP = 0, STAT_ATK, STAT_DEF, STAT_SPA, STAT_SPD, STAT_SPE };
enum { ST_NONE = 0, ST_BRN, ST_PAR, ST_PSN, ST_TOX, ST_SLP, ST_FRZ, ST_CNF };

/* ---------------------------------------------------------------- objets */
enum {
    IT_POTION, IT_SUPERPOTION, IT_HYPERPOTION, IT_FULLRESTORE, IT_REVIVE, IT_MAXREVIVE,
    IT_ETHER, IT_FULLHEAL, IT_RARECANDY, IT_BALL_POKE, IT_BALL_GREAT, IT_BALL_ULTRA, IT_BALL_MASTER,
    IT_BALL_ROGUE,
    IT_LEFTOVERS, IT_LIFEORB, IT_SCOPE, IT_QUICKCLAW, IT_LUCKYEGG, IT_SITRUS, IT_TYPEBOOST, IT_AMULET,
    IT_COUNT
};
/* objets tenus (index dans Mon.held) */
enum { HI_LEFTOVERS, HI_LIFEORB, HI_SCOPE, HI_QUICKCLAW, HI_LUCKYEGG, HI_SITRUS, HI_TYPEBOOST, HI_AMULET, HI_COUNT };

typedef struct {
    const char *name;
    const char *desc;
    u8 tier;        /* 0 commun, 1 super, 2 ultra, 3 rogue, 4 master */
    u16 price;
    u8 kind;        /* 0 soin, 1 balle, 2 tenu, 3 autre */
} ItemDef;
extern const ItemDef itemDefs[IT_COUNT];

/* ---------------------------------------------------------------- Pokémon */
typedef struct {
    u8 species;
    u8 nature;
    u8 iv[6];
    u8 shiny;
    int level;
    int exp;
    int hp;
    int stat[6];         /* stat[0] = PV max */
    u8 moves[4];
    u8 pp[4], ppmax[4];
    u8 status, statusTurns;
    u8 held[HI_COUNT];
    u8 boostType;
    /* états temporaires de combat (Méga / Dynamax / Téra) */
    u8 gim;              /* 0 aucun, 1 méga, 2 dynamax, 3 téra */
    u8 tOv, tmpT1, tmpT2, dynaTurns;
    int bakStat[6];
    int bakHp;
} Mon;

typedef struct {
    Mon *mon;
    s8 stage[5];         /* Atq, Déf, AtqSpé, DéfSpé, Vit */
    u8 confused, flinch, toxic;
    u8 critBoost;
    u8 side;             /* 0 joueur, 1 ennemi */
} Fighter;

/* ---------------------------------------------------------------- partie */
typedef struct {
    u32 magic;
    int wave;
    int money;
    int biome;
    int partyCount;
    Mon party[MAX_PARTY];
    u8 balls[5];       /* Poké, Super, Hyper, Rogue, Master */
    u32 seed;
    int rerolls;
    int bestWave;
    int caught;
    int kills;
    u32 frames;
} Run;
extern Run run;

/* ---------------------------------------------------------------- aléatoire */
void rng_seed(u32 s);
u32 rng(void);
int rnd(int n);              /* 0..n-1 */
extern u32 rngState;

/* ---------------------------------------------------------------- graphismes */
#define SCR_W 256
#define SCR_H 192
#define COL(r, g, b) (RGB15((r) >> 3, (g) >> 3, (b) >> 3) | BIT(15))

extern u32 kDown, kHeld;
extern int touchNew, touchHeld, touchX, touchY;
extern u32 frameCount;

void gfx_init(void);
void gfx_target(int screen);         /* 0 = haut, 1 = bas */
void gfx_present(void);              /* attend le VBlank et copie vers l'écran */
void gfx_clear(u16 c);
void gfx_rect(int x, int y, int w, int h, u16 c);
void gfx_shade(int x, int y, int w, int h, int amount256);  /* assombrit (0..256) */
void gfx_tint(int x, int y, int w, int h, u16 c, int amount256);
void gfx_box(int x, int y, int w, int h, u16 fill, u16 border);
void gfx_ellipse(int cx, int cy, int rx, int ry, u16 c);
void gfx_gradient(int y0, int y1, u16 c0, u16 c1);
void gfx_hline(int x, int y, int w, u16 c);
void gfx_pixel(int x, int y, u16 c);
void gfx_copy_from(const u16 *src);  /* copie une image 256x192 */
void gfx_save_to(u16 *dst);
int gfx_text_width(const char *s);
void gfx_text(int x, int y, const char *s, u16 c);
void gfx_text_shadow(int x, int y, const char *s, u16 c, u16 shadow);
void gfx_text_center(int cx, int y, const char *s, u16 c);
void gfx_text_big(int x, int y, const char *s, u16 c, int scale);
extern int gfxClipY1;
void gfx_set_clip(int x, int y, int w, int h);
void gfx_reset_clip(void);
void gfx_mark(int screen, int x, int y, int w, int h);
void gfx_mark_all(int screen);
void gfx_text_right(int rx, int y, const char *s, u16 c);
void gfx_printf(int x, int y, u16 c, const char *fmt, ...);
void gfx_blit(const u16 *src, int sw, int sh, int x, int y, int scale, int flip, int white256);
u16 gfx_mix(u16 a, u16 b, int amount256);   /* 0 = a, 256 = b */
u16 gfx_lighten(u16 c, int amount256);
u16 gfx_darken(u16 c, int amount256);
void input_poll(void);

/* ---------------------------------------------------------------- sprites */
#define SPR 32
const u16 *sprite_get(int speciesId, int back, int shiny);   /* 32x32 RGB15 (bit15 = opaque) */
void sprite_draw(int speciesId, int back, int shiny, int cx, int baseY, int scale, int white256);
void sprite_draw_icon(int speciesId, int x, int y);
void draw_trainer(int cx, int baseY, int scale, int seed);

/* ---------------------------------------------------------------- audio */
enum { MUS_NONE = 0, MUS_TITLE, MUS_BATTLE, MUS_BOSS, MUS_SHOP, MUS_VICTORY, MUS_BATTLE2, MUS_TRAINER, MUS_CUSTOM, MUS_COUNT };
enum { SFX_HIT, SFX_SUPER, SFX_WEAK, SFX_FAINT, SFX_LEVELUP, SFX_BALL, SFX_CATCH, SFX_SELECT, SFX_CANCEL, SFX_HEAL, SFX_COUNT };
void audio_init(void);
void audio_music(int id);
int audio_battle_track(int isBoss, int isTrainer, int biome);
extern int battleMusicSetting;   /* 0 auto, 1 classique, 2 route, 3 épique, 4 perso (carte SD) */
extern int musicOn;
void audio_sfx(int id);
void audio_update(void);
int audio_ready(void);

/* ---------------------------------------------------------------- monstres / combat */
int mon_calc_stat(int idx, const Species *sp, const Mon *m);
int mon_ability(const Mon *m);
const char *ability_name(int ab);
void mon_recalc(Mon *m);
void mon_init(Mon *m, int speciesId, int level);
void mon_fill_moves(Mon *m);
int mon_exp_for(int level, int speciesId);
int mon_can_learn_at(int speciesId, int level, int *moveOut);   /* move appris pile à ce niveau ? */
const char *mon_name(const Mon *m);
int mon_alive(const Mon *m);
int type_mult4(int moveType, int t1, int t2);   /* x4 : 0,1,2,4,8,16 */
int evo_stage_level(int speciesId);
int pick_wild_species(int biome, int level, int boss);

/* ---------------------------------------------------------------- interface */
enum { BM_NONE = 0, BM_INFO, BM_MENU, BM_MOVES, BM_LIST, BM_CUSTOM };

typedef struct {
    int visible;
    int species;
    int level;
    int hp, hpMax;       /* valeurs réelles */
    int hpShown;         /* valeur animée */
    int status;
    int shiny;
    int boss;
    int expPct;          /* 0..100 (joueur) */
    int dx, dy;          /* décalage d'animation */
    int white;           /* flash blanc 0..256 */
    int fade;            /* 0..256 : disparition */
    int hidden;
    int ballPhase;       /* animation Poké Ball */
    char name[24];
    int stages[5];
    int gim;
    int t1, t2;
} Actor;

typedef struct {
    int biome;
    Actor ally, foe;
    char msg[160];
    int msgLen;          /* caractères affichés */
    int msgFull;
    int showWave;
    int wave;
    int flash;           /* flash écran blanc */
    int shake;
    int trainer;         /* 1 = dresseur adverse affiché */
    int trainerSeed;
    int ballX, ballY, ballShow, ballAngle;  /* Poké Ball lancée */
    int fx;              /* effet visuel d'attaque : type+1 */
    int fxT;
    int fxTarget;        /* 0 = allié, 1 = ennemi */
    int showParty;
} Scene;
extern Scene scene;

typedef struct {
    int mode;
    int count;
    int sel;
    char label[8][28];
    char sub[8][28];
    u16 color[8];
    u8 enabled[8];
    char title[40];
    int cols;
    int pageInfo;
    int icon[8];
    int hp[8], hpmax[8];
    int back;            /* bouton retour affiché */
    int moveId[8];
    int gimShow;         /* boutons Méga/Dynamax/Téra affichés (écran attaques) */
    int gimAvail[3];
    int gimSel;          /* -1 aucun, 0 méga, 1 dynamax, 2 téra */
} Bottom;
extern Bottom bottom;

void ui_set_custom(void (*top)(void), void (*bot)(void));
void ui_btn_rect(int i, int *x, int *y, int *w, int *h);
void frame(void);
void anim_frame(void);
void ui_force_redraw(void);
extern int gameSpeed;                              /* une image : rendu + attente VBlank */
void ui_reset_scene(void);
void ui_msg(const char *fmt, ...);             /* message avec attente */
void ui_msg_nowait(const char *fmt, ...);
void ui_wait(int frames);
void ui_wait_key(void);
void ui_set_foe(const Mon *m, int boss);
void ui_set_ally(const Mon *m);
void ui_sync_hp(void);
void ui_animate_hp(void);
void ui_flash(int frames);
void ui_shake(int frames);
void ui_attack_fx(int type, int toFoe);
void ui_bottom_info(void);
int ui_choose(int mode, const char *title, int n, const char labels[][28], const char subs[][28],
              const u16 *colors, const u8 *enabled, int cols, int allowBack);
int ui_main_menu(void);                        /* 0 combat 1 balle 2 équipe 3 fuite */
int ui_pick_party(const char *title, int allowBack, int onlyAlive, int onlyFainted);
void ui_party_screen(void);
void ui_draw_button(int x, int y, int w, int h, u16 color, int selected, int enabled);
void ui_draw_hpbar(int x, int y, int w, int hp, int hpMax);
void ui_status_tag(int x, int y, int st);
void ui_type_badge(int x, int y, int type);
void ui_info_card(const Mon *m);
void ui_fade_out(void);
void ui_fade_in(void);
void ui_title_screen_draw(void);
void ui_draw_gameover(void);
void ui_set_end_won(int w);
void ui_lunge(int side);

/* ---------------------------------------------------------------- combat */
typedef struct {
    int wave;
    int isTrainer;
    int isBoss;
    int enemyCount;
    Mon enemies[3];
    int enemyIdx;
    int canCatch;
    int moneyReward;
} BattleSetup;

enum { BR_WIN = 0, BR_LOSE, BR_FLED, BR_CAUGHT };
int battle_run(BattleSetup *bs);
void battle_clear(void);
void apply_exp(Mon *m, int amount);
int heal_mon(Mon *m, int amount);
void learn_moves_after_level(Mon *m, int oldLevel, int newLevel);
int check_evolution(Mon *m, int oldLevel);

/* ---------------------------------------------------------------- partie */
void game_main_loop(void);
int save_available(void);
void save_run(void);
int load_run(void);
void save_delete(void);
void new_run_setup(void);
int starter_select(void);
int biome_choose(void);
void reward_screen(int wave);
void shop_screen(void);
void give_item(int item, int tier);
int item_pick_target(int item);
void game_over(int won);
extern int uiScreen;
extern int uiWaitingChoice;

#endif
