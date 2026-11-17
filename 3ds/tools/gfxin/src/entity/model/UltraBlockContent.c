#include "src/entity/model/UltraBlockContent.c"
Gfx Entity_UltraBlockContent_LoadPalette[] = {
    gsDPSetTextureLUT(G_TT_RGBA16),
    gsDPLoadTLUT_pal16(0, D_0A000600_E4B080),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsSPEndDisplayList(),
};
Gfx Entity_UltraBlockContent_RenderImage[] = {
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsSPDisplayList(Entity_UltraBlockContent_LoadPalette),
    gsSPClearGeometryMode(G_CULL_BACK | G_LIGHTING | G_SHADING_SMOOTH),
    gsSPVertex(Entity_UltraBlockContent_vtx, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};
Gfx Entity_UltraBlockContent_Render4[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_AA_ZB_TEX_EDGE, G_RM_AA_ZB_TEX_EDGE2),
    gsDPLoadTextureBlock_4b(D_0A000400_E4AE80, G_IM_FMT_CI, 32, 32, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPDisplayList(Entity_UltraBlockContent_RenderImage),
    gsSPEndDisplayList(),
};
Gfx Entity_UltraBlockContent_Render2[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2),
    gsDPLoadTextureBlock_4b(D_0A000400_E4AE80, G_IM_FMT_CI, 32, 32, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPDisplayList(Entity_UltraBlockContent_RenderImage),
    gsSPEndDisplayList(),
};
Gfx Entity_UltraBlockContent_Render3[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_AA_ZB_TEX_EDGE, G_RM_AA_ZB_TEX_EDGE2),
    gsDPLoadTextureBlock_4b(D_0A000200_E4AC80, G_IM_FMT_CI, 32, 32, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPDisplayList(Entity_UltraBlockContent_RenderImage),
    gsSPEndDisplayList(),
};
Gfx Entity_UltraBlockContent_Render[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2),
    gsDPLoadTextureBlock_4b(D_0A000200_E4AC80, G_IM_FMT_CI, 32, 32, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPDisplayList(Entity_UltraBlockContent_RenderImage),
    gsSPEndDisplayList(),
};
