/* IMGFX: the build-time units of the 20 anim lists by anim type (nd_imgfx_tab.c, gen_imgfx_tab.py); a vertex load b is 16 * the first vertex,
 * the copy in imgfx.c adds vtxBufs[i]. */
#ifndef ND_IMGFX_H
#define ND_IMGFX_H
#include <stdint.h>
#define ND_IMGFX_N 20u
extern const uint32_t* const nd_imgfx_tab[ND_IMGFX_N];
extern const uint16_t nd_imgfx_n[ND_IMGFX_N];
#endif
