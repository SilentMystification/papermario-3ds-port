#include "src/world/area_mgm/mgm_00/scoreboard.c"
Gfx N(Gfx_RecordDisplay_Init)[] = {
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    gsDPSetCombineMode(G_CC_PRIMITIVE, G_CC_PRIMITIVE),
    gsDPSetColorDither(G_CD_DISABLE),
    gsDPSetAlphaDither(G_AD_DISABLE),
    gsDPSetCombineKey(G_CK_NONE),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsDPNoOp(),
    gsDPSetPrimColor(0, 0, 255, 0, 0, 0),
    gsDPFillRectangle(44, 49, 276, 51),
    gsDPFillRectangle(44, 49, 46, 133),
    gsDPFillRectangle(275, 49, 276, 133),
    gsDPFillRectangle(44, 132, 276, 133),
    gsSPEndDisplayList(),
};
