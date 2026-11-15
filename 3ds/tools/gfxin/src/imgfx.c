#include "src/imgfx.c"
Gfx DefaultImgFXSetupGfx[] = {
    gsSPClearGeometryMode(G_CULL_BOTH | G_LIGHTING),
    gsSPSetGeometryMode(G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH),
    gsSPTexture(-1, -1, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_ALPHADITHER, 18,
                     G_AD_DISABLE | G_CD_DISABLE | G_TC_FILT | G_TF_BILERP | G_TP_PERSP),
    gsSPEndDisplayList(),
};
