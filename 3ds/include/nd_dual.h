#ifndef ND_DUAL_H
#define ND_DUAL_H
static inline int nd_dual_keep(unsigned comb_a, unsigned comb_c) { return comb_a == 1u && comb_c == 2u; } /* COIN: dual_on (nd_comb.c) stays on only while cycle 1 reads TEXEL0 in a and TEXEL1 in c, the tex_shaded0 test (nd_tex_set.c). Fields after the op_setcombine rename. 1 = G_CCMUX_TEXEL0, 2 = G_CCMUX_TEXEL1. */
#endif
