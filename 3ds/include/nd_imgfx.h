/* PIPEFX: the imgfx anim list (raw F3DEX2 words from the ROM, imgfx.c imgfx_load_anim) to typed units in place. A vertex load points
 * into vb (ImgFXVtx is 12 bytes, Vtx 16). Stops after the end unit or after n units. Returns the units done. */
#ifndef ND_IMGFX_H
#define ND_IMGFX_H
#include <stdint.h>
#include "nd_op.h"
#include "nd_fs.h"
static inline unsigned nd_imgfx_type(uint32_t* g, unsigned n, uint32_t koff, uintptr_t vb) {
    unsigned j = 0;
    while (j < n) {
        const uint32_t w0 = g[2u * j], w1 = g[2u * j + 1u];
        const uint32_t t0 = ND_X0(w0, w1);
        uint32_t t1 = ND_X1(w0, w1);
        if ((t0 >> 24) == ND_FS_VTX) t1 = (uint32_t)((((int32_t)t1 - (int32_t)koff) / 3) * 4 + (int32_t)vb);
        g[2u * j] = t0; g[2u * j + 1u] = t1;
        j++;
        if ((t0 >> 24) == ND_FS_END) break;
    }
    return j;
}
#endif
