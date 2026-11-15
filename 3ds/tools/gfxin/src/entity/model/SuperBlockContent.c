#include "src/entity/model/SuperBlockContent.c"
Gfx Entity_SuperBlockContent_Prepare[] = {
    gsDPSetTextureLUT(G_TT_RGBA16),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsSPEndDisplayList(),
};
Gfx Entity_SuperBlockContent_RenderImage[] = {
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsSPDisplayList(Entity_SuperBlockContent_Prepare),
    gsSPClearGeometryMode(G_CULL_BACK | G_LIGHTING | G_SHADING_SMOOTH),
    gsSPVertex(Entity_SuperBlockContent_vtx, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};
Gfx Entity_SuperBlockContent_Render[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_ZB_CLD_SURF, G_RM_ZB_CLD_SURF2),
    gsDPLoadTextureBlock_4b(D_0A000000_E4A6A0, G_IM_FMT_CI, 32, 32, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPDisplayList(Entity_SuperBlockContent_RenderImage),
    gsSPEndDisplayList(),
};
Gfx Entity_SuperBlockContent_Render2[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_ZB_CLD_SURF, G_RM_ZB_CLD_SURF2),
    gsDPLoadTextureBlock_4b(D_0A000000_E4A6A0, G_IM_FMT_CI, 32, 32, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPDisplayList(Entity_SuperBlockContent_RenderImage),
    gsSPEndDisplayList(),
};
