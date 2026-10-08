#ifndef PM_TEX_H
#define PM_TEX_H

void pm_tex_init(void);

/* fmt/siz are the G_IM_FMT_* / G_IM_SIZ_* values. Returns 1 if the texture is resident. */
int pm_tex_load(const void* img, unsigned fmt, unsigned siz, int width, int height,
                const void* tlut, int tlut_n);
int pm_tex_bind(void);
void pm_tex_size(int* w, int* h);

#endif
