/* VCOUNT: mdl_get_vertex_count (model.c): at most n units (a moved list: NdNDev nu, no end unit), else up to NDO_END; N64 min and max. */
#ifndef ND_VCOUNT_H
#define ND_VCOUNT_H
#include "nd_op.h"
static inline unsigned nd_vcount(const uint32_t* u, unsigned n, uint32_t* lo, uint32_t* hi) {
    uint32_t mn = 0, mx = 0; unsigned k = 0;
    while (k < n) {
        const uint32_t w0 = u[2u * k], w1 = u[2u * k + 1u]; k++;
        if ((w0 >> 24) == NDO_VTX) { const uint32_t e = w1 + ((w0 >> 12) & 0xffu) * 16u; if (mn == 0) { mn = w1; mx = e; } if (mx < e) mx = e; if (mn > e) mn = e; }
        if ((w0 >> 24) == NDO_END) break;
    }
    *lo = mn; *hi = mx; return k;
}
#endif
