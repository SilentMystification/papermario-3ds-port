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
/* A 4-bit sprite drawn through two palettes at once. lut[sprite_index * 16 + shade_index] is the
 * RGBA8 (R high byte) the combiner gives. The shade index is read at (x - dx4/4, y - dy4/4),
 * clamped to the image. spal is only the cache key and the invalidation range. */
int pm_tex_load_shaded(const void* img, int width, int height, int stride, const void* spal,
                       const unsigned* lut, int dx4, int dy4);
int pm_tex_load_rgba8(const void* buf, unsigned key, int w, int h, const unsigned char* rgba);
int pm_tex_bind(void);
void pm_tex_size(int* w, int* h);
/* G_TX_WRAP / G_TX_MIRROR / G_TX_CLAMP for the render tile. mask 0 clamps. */
void pm_tex_set_wrap(unsigned cms, unsigned cmt, unsigned masks, unsigned maskt);

#endif
