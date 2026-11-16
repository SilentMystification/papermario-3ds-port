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
