/* nd_psochk.c: NPSO4, the field compare of two NdPsoDesc values. Pure C, no walker state (host-testable). */
#include <stdio.h>
#include "nd_psochk.h"

int nd_psochk_cmp(const NdPsoDesc* s, const NdPsoDesc* w, char* out, unsigned n) {
    int bad = 0;
    if (out && n) out[0] = 0;
#define PSOCHK_F(f, fmt) do { if ((uint32_t)s->f != (uint32_t)w->f) { \
        if (!bad && out && n) { snprintf(out, n, #f " stated " fmt " walker " fmt, (unsigned)s->f, (unsigned)w->f); } \
        bad++; } } while (0)
    /* field order of NdPsoDesc */
    PSOCHK_F(cw0, "%08x"); PSOCHK_F(cw1, "%08x"); PSOCHK_F(omh, "%08x"); PSOCHK_F(oml, "%08x"); PSOCHK_F(geom, "%08x");
    PSOCHK_F(key_c, "%08x"); PSOCHK_F(key_s, "%08x"); PSOCHK_F(conv_k4, "%08x");
    PSOCHK_F(tex_on, "%02x"); PSOCHK_F(use_t1, "%02x"); PSOCHK_F(t1_valid, "%02x"); PSOCHK_F(dual_on, "%02x");
    PSOCHK_F(box_prod, "%02x"); PSOCHK_F(box_folded, "%02x"); PSOCHK_F(comb_set, "%02x"); PSOCHK_F(comb_alpha_shade, "%02x");
    PSOCHK_F(leg_prev, "%02x"); PSOCHK_F(vprim, "%02x"); PSOCHK_F(zimg, "%02x"); PSOCHK_F(plod, "%02x");
#undef PSOCHK_F
    return bad;
}
