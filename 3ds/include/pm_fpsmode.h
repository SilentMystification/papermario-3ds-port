/* FSKIP2 (user 2026-11-16): the Frame skip option (options.ini fps60, Options row "Frame skip", General tab row 0).
 *  0 Off: 30 FPS, no interpolation. FS1 (pm_main.c) drops the draws of a late step, so the game logic keeps 30 steps a second.
 *  1 On: every drawn game step shows the key frame and the interpolated frame. No draw is dropped: a slow step makes the game slower.
 *  2 Dynamic: 60 FPS while the walk and the GPU fit, 30 FPS (no interpolated frame) under load, with hysteresis. FS1 drops late draws.
 * The fps60 launch switch with fps60=0 acts as Dynamic. The fs1off switch turns FS1 off, fs1force=<n> forces it on. Integers only, no division. */
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
/* FSKIP2: the mode in force. interp_on is pm_interp_on (> 0: the interpolation runs). */
static inline int pm_fps_mode(int opt, int interp_on) { return interp_on <= 0 ? PM_FPS_OFF : opt == PM_FPS_ON ? PM_FPS_ON : PM_FPS_DYN; }
/* FSKIP2: FS1 (drop the draws of a late step) runs in Off and Dynamic, never in On. */
static inline int pm_fps_fs1(int mode) { return mode != PM_FPS_ON; }
static inline int pm_fps_next(int mode) { return mode == PM_FPS_OFF ? PM_FPS_ON : mode == PM_FPS_ON ? PM_FPS_DYN : PM_FPS_OFF; }
static inline const char* pm_fps_name(int mode) { return mode == PM_FPS_ON ? "On" : mode == PM_FPS_DYN ? "Dynamic" : "Off"; }
