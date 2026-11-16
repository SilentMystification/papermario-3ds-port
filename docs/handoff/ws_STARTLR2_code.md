# ws_STARTLR2 code blocks (exact text, LF line ends). Designer to ask: the oracle.
## Block A: 3ds/include/pm_shoulder.h (replace the whole file)
```c
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
```
## Block B: 3ds/src/pm_input.c line 203 (one line, comment only)
Before: ` * STARTLR: in the start menu L gives N64 L and R gives N64 Z (pm_shoulder.h). */`
After:  ` * STARTLR2: in the start menu L gives N64 Z and R gives N64 R (pm_shoulder.h). */`
## Block C: 3ds/tools/pm_shoulder_test.c lines 7 and 8 (two lines)
Before:
```c
    { "start L", 1, 0, 0, 0, 0, 1, 0x0020 },
    { "start R", 0, 1, 0, 0, 0, 1, 0x2000 },
```
After:
```c
    { "start L", 1, 0, 0, 0, 0, 1, 0x2000 },
    { "start R", 0, 1, 0, 0, 0, 1, 0x0010 },
```
## Block D: 3ds/tests/host.d/startlr.sh lines 2 and 6 (two lines)
Line 2 after:
```sh
# STARTLR2: pm_shoulder_bits (pm_shoulder.h): in the start menu L gives N64 Z, R gives N64 R. The seed (no start menu branch) must fail "start R".
```
Line 6 after:
```sh
got=$(/tmp/startlr_seed/t); if [ "$got" != "startlr FAIL start R got 2000" ]; then echo "startlr seed FAIL: got [$got]"; exit 1; fi
```
Why the seed line is right (FACT): with the start menu branch off, start L gives sh4[0] = 0x2000 (equal to the new want), start R
gives sh4[1] = 0x2000 (not 0x0010). So the first failing case is "start R" with 2000.
