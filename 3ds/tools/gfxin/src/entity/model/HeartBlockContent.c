#include "src/entity/model/HeartBlockContent.c"
Gfx Entity_HeartBlockContent_LoadPalette[] = {
    gsDPSetTextureLUT(G_TT_RGBA16),
    gsDPLoadTLUT_pal16(0, D_0A000600_E4A440),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsDPSetTextureConvert(G_TC_FILT),
    gsSPEndDisplayList(),
};
Gfx Entity_HeartBlockContent_RenderImage[] = {
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsSPDisplayList(Entity_HeartBlockContent_LoadPalette),
    gsSPClearGeometryMode(G_CULL_BACK | G_LIGHTING | G_SHADING_SMOOTH),
    gsSPVertex(Entity_HeartBlockContent_vtx, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};
Gfx Entity_HeartBlockContent_RenderHeartSleeping[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_AA_ZB_TEX_EDGE, G_RM_AA_ZB_TEX_EDGE2),
    gsDPLoadTextureBlock_4b(D_0A000000_E49E40, G_IM_FMT_CI, 32, 32, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPDisplayList(Entity_HeartBlockContent_RenderImage),
    gsSPEndDisplayList(),
};
Gfx D_0A000758_E4A598[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_AA_ZB_TEX_EDGE, G_RM_AA_ZB_TEX_EDGE2),
    gsDPLoadTextureBlock_4b(D_0A000200_E4A040, G_IM_FMT_CI, 32, 32, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPDisplayList(Entity_HeartBlockContent_RenderImage),
    gsSPEndDisplayList(),
};
Gfx Entity_HeartBlockContent_RenderHeartAwake[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_ZB_CLD_SURF, G_RM_ZB_CLD_SURF2),
    gsDPLoadTextureBlock_4b(D_0A000200_E4A040, G_IM_FMT_CI, 32, 32, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPDisplayList(Entity_HeartBlockContent_RenderImage),
    gsSPEndDisplayList(),
};
Gfx Entity_HeartBlockContent_RenderHeartHappy[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_ZB_CLD_SURF, G_RM_ZB_CLD_SURF2),
    gsDPLoadTextureBlock_4b(D_0A000400_E4A240, G_IM_FMT_CI, 32, 32, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPDisplayList(Entity_HeartBlockContent_RenderImage),
    gsSPEndDisplayList(),
};
