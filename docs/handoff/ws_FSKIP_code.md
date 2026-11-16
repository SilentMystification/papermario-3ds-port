# ws_FSKIP code blocks (exact text, LF line ends). Designer to ask: the oracle.
## Block A: 3ds/include/pm_fpsmode.h (new file)
```c
/* FSKIP (user 2026-11-16): the 60 FPS option (options.ini fps60, Options row "60 FPS").
 *  0 Off: 30 FPS, no interpolation (no change).
 *  1 On: every drawn game step shows the key frame and the interpolated frame. A slow step makes the game slower; no key frame is dropped
 *    for load. The Frame skip row (fskip) still drops the draws of a late step when it is on (pm_main.c FS1).
 *  2 Dynamic: 60 FPS while the walk and the GPU fit, 30 FPS (no interpolated frame) under load, with hysteresis.
 * The fps60 launch switch with fps60=0 in options.ini acts as Dynamic (the rule before FSKIP). Integers only, no division. */
#pragma once
#define PM_FPS_OFF 0
#define PM_FPS_ON 1
#define PM_FPS_DYN 2
#define PM_DYN_W_HI 30000u /* walk us over this: heavy (30 FPS) at once */
#define PM_DYN_G_HI 15000u /* GPU us of the last frame over this: heavy at once */
#define PM_DYN_W_LO 27000u /* calm step: walk us under this */
#define PM_DYN_G_LO 12000u /* calm step: GPU us under this */
#define PM_DYN_CALM 30     /* calm steps in a row that end heavy */
typedef struct { int heavy, calm; } PmFpsDyn;
static inline int pm_fps_dyn_step(PmFpsDyn* d, unsigned w_us, unsigned g_us) {
    if (w_us > PM_DYN_W_HI || g_us > PM_DYN_G_HI) { d->heavy = 1; d->calm = 0; }
    else if (d->heavy && w_us < PM_DYN_W_LO && g_us < PM_DYN_G_LO) { if (++d->calm >= PM_DYN_CALM) { d->heavy = 0; d->calm = 0; } }
    else d->calm = 0;
    return d->heavy;
}
/* The key frame skip (LGF_ISKIP) of one drawn step with interpolation on. late: the step came more than 2 vblanks after the last one. */
static inline int pm_fps_iskip(int mode, int heavy, int late) { return mode == PM_FPS_ON ? 0 : (late || heavy); }
static inline int pm_fps_next(int mode) { return mode == PM_FPS_OFF ? PM_FPS_ON : mode == PM_FPS_ON ? PM_FPS_DYN : PM_FPS_OFF; }
static inline const char* pm_fps_name(int mode) { return mode == PM_FPS_ON ? "On" : mode == PM_FPS_DYN ? "Dynamic" : "Off"; }
```
## Block B: 3ds/src/pm_main.c
B1: after the line `#include "pm_port.h"` (line 12) add the line `#include "pm_fpsmode.h"`.
B2: in interp_heavy replace lines 510 to 516 (7 lines). Before:
```c
    static int heavy, calm;
    static unsigned n, nh, sl, sk, sp, sg;
    unsigned w = heavy ? pm_iw_us[2] : (pm_iw_us[0] > pm_iw_us[1] ? pm_iw_us[0] : pm_iw_us[1]);
    unsigned g = (unsigned)(C3D_GetDrawingTime() * 1000.f);
    if (w > 26000u || g > 13000u) { heavy = 1; calm = 0; } /* INTERPB3: one walk a step (the key frame is the K copy): the walk limit is 26 ms, calm under 20 ms; two GPU frames a step: the GPU limit stays 13 ms */
    else if (heavy && w < 20000u && g < 10000u) { if (++calm >= 30) heavy = 0; }
    else calm = 0;
```
After (5 lines):
```c
    static PmFpsDyn dyn; /* FSKIP: the Dynamic rule (pm_fpsmode.h) */
    static unsigned n, nh, sl, sk, sp, sg;
    unsigned w = dyn.heavy ? pm_iw_us[2] : (pm_iw_us[0] > pm_iw_us[1] ? pm_iw_us[0] : pm_iw_us[1]);
    unsigned g = (unsigned)(C3D_GetDrawingTime() * 1000.f);
    const int heavy = pm_fps_dyn_step(&dyn, w, g);
```
The rest of interp_heavy (the counters, the interp log line, `return heavy;`) stays.
B3, line 721. Before:
```c
                if (!pm_det_on() && (iv > PM_VBLANKS_PER_STEP || (pm_interp_on > 0 && (interp_heavy() || PMDL("iskipall"))))) lc.flags |= LGF_ISKIP; /* F06-5; DET: the key path is never taken on the vblank count. INTERP1: heavy walks skip the key frame */
```
After:
```c
                if (!pm_det_on() && (pm_interp_on > 0 ? pm_fps_iskip(pm_opt.fps60 == PM_FPS_ON ? PM_FPS_ON : PM_FPS_DYN, interp_heavy(), iv > PM_VBLANKS_PER_STEP) || PMDL("iskipall") : iv > PM_VBLANKS_PER_STEP)) lc.flags |= LGF_ISKIP; /* FSKIP: On never skips the key frame, Dynamic skips it under load or when late (pm_fpsmode.h); interp off as before. DET: never */
```
## Block C: 3ds/src/pm_options.c
C1: after the line `#include "pm_port.h"` add the line `#include "pm_fpsmode.h"`.
C2, line 208. Before: `            if (!fld || end == eq + 1 || *end || v < 0 || v > 1) {`
After: `            if (!fld || end == eq + 1 || *end || v < 0 || v > (fld == &pm_opt.fps60 ? 2 : 1)) { /* FSKIP: fps60 is 0, 1 or 2 */`
C3: after line 313 (`    if (row == 12) { pm_opt.poslog = !pm_opt.poslog; ...`) add:
```c
    if (row == 0) { /* FSKIP: Off, On, Dynamic. On and Dynamic both run the interpolation; the mode is read at each drawn step */
        extern int pm_interp_want;
        pm_opt.fps60 = pm_fps_next(pm_opt.fps60);
        pm_interp_want = pm_opt.fps60 != PM_FPS_OFF;
        changed = 1;
        pm_log("options: live row 0 value %d (%s)", pm_opt.fps60, pm_fps_name(pm_opt.fps60));
        return;
    }
```
C4, line 730. Before: `        case RW_FPS: return pm_options_get(0) ? "On" : "Off";`
After: `        case RW_FPS: return pm_fps_name(pm_opt.fps60);`
## Block D: 3ds/tools/pm_fpsmode_test.c (new file)
```c
#include <stdio.h>
#include <string.h>
#include "pm_fpsmode.h"
/* FSKIP host test: the 3 modes of the 60 FPS option and the Dynamic rule. O3DS values from the console log build3ds/hw_tidal1_live.txt,
 * interp line f11719: plain_us 22440 gpu_us 7364 (the old rule kept 300 of 300 steps heavy: calm needed a walk under 20000 us). */
static int feed(PmFpsDyn* d, unsigned w, unsigned g, int n) { int h = d->heavy; while (n-- > 0) h = pm_fps_dyn_step(d, w, g); return h; }
int main(void) {
    PmFpsDyn d;
    if (pm_fps_iskip(PM_FPS_ON, 1, 1) != 0) { printf("fskip FAIL iskip on\n"); return 1; }
    if (pm_fps_iskip(PM_FPS_DYN, 0, 0) != 0 || pm_fps_iskip(PM_FPS_DYN, 1, 0) != 1 || pm_fps_iskip(PM_FPS_DYN, 0, 1) != 1) { printf("fskip FAIL iskip dyn\n"); return 1; }
    if (pm_fps_next(PM_FPS_OFF) != PM_FPS_ON || pm_fps_next(PM_FPS_ON) != PM_FPS_DYN || pm_fps_next(PM_FPS_DYN) != PM_FPS_OFF) { printf("fskip FAIL next\n"); return 1; }
    if (strcmp(pm_fps_name(0), "Off") || strcmp(pm_fps_name(1), "On") || strcmp(pm_fps_name(2), "Dynamic")) { printf("fskip FAIL name\n"); return 1; }
    memset(&d, 0, sizeof d);
    if (feed(&d, 22440u, 7364u, 300) != 0) { printf("fskip FAIL calm start\n"); return 1; }
    d.heavy = 1; d.calm = 0;
    if (feed(&d, 22440u, 7364u, PM_DYN_CALM - 1) != 1) { printf("fskip FAIL calm early\n"); return 1; }
    if (feed(&d, 22440u, 7364u, 1) != 0) { printf("fskip FAIL calm o3ds\n"); return 1; }
    if (feed(&d, PM_DYN_W_HI + 1u, 0u, 1) != 1) { printf("fskip FAIL spike walk\n"); return 1; }
    memset(&d, 0, sizeof d);
    if (feed(&d, 0u, PM_DYN_G_HI + 1u, 1) != 1) { printf("fskip FAIL spike gpu\n"); return 1; }
    if (feed(&d, 28000u, 7364u, 100) != 1) { printf("fskip FAIL hold\n"); return 1; } /* between LO and HI the state stays */
    feed(&d, 22440u, 7364u, PM_DYN_CALM - 1);
    feed(&d, 28000u, 7364u, 1); /* one step that is not calm starts the count again */
    if (feed(&d, 22440u, 7364u, PM_DYN_CALM - 1) != 1) { printf("fskip FAIL hysteresis\n"); return 1; }
    if (feed(&d, 22440u, 7364u, 1) != 0) { printf("fskip FAIL calm end\n"); return 1; }
    printf("fskip ok 12/12\n");
    return 0;
}
```
## Block E: 3ds/tests/host.d/fskip.sh (new file)
```sh
#!/bin/sh
# FSKIP: host test of pm_fpsmode.h (60 FPS modes Off, On, Dynamic and the Dynamic rule). Each seeded fault must print its exact line.
mkdir -p /tmp/fskip_seed
seed() {
    sed "$1" 3ds/include/pm_fpsmode.h > /tmp/fskip_seed/pm_fpsmode.h
    gcc -m32 -Wall -I /tmp/fskip_seed -o /tmp/fskip_seed/t 3ds/tools/pm_fpsmode_test.c || exit 1
    got=$(/tmp/fskip_seed/t)
    if [ "$got" != "$2" ]; then echo "fskip seed FAIL: [$1] got [$got]"; exit 1; fi
}
seed 's/mode == PM_FPS_ON ? 0 : (late || heavy)/(late || heavy)/' 'fskip FAIL iskip on'
seed 's/#define PM_DYN_W_LO 27000u/#define PM_DYN_W_LO 20000u/' 'fskip FAIL calm o3ds'
seed 's/else d->calm = 0;/else (void)0;/' 'fskip FAIL hysteresis'
grep -q 'pm_fps_iskip(pm_opt.fps60 == PM_FPS_ON' 3ds/src/pm_main.c || { echo "fskip FAIL: pm_main.c does not call pm_fps_iskip"; exit 1; }
grep -q 'pm_fps_dyn_step(&dyn, w, g)' 3ds/src/pm_main.c || { echo "fskip FAIL: interp_heavy does not call pm_fps_dyn_step"; exit 1; }
grep -q 'case RW_FPS: return pm_fps_name(pm_opt.fps60);' 3ds/src/pm_options.c || { echo "fskip FAIL: the 60 FPS row does not show the mode"; exit 1; }
gcc -m32 -Wall -Wextra -I 3ds/include -o /tmp/fskip_t 3ds/tools/pm_fpsmode_test.c && /tmp/fskip_t || exit 1
```
