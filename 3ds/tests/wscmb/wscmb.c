/* WSCMB: ND_CMB_W and ND_CMB_NOP (3ds/include/nd_gbi.h) give the words of gsDPSetCombineMode and gsDPNoOp for every pair the window styles use. -DWSCMB_SEED changes one word (the seeded fault). */
#include "common.h"
#include <stdio.h>
static const Gfx g[] = {
    gsDPNoOp(),
    gsDPSetCombineMode(PM_CC_14, G_CC_PASS2),
    gsDPSetCombineMode(PM_CC_15, G_CC_PASS2),
    gsDPSetCombineMode(PM_CC_2B, G_CC_PASS2),
    gsDPSetCombineMode(PM_CC_2C, G_CC_PASS2),
    gsDPSetCombineMode(PM_CC_30, PM_CC_BOX2_CYC2),
    gsDPSetCombineMode(PM_CC_31, PM_CC_BOX2_CYC2),
    gsDPSetCombineMode(PM_CC_BOX1_OPAQUE, PM_CC_BOX1_CYC2),
    gsDPSetCombineMode(PM_CC_BOX1_TRANSPARENT, PM_CC_BOX1_CYC2),
    gsDPSetCombineMode(PM_CC_BOX2_OPAQUE, PM_CC_BOX2_CYC2),
    gsDPSetCombineMode(PM_CC_BOX2_TRANSPARENT, PM_CC_BOX2_CYC2),
    gsDPSetCombineMode(PM_CC_CONST_0, PM_CC_CONST_0),
    gsDPSetCombineMode(PM_CC_WINDOW_2, PM_CC_WINDOW_1),
    gsDPSetCombineMode(PM_CC_WINDOW_3, PM_CC_WINDOW_1),
    gsDPSetCombineMode(PM_CC_WINDOW_4, G_CC_PASS2),
    gsDPSetCombineMode(PM_CC_WINDOW_5, G_CC_PASS2),
    gsDPSetCombineMode(PM_CC_WINDOW_6, G_CC_PASS2),
    gsDPSetCombineMode(PM_CC_WINDOW_7, G_CC_PASS2),
};
static NdCmbW n[] = {
    ND_CMB_NOP,
    ND_CMB_W(PM_CC_14, G_CC_PASS2),
    ND_CMB_W(PM_CC_15, G_CC_PASS2),
    ND_CMB_W(PM_CC_2B, G_CC_PASS2),
    ND_CMB_W(PM_CC_2C, G_CC_PASS2),
    ND_CMB_W(PM_CC_30, PM_CC_BOX2_CYC2),
    ND_CMB_W(PM_CC_31, PM_CC_BOX2_CYC2),
    ND_CMB_W(PM_CC_BOX1_OPAQUE, PM_CC_BOX1_CYC2),
    ND_CMB_W(PM_CC_BOX1_TRANSPARENT, PM_CC_BOX1_CYC2),
    ND_CMB_W(PM_CC_BOX2_OPAQUE, PM_CC_BOX2_CYC2),
    ND_CMB_W(PM_CC_BOX2_TRANSPARENT, PM_CC_BOX2_CYC2),
    ND_CMB_W(PM_CC_CONST_0, PM_CC_CONST_0),
    ND_CMB_W(PM_CC_WINDOW_2, PM_CC_WINDOW_1),
    ND_CMB_W(PM_CC_WINDOW_3, PM_CC_WINDOW_1),
    ND_CMB_W(PM_CC_WINDOW_4, G_CC_PASS2),
    ND_CMB_W(PM_CC_WINDOW_5, G_CC_PASS2),
    ND_CMB_W(PM_CC_WINDOW_6, G_CC_PASS2),
    ND_CMB_W(PM_CC_WINDOW_7, G_CC_PASS2),
};
int main(void) {
    unsigned bad = 0, i;
#ifdef WSCMB_SEED
    n[1].w1 ^= 1u;
#endif
    for (i = 0; i < sizeof g / sizeof g[0]; i++)
        if (g[i].words.w0 != n[i].w0 || g[i].words.w1 != n[i].w1) { printf("wscmb differ %u: %08x %08x vs %08x %08x\n", i, (unsigned)g[i].words.w0, (unsigned)g[i].words.w1, (unsigned)n[i].w0, (unsigned)n[i].w1); bad++; }
    printf("wscmb %u pairs bad %u\n", i, bad);
    return bad != 0;
}
