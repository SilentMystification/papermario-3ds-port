#ifndef PM_GBI_H
#define PM_GBI_H

void pm_gbi_init(void);
void pm_gbi_run(void* dl, unsigned nbytes);
void pm_gbi_fallback(void);
/* Once a frame. With the fast/stats switch, logs i/c/d and returns a HUD line. */
void pm_gbi_end_frame(void);
const char* pm_gbi_stats(void);

#endif
