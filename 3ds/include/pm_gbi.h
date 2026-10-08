#ifndef PM_GBI_H
#define PM_GBI_H

void pm_gbi_init(void);
void pm_gbi_run(void* dl, unsigned nbytes);
void pm_gbi_fallback(void);

#endif
