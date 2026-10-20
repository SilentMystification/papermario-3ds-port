#ifndef PM_TEX_H
#define PM_TEX_H

/* V05: size of an off-screen image (a texture the GPU renders into) */
#define PM_OFF_W 512
#define PM_OFF_H 256
void pm_tex_init(void);
unsigned pm_tex_epoch(void); /* PG1 */
/* V05: image i holds the colour image at base (width stride pixels). A load inside it binds the render texture. */
void pm_tex_set_offscreen(int i, const void* base, unsigned stride, void* c3d_tex, int siz, int w, int h);
void pm_tex_clear_offscreen(void);
int pm_tex_i8(int aux);       /* FX2a: the bound tile 0 (or tile 1 with aux) texture is an 8 bit slot */
int pm_tex_off_idx(void);     /* FX2a: slot index of the bound tile 0 texture, -1 if it is not a slot */
int pm_tex_aux_off_idx(void); /* same for tile 1 */
/* V05: texel offset of the last loaded image inside its off-screen texture (0, 0 for an ordinary texture) */
void pm_tex_origin(int* x, int* y);
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
void pm_tex_bind_reset(void); /* DR1: forget the texture bind shadow */
int pm_tex_failed(void);
unsigned pm_tex_fail_count(void);
void pm_tex_size(int* w, int* h);
/* G_TX_WRAP / G_TX_MIRROR / G_TX_CLAMP for the render tile. mask 0 clamps. */
void pm_tex_set_wrap(unsigned cms, unsigned cmt, unsigned masks, unsigned maskt);

/* S25: TEXEL1. Load a texture with pm_tex_load, then call set_aux to keep it at unit 1; load tile 0 after it. */
int pm_tex_set_aux(unsigned cms, unsigned cmt, unsigned masks, unsigned maskt);
int pm_tex_bind_aux(void);
int pm_tex_aux_valid(void);
void pm_tex_aux_origin(int* x, int* y); /* V12c: window start inside an off-screen TEXEL1 slot */
void pm_tex_unbind_aux(void);
void pm_tex_aux_size(int* w, int* h);

#endif
