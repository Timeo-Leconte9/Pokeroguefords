/* sound.c : musique et effets générés en PCM 8 bits (synthé chiptune). Compositions originales.
 * Aucun fichier audio externe : tout est calculé au lancement / au changement de morceau. */
#include "game.h"

#define MUS_RATE 16000
#define SFX_RATE 11025

typedef struct {
    int bpm;
    int bars;
    const char *const *lead;
    const u8 *roots;      /* tonique de chaque mesure (MIDI, grave) */
    const u8 *minor;      /* 1 = accord mineur */
    int style;            /* 0 calme, 1 combat, 2 boss */
    int loop;
    int leadDuty;         /* 0=12.5% 1=25% 2=50% */
} TrackDef;

/* ---------------------------------------------------------------- partitions */
static const char *const battleLead[16] = {
    "e5 - e5 - a5 - g5 e5 d5 - e5 - c5 - - -",
    "c5 - c5 - f5 - e5 c5 a4 - c5 - a4 - - -",
    "e5 - e5 - g5 - e5 c5 g4 - c5 - e5 - - -",
    "d5 - d5 - g5 - f5 d5 b4 - d5 - g4 - - -",
    "e5 - e5 - a5 - c6 b5 a5 - g5 - e5 - - -",
    "f5 - f5 - a5 - g5 f5 e5 - f5 - c5 - - -",
    "d5 - g5 - b5 - a5 g5 f5 - e5 - d5 - - -",
    "e5 - g#5 - b5 - g#5 e5 e5 - - - - - - -",
    "a5 - a5 e5 a5 - c6 - b5 - a5 - g5 - e5 -",
    "f5 - f5 c5 f5 - a5 - g5 - f5 - e5 - c5 -",
    "g5 - g5 e5 g5 - c6 - b5 - g5 - e5 - g5 -",
    "g5 - g5 d5 g5 - b5 - a5 - g5 - d5 - b4 -",
    "c6 - b5 - a5 - e5 - a5 - c6 - e6 - - -",
    "c6 - a5 - f5 - a5 - c6 - a5 - f5 - - -",
    "b5 - g5 - d5 - g5 - b5 - d6 - b5 - g5 -",
    "g#5 - b5 - e6 - b5 - g#5 - e5 - b4 - - -"};
static const u8 battleRoots[16] = {45, 41, 48, 43, 45, 41, 43, 40, 45, 41, 48, 43, 45, 41, 43, 40};
static const u8 battleMinor[16] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};

static const char *const titleLead[8] = {
    "e5 - - g5 - - e5 - c5 - - e5 - - - -",
    "c5 - - e5 - - a5 - - g5 - - e5 - - -",
    "a5 - - c6 - - a5 - f5 - - a5 - - - -",
    "g5 - - b5 - - d6 - - b5 - - g5 - - -",
    "g5 - e5 - c5 - e5 - g5 - c6 - - - - -",
    "e5 - a5 - c6 - a5 - e5 - c5 - a4 - - -",
    "f5 - a5 - c6 - a5 - f5 - e5 - d5 - - -",
    "d5 - g5 - b5 - g5 - d5 - - - - - - -"};
static const u8 titleRoots[8] = {48, 45, 41, 43, 48, 45, 41, 43};
static const u8 titleMinor[8] = {0, 1, 0, 0, 0, 1, 0, 0};

static const char *const bossLead[8] = {
    "d5 d5 - d5 f5 - a5 - g5 - f5 - e5 - d5 -",
    "d5 d5 - d5 f5 - a#5 - a5 - f5 - d5 - a#4 -",
    "e5 e5 - e5 g5 - c6 - b5 - g5 - e5 - c5 -",
    "c#5 c#5 - c#5 e5 - a5 - g5 - e5 - c#5 - a4 -",
    "a5 - a5 - d6 - a5 - f5 - a5 - d5 - f5 -",
    "a#5 - a#5 - d6 - a#5 - f5 - a#5 - d5 - f5 -",
    "g5 - g5 - a#5 - d6 - c6 - a#5 - g5 - d5 -",
    "a5 - c#6 - e6 - c#6 - a5 - e5 - c#5 - a4 -"};
static const u8 bossRoots[8] = {50, 46, 48, 45, 50, 46, 43, 45};
static const u8 bossMinor[8] = {1, 0, 0, 0, 1, 0, 1, 0};

static const char *const victoryLead[2] = {
    "c5 - e5 - g5 - c6 - - - g5 - c6 - - -",
    "c6 - - - - - - - - - - - - - - -"};
static const u8 victoryRoots[2] = {48, 48};
static const u8 victoryMinor[2] = {0, 0};

static const char *const trainerLead[8] = {
    "e5 e5 - g5 - e5 - b4 e5 - g5 - b5 - a5 -",
    "c5 c5 - e5 - c5 - g4 c5 - e5 - g5 - e5 -",
    "d5 d5 - f#5 - d5 - a4 d5 - f#5 - a5 - f#5 -",
    "b4 b4 - d#5 - f#5 - b5 - a5 - f#5 - d#5 - -",
    "e5 g5 b5 - a5 g5 e5 - g5 b5 e6 - d6 b5 g5 -",
    "c5 e5 g5 - c6 g5 e5 - a5 g5 e5 - c5 - - -",
    "d5 f#5 a5 - d6 a5 f#5 - c6 a5 f#5 - d5 - - -",
    "b5 - a5 - g5 - f#5 - e5 - d#5 - b4 - - -"};
static const u8 trainerRoots[8] = {40, 48, 50, 47, 40, 48, 50, 47};
static const u8 trainerMinor[8] = {1, 0, 0, 0, 1, 0, 0, 0};

static const char *const route2Lead[8] = {
    "g5 - e5 g5 - c6 - g5 - e5 - g5 - c5 - -",
    "f5 - d5 f5 - a5 - f5 - d5 - f5 - a4 - -",
    "e5 - c5 e5 - g5 - e5 - c5 - e5 - g4 - -",
    "d5 - b4 d5 - g5 - f5 - d5 - b4 - g4 - -",
    "a5 - f5 a5 - c6 - a5 - f5 - a5 - c5 - -",
    "g5 - e5 g5 - b5 - g5 - e5 - g5 - e5 - -",
    "f5 - a5 - c6 - a5 - f5 - e5 - d5 - c5 -",
    "d5 - e5 - f5 - g5 - b5 - d6 - g5 - - -"};
static const u8 route2Roots[8] = {48, 41, 48, 43, 41, 48, 41, 43};
static const u8 route2Minor[8] = {0, 0, 0, 0, 0, 0, 0, 0};

static const TrackDef tracks[MUS_COUNT] = {
    {0},
    {96, 8, titleLead, titleRoots, titleMinor, 0, 1, 2},
    {160, 16, battleLead, battleRoots, battleMinor, 1, 1, 1},
    {176, 8, bossLead, bossRoots, bossMinor, 2, 1, 1},
    {96, 8, titleLead, titleRoots, titleMinor, 0, 1, 2},
    {140, 2, victoryLead, victoryRoots, victoryMinor, 0, 0, 1},
    {150, 8, route2Lead, route2Roots, route2Minor, 1, 1, 2},
    {168, 8, trainerLead, trainerRoots, trainerMinor, 1, 1, 1},
    {0}};

/* ---------------------------------------------------------------- synthé */
static const u16 base16[12] = {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494};   /* Hz x16 (octave 0) */

static u32 phase_inc(int midi, int rate)
{
    int oct = midi / 12 - 1;   /* C4 = 60 -> octave 4 */
    u32 f16 = base16[midi % 12];
    u32 v;
    if (oct < 0) oct = 0;
    v = (f16 << 12) << oct;    /* Hz*16*4096 */
    return v / (u32)rate;      /* = Hz * 65536 / rate */
}

static int parse_note(const char *s, int *len)
{
    static const int semi[7] = {9, 11, 0, 2, 4, 5, 7};   /* a b c d e f g */
    int n, oct, m;
    if (*s < 'a' || *s > 'g') { *len = 0; return 0; }
    n = semi[*s - 'a'];
    s++;
    *len = 1;
    if (*s == '#') { n++; s++; (*len)++; }
    oct = *s - '0';
    (*len)++;
    m = 12 * (oct + 1) + n;
    return m;
}

static int square(u32 ph, int duty)
{
    u32 thr = duty == 0 ? 8192 : duty == 1 ? 16384 : 32768;
    return ((ph & 0xFFFF) < thr) ? 1 : -1;
}
static int triangle(u32 ph)
{
    int p = (int)(ph & 0xFFFF);
    int v = p < 32768 ? p : 65535 - p;       /* 0..32767 */
    return (v - 16384) >> 7;                 /* -128..127 */
}

typedef struct {
    int midi;
    int age, len;
    u32 ph;
} Voice;

static int env(const Voice *v)
{
    int a = 256;
    int rel = v->len / 12;
    if (v->age < 40) a = v->age * 256 / 40;
    else a = 256 - (v->age * 90) / (v->len ? v->len : 1);
    if (v->len - v->age < rel) a = a * (v->len - v->age) / (rel ? rel : 1);
    if (a < 0) a = 0;
    return a;
}

static s8 *render_track(int id, int *outLen, int *outRate)
{
    const TrackDef *td = &tracks[id];
    int rate = MUS_RATE;
    int steps = td->bars * 16;
    int sps = rate * 60 / (td->bpm * 4);
    int total = steps * sps;
    s8 *buf;
    int step, i;
    Voice lead = {0, 0, 0, 0}, bass = {0, 0, 0, 0}, arp = {0, 0, 0, 0};
    int kickAge = 100000, snareAge = 100000, hatAge = 100000;
    u32 lfsr = 0xACE1u;
    u32 kickPh = 0;

    total = (total + 3) & ~3;
    buf = (s8 *)malloc((size_t)total + 8);
    if (!buf) return 0;
    memset(buf, 0, (size_t)total + 8);

    for (step = 0; step < steps; step++) {
        int bar = step / 16, st = step % 16;
        const char *tok = td->lead[bar];
        int k, tokIdx = 0;
        int root = td->roots[bar], minor = td->minor[bar];
        int bassNote = 0, arpNote = 0;
        int tri[3];
        int start = step * sps;

        /* ---- jeton de mélodie */
        while (*tok && tokIdx < st) {
            while (*tok && *tok != ' ') tok++;
            while (*tok == ' ') tok++;
            tokIdx++;
        }
        if (*tok == '.') lead.midi = 0;
        else if (*tok == '-') { /* tenue */ }
        else {
            int nl = 0;
            int m = parse_note(tok, &nl);
            if (nl) {
                /* durée = 1 + nombre de '-' qui suivent (dans la mesure) */
                int dur = 1, tj = st + 1;
                const char *q = tok;
                while (*q && *q != ' ') q++;
                while (*q == ' ') q++;
                while (tj < 16 && *q == '-') {
                    dur++;
                    tj++;
                    while (*q && *q != ' ') q++;
                    while (*q == ' ') q++;
                }
                lead.midi = m;
                lead.age = 0;
                lead.len = dur * sps;
            }
        }

        /* ---- basse */
        tri[0] = 0;
        tri[1] = minor ? 3 : 4;
        tri[2] = 7;
        if (td->style == 0) {
            if (st == 0 || st == 8) bassNote = root;
            else if (st == 4 || st == 12) bassNote = root + 7;
        } else if (td->style == 1) {
            static const int pat[16] = {0, -1, 0, -1, 12, -1, 0, -1, 0, -1, 0, -1, 12, -1, 7, -1};
            if (pat[st] >= 0) bassNote = root + pat[st];
        } else {
            static const int pat[16] = {0, 0, 12, 0, 0, 0, 12, 0, 0, 0, 12, 0, 7, 0, 12, 0};
            bassNote = root + pat[st];
        }
        if (bassNote) {
            bass.midi = bassNote;
            bass.age = 0;
            bass.len = (td->style == 0 ? 4 : 1) * sps - sps / 8;
        }
        /* ---- arpège */
        k = st % 4;
        arpNote = root + 24 + tri[k == 3 ? 1 : k] ;
        if (td->style == 0 && (st & 1)) arpNote = 0;
        if (arpNote) {
            arp.midi = arpNote;
            arp.age = 0;
            arp.len = sps - sps / 6;
        }
        /* ---- batterie */
        if (td->style >= 1) {
            if (st == 0 || st == 8 || (td->style == 2 && (st == 3 || st == 6 || st == 11 || st == 14)) || (td->style == 1 && st == 10)) kickAge = 0;
            if (st == 4 || st == 12) snareAge = 0;
            if (st % 2 == 0) hatAge = 0;
        } else if (st == 0 || st == 8) {
            hatAge = 0;
        }

        /* ---- échantillons */
        for (i = 0; i < sps; i++) {
            int out = 0;
            if (lead.midi && lead.age < lead.len) {
                int a = env(&lead);
                lead.ph += phase_inc(lead.midi, rate);
                out += (square(lead.ph, td->leadDuty) * 34 * a) >> 8;
                lead.age++;
            }
            if (arp.midi && arp.age < arp.len) {
                int a = env(&arp);
                arp.ph += phase_inc(arp.midi, rate);
                out += (square(arp.ph, 0) * 11 * a) >> 8;
                arp.age++;
            }
            if (bass.midi && bass.age < bass.len) {
                int a = env(&bass);
                bass.ph += phase_inc(bass.midi, rate);
                out += (triangle(bass.ph) * 36 / 128 * a) >> 8;
                bass.age++;
            }
            /* bruit */
            lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
            if (kickAge < 2400) {
                int f = 160 - kickAge / 22;
                int a = 255 - kickAge * 255 / 2400;
                if (f < 40) f = 40;
                kickPh += (u32)f * 65536u / (u32)rate;
                out += (triangle(kickPh) * 40 / 128 * a) >> 8;
                kickAge++;
            }
            if (snareAge < 1400) {
                int a = 255 - snareAge * 255 / 1400;
                out += (((lfsr & 1) ? 1 : -1) * 26 * a) >> 8;
                snareAge++;
            }
            if (hatAge < 300) {
                int a = 255 - hatAge * 255 / 300;
                out += (((lfsr & 2) ? 1 : -1) * 9 * a) >> 8;
                hatAge++;
            }
            if (out > 127) out = 127;
            if (out < -127) out = -127;
            buf[start + i] = (s8)out;
        }
    }
    *outLen = total;
    *outRate = rate;
    return buf;
}

/* exporté pour les tests PC */
s8 *audio_render_track_debug(int id, int *n, int *rate) { return render_track(id, n, rate); }

/* ---------------------------------------------------------------- effets sonores */
typedef struct { s8 *data; int len; } Sfx;
static Sfx sfx[SFX_COUNT];
static int sfxReady;

static int tone(s8 *buf, int pos, int cap, int f0, int f1, int ms, int vol, int wave /*0 carré 1 triangle 2 bruit*/, int decay)
{
    int n = SFX_RATE * ms / 1000, i;
    u32 ph = 0, lf = 0xBEEFu;
    for (i = 0; i < n && pos + i < cap; i++) {
        int f = f0 + (f1 - f0) * i / (n ? n : 1);
        int a = decay ? vol * (n - i) / n : vol;
        int v;
        ph += (u32)f * 65536u / SFX_RATE;
        lf = (lf >> 1) ^ (-(lf & 1u) & 0xB400u);
        if (wave == 0) v = ((ph & 0xFFFF) < 32768 ? 1 : -1) * a;
        else if (wave == 1) v = triangle(ph) * a / 128;
        else v = ((lf & 1) ? 1 : -1) * a;
        {
            int o = buf[pos + i] + v;
            if (o > 127) o = 127;
            if (o < -127) o = -127;
            buf[pos + i] = (s8)o;
        }
    }
    return pos + n;
}

static void make_sfx(int id)
{
    int cap = SFX_RATE * 1, pos = 0;
    s8 *b = (s8 *)malloc((size_t)cap + 8);
    if (!b) return;
    memset(b, 0, (size_t)cap + 8);
    switch (id) {
    case SFX_HIT: tone(b, 0, cap, 0, 0, 110, 70, 2, 1); tone(b, 0, cap, 160, 70, 130, 50, 0, 1); pos = SFX_RATE * 130 / 1000; break;
    case SFX_SUPER: tone(b, 0, cap, 0, 0, 140, 80, 2, 1); tone(b, 0, cap, 700, 280, 160, 55, 0, 1); pos = SFX_RATE * 160 / 1000; break;
    case SFX_WEAK: tone(b, 0, cap, 130, 85, 150, 45, 0, 1); tone(b, 0, cap, 0, 0, 60, 25, 2, 1); pos = SFX_RATE * 150 / 1000; break;
    case SFX_FAINT: pos = tone(b, 0, cap, 420, 50, 600, 55, 0, 1); break;
    case SFX_LEVELUP:
        pos = tone(b, pos, cap, 523, 523, 90, 50, 0, 0);
        pos = tone(b, pos, cap, 659, 659, 90, 50, 0, 0);
        pos = tone(b, pos, cap, 784, 784, 90, 50, 0, 0);
        pos = tone(b, pos, cap, 1047, 1047, 260, 50, 0, 1);
        break;
    case SFX_BALL: pos = tone(b, 0, cap, 900, 500, 70, 45, 0, 1); break;
    case SFX_CATCH:
        pos = tone(b, pos, cap, 523, 523, 80, 45, 0, 0);
        pos = tone(b, pos, cap, 659, 659, 80, 45, 0, 0);
        pos = tone(b, pos, cap, 784, 784, 80, 45, 0, 0);
        pos = tone(b, pos, cap, 1047, 1047, 80, 45, 0, 0);
        pos = tone(b, pos, cap, 1319, 1319, 300, 45, 0, 1);
        break;
    case SFX_SELECT: pos = tone(b, 0, cap, 880, 880, 45, 40, 0, 1); break;
    case SFX_CANCEL: pos = tone(b, 0, cap, 330, 260, 70, 40, 0, 1); break;
    case SFX_HEAL: pos = tone(b, 0, cap, 600, 1300, 280, 40, 1, 1); break;
    default: break;
    }
    pos = (pos + 3) & ~3;
    sfx[id].data = b;
    sfx[id].len = pos < 8 ? 8 : pos;
}

/* ---------------------------------------------------------------- API */
static s8 *musBuf;
static int musHandle = -1;
static int curMus = MUS_NONE;
static int audioOk;

int audio_ready(void) { return audioOk; }

void audio_init(void)
{
    int i;
    soundEnable();
    audioOk = 1;
    for (i = 0; i < SFX_COUNT; i++) make_sfx(i);
    sfxReady = 1;
}

int battleMusicSetting = 0;
int musicOn = 1;

/* Musique perso : fichier PCM brut 8 bits signé, mono, 16000 Hz, posé à côté du .nds
 * (PokeRogueDS_music.raw). Voir le README pour la conversion avec ffmpeg. */
static s8 *load_custom(int *n)
{
    FILE *f = fopen("PokeRogueDS_music.raw", "rb");
    long len;
    s8 *b;
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len > 1200000L) len = 1200000L;
    len &= ~3L;
    if (len < 4000) { fclose(f); return 0; }
    b = (s8 *)malloc((size_t)len + 8);
    if (!b) { fclose(f); return 0; }
    if (fread(b, 1, (size_t)len, f) != (size_t)len) { free(b); fclose(f); return 0; }
    fclose(f);
    *n = (int)len;
    return b;
}

int audio_battle_track(int isBoss, int isTrainer, int biome)
{
    if (isBoss) return MUS_BOSS;
    switch (battleMusicSetting) {
    case 1: return MUS_BATTLE;
    case 2: return MUS_BATTLE2;
    case 3: return MUS_TRAINER;
    case 4: return MUS_CUSTOM;
    default: return isTrainer ? MUS_TRAINER : (biome & 1) ? MUS_BATTLE2 : MUS_BATTLE;
    }
}

void audio_music(int id)
{
    int n = 0, rate = MUS_RATE, loop = 1;
    if (!audioOk) return;
    if (!musicOn) id = MUS_NONE;
    if (id == curMus) return;
    if (musHandle >= 0) { soundKill(musHandle); musHandle = -1; }
    if (musBuf) { free(musBuf); musBuf = 0; }
    curMus = id;
    if (id == MUS_NONE) return;
    if (id == MUS_CUSTOM) {
        musBuf = load_custom(&n);
        if (!musBuf) { curMus = MUS_NONE; audio_music(MUS_BATTLE); return; }
    } else {
        musBuf = render_track(id, &n, &rate);
        loop = tracks[id].loop;
    }
    if (!musBuf) return;
    DC_FlushRange(musBuf, (u32)n);
    musHandle = soundPlaySample(musBuf, SoundFormat_8Bit, (u32)n, (u16)rate, 70, 64, loop ? true : false, 0);
}

void audio_sfx(int id)
{
    if (!audioOk || !sfxReady || id < 0 || id >= SFX_COUNT || !sfx[id].data) return;
    DC_FlushRange(sfx[id].data, (u32)sfx[id].len);
    soundPlaySample(sfx[id].data, SoundFormat_8Bit, (u32)sfx[id].len, SFX_RATE, 100, 64, false, 0);
}

void audio_update(void)
{
    /* le jingle de victoire ne boucle pas : rien à faire, le morceau suivant est lancé par le jeu */
}
