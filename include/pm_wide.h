#ifndef PM_WIDE_H
#define PM_WIDE_H
/* F07-3: UI anchors. With `wide` the 4:3 UI stays centered at 1:1 and the screen shows 40 px more on each side. */
extern int pm_wide;
static inline int pm_wide_left(int x) { return pm_wide ? x - 40 : x; }  /* a left-edge anchored x */
static inline int pm_wide_right(int x) { return pm_wide ? x + 40 : x; } /* a right-edge anchored x */
#endif
