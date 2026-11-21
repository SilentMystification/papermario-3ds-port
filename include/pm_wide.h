#ifndef PM_WIDE_H
#define PM_WIDE_H
/* F07-3: UI anchors. With `wide` the 4:3 UI stays centered at 1:1 and the screen shows 40 px more on each side. */
extern int pm_wide;
extern int pm_wout; /* FSUI1: pm_gpu.c, Widescreen or Fullscreen output (3D 1.25 x, 2D 1:1 at 40 + x) */
static inline int pm_scr_left(void) { return pm_wout ? -40 : 0; } /* FSUI2: game x of the left screen edge */
static inline int pm_scr_right(void) { return pm_wout ? 360 : 320; } /* FSUI2: the right edge (exclusive) */
static inline int pm_wide_left(int x) { return pm_wide ? x - 40 : x; }  /* a left-edge anchored x */
static inline int pm_wide_right(int x) { return pm_wide ? x + 40 : x; } /* a right-edge anchored x */
static inline int pm_wide_top(int y) { /* UA2: a top-edge anchored y, clamped at row 0 (the view top moved up 20 rows) */
    if (!pm_wide) return y;
    return y - 20 < 0 ? 0 : y - 20;
}
static inline int pm_wide_bottom(int y) { return pm_wide ? y + 20 : y; } /* UA2: a bottom-edge anchored y (the view bottom moved down 20 rows) */
#endif
