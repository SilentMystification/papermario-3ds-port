#ifndef PM_TEX_H
#define PM_TEX_H

void pm_tex_init(void);
/* Call after the previous frame's GPU work has finished. */
void pm_tex_begin_frame(void);
/* Drop cached copies of any image or palette inside this range. The game
 * rewrites storybook pages in place; the pointer alone is not a new texture. */
void pm_tex_invalidate(const void* ptr, unsigned size);

/* fmt/siz are the G_IM_FMT_* / G_IM_SIZ_* values. Returns 1 if the texture is resident. */
int pm_tex_load(const void* img, unsigned fmt, unsigned siz, int width, int height,
                const void* tlut, int tlut_n, int stride);
int pm_tex_bind(void);
void pm_tex_size(int* w, int* h);

#endif
