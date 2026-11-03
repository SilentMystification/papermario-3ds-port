#ifndef PM_GBI_H
#define PM_GBI_H

void pm_gbi_init(void);
void pm_gbi_run(void* dl, unsigned nbytes);
void pm_gbi_fallback(void);
/* Once a frame. The log gets one summed line per second, and only when it changes. */
void pm_gbi_end_frame(void);
void pm_gbi_eye_changed(void); /* STEREO: the eye shift changed, the combined matrix changes */
const char* pm_gbi_stats(void);

#endif
