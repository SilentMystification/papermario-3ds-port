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
    if (pm_fps_mode(PM_FPS_ON, 0) != PM_FPS_OFF || pm_fps_mode(PM_FPS_OFF, 1) != PM_FPS_DYN || pm_fps_mode(PM_FPS_ON, 1) != PM_FPS_ON || pm_fps_mode(PM_FPS_DYN, 1) != PM_FPS_DYN) { printf("fskip FAIL mode\n"); return 1; }
    if (pm_fps_fs1(PM_FPS_OFF) != 1 || pm_fps_fs1(PM_FPS_ON) != 0 || pm_fps_fs1(PM_FPS_DYN) != 1) { printf("fskip FAIL fs1\n"); return 1; }
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
    printf("fskip ok 14/14\n");
    return 0;
}
