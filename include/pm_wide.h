#ifndef PM_WIDE_H
#define PM_WIDE_H
/* F07-3: UI anchors. With `wide` the 4:3 UI stays centered at 1:1 and the screen shows 40 px more on each side. */
extern int pm_wide;
static inline int pm_wide_left(int x) { return pm_wide ? x - 40 : x; }  /* a left-edge anchored x */
static inline int pm_wide_right(int x) { return pm_wide ? x + 40 : x; } /* a right-edge anchored x */
static inline int pm_wide_vx(int x) { return pm_wide ? x + (x - 160) / 5 : x; } /* UA4: a point in the zoomed battle view (6 / 5 about 160, 120) */
static inline int pm_wide_vy(int y) { return pm_wide ? y + (y - 120) / 5 : y; }
#endif
