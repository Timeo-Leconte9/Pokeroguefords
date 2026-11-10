/* Stub minimal de libnds pour compiler/tester le jeu sur PC (aucun effet sur la vraie compilation DS). */
#ifndef HOST_NDS_H
#define HOST_NDS_H
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef int8_t s8; typedef int16_t s16; typedef int32_t s32;
#define BIT(n) (1u << (n))
#define RGB15(r, g, b) ((r) | ((g) << 5) | ((b) << 10))

enum { MODE_5_2D = 5 };
enum { VRAM_A_MAIN_BG = 1, VRAM_C_SUB_BG = 4 };
enum { BgType_Bmp16 = 1 };
enum { BgSize_B16_256x256 = 1 };
#define videoSetMode(x) ((void)0)
#define videoSetModeSub(x) ((void)0)
#define vramSetBankA(x) ((void)0)
#define vramSetBankC(x) ((void)0)
#define lcdMainOnTop() ((void)0)
extern u16 host_vram[2][256 * 256];
static inline int bgInit(int l, int t, int s, int a, int b) { (void)l; (void)t; (void)s; (void)a; (void)b; return 0; }
static inline int bgInitSub(int l, int t, int s, int a, int b) { (void)l; (void)t; (void)s; (void)a; (void)b; return 1; }
static inline u16 *bgGetGfxPtr(int id) { return host_vram[id]; }
static inline void bgSetPriority(int a, int b) { (void)a; (void)b; }

#define dmaCopy(src, dst, n) memcpy((dst), (src), (n))
#define DC_FlushRange(p, n) ((void)0)
void swiWaitForVBlank(void);
#define swiWaitForVBlank swiWaitForVBlank

#define KEY_A BIT(0)
#define KEY_B BIT(1)
#define KEY_SELECT BIT(2)
#define KEY_START BIT(3)
#define KEY_RIGHT BIT(4)
#define KEY_LEFT BIT(5)
#define KEY_UP BIT(6)
#define KEY_DOWN BIT(7)
#define KEY_R BIT(8)
#define KEY_L BIT(9)
#define KEY_X BIT(10)
#define KEY_Y BIT(11)
#define KEY_TOUCH BIT(12)
typedef struct { u16 rawx, rawy, px, py, z1, z2; } touchPosition;
void scanKeys(void);
u32 keysDown(void);
u32 keysHeld(void);
void touchRead(touchPosition *t);

/* son */
enum { SoundFormat_8Bit = 1, SoundFormat_16Bit = 0 };
void soundEnable(void);
int soundPlaySample(const void *data, int fmt, u32 size, u16 freq, u8 vol, u8 pan, bool loop, u16 loopPoint);
void soundKill(int h);
void soundSetVolume(int h, u8 vol);

/* fat */
static inline bool fatInitDefault(void) { return true; }

/* timers / hasard */
#define irqEnable(x) ((void)0)
#endif
