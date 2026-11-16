#ifndef PM_SHOULDER_H
#define PM_SHOULDER_H
/* STARTLR: the N64 bits of the 3DS shoulder keys. l, r, zl, zr: the key is held (0 or 1). sh4: pm_map from PM_IN_L (L, R, ZL, ZR,
 * pm_port.h:334). zcombo: the O3DS Z key is held (X, or Y with o3dsz=1). start_menu: the start (pause) menu (GAME_MODE_PAUSE).
 * STARTLR2 (user, 2026-11-16): in the start menu L gives N64 Z (0x2000, previous: src/pause/pause_tabs.c:406) and R gives N64 R
 * (0x0010, next: pause_tabs.c:417). No pause code reads N64 L. ZL and ZR keep their map. The O3DS combo comes first. */
static inline unsigned short pm_shoulder_bits(int l, int r, int zl, int zr, const unsigned short* sh4, int zcombo, int start_menu) {
    if (zcombo) return (unsigned short)((l ? 0x0020 : 0) | (r ? 0x0010 : 0));
    if (start_menu) return (unsigned short)((l ? 0x2000 : 0) | (r ? 0x0010 : 0) | (zl ? sh4[2] : 0) | (zr ? sh4[3] : 0));
    return (unsigned short)((l ? sh4[0] : 0) | (r ? sh4[1] : 0) | (zl ? sh4[2] : 0) | (zr ? sh4[3] : 0));
}
#endif
