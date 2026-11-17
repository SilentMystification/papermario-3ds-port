#include "src/screen_overlays.c"
Gfx Gfx_LoadStencilTex_CommonParams[] = {
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureFilter(G_TF_AVERAGE),
    gsDPSetTextureConvert(G_TC_FILT),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetColorDither(G_CD_MAGICSQ),
    gsDPSetAlphaDither(G_AD_PATTERN),
    gsDPSetCombineMode(PM_CC_3A, PM_CC_3A),
    gsDPSetRenderMode(G_RM_CLD_SURF, G_RM_CLD_SURF2),
    gsSPClearGeometryMode(G_ZBUFFER | G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH | G_CLIPPING | 0x0040F9FA),
    gsSPSetGeometryMode(G_SHADE | G_SHADING_SMOOTH),
    gsSPEndDisplayList()
};
Gfx Gfx_LoadStencilTex_SharpCircle[] = {
    gsSPDisplayList(Gfx_LoadStencilTex_CommonParams),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPLoadTextureTile_4b(ui_stencil_sharp_circle_png, G_IM_FMT_I, 32, 0, 0, 0, 31, 31, 0, G_TX_MIRROR | G_TX_CLAMP, G_TX_MIRROR | G_TX_CLAMP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 0x00FC, 0x00FC),
    gsSPEndDisplayList()
};
Gfx Gfx_LoadStencilTex_Mario[] = {
    gsSPDisplayList(Gfx_LoadStencilTex_CommonParams),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPLoadTextureTile_4b(ui_stencil_mario_png, G_IM_FMT_I, 64, 0, 0, 0, 63, 63, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 6, 6, G_TX_NOLOD, G_TX_NOLOD),
    gsSPEndDisplayList()
};
Gfx Gfx_LoadStencilTex_Star[] = {
    gsSPDisplayList(Gfx_LoadStencilTex_CommonParams),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPLoadTextureTile_4b(ui_stencil_star_png, G_IM_FMT_I, 32, 0, 0, 0, 31, 63, 0, G_TX_MIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 5, 6, G_TX_NOLOD, G_TX_NOLOD),
    gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 0x00FC, 0x00FC),
    gsSPEndDisplayList(),
};
Gfx Gfx_LoadStencilTex_BlurryCircle[] = {
    gsSPDisplayList(Gfx_LoadStencilTex_CommonParams),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetTextureImage(G_IM_FMT_I, G_IM_SIZ_8b, 64, ui_stencil_blurry_circle_png),
    gsDPSetTile(G_IM_FMT_I, G_IM_SIZ_8b, 8, 0x0000, G_TX_LOADTILE, 0, G_TX_MIRROR | G_TX_CLAMP, 6, G_TX_NOLOD, G_TX_MIRROR | G_TX_CLAMP, 6, G_TX_NOLOD),
    gsDPLoadSync(),
    gsDPLoadTile(G_TX_LOADTILE, 0, 0, 0x00FC, 0x00FC),
    gsDPPipeSync(),
    gsDPSetTile(G_IM_FMT_I, G_IM_SIZ_8b, 8, 0x0000, G_TX_RENDERTILE, 0, G_TX_MIRROR | G_TX_CLAMP, 6, G_TX_NOLOD, G_TX_MIRROR | G_TX_CLAMP, 6, G_TX_NOLOD),
    gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 0x00FC, 0x00FC),
    gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 0x01FC, 0x01FC),
    gsSPEndDisplayList()
};
Gfx D_8014E8F0[] = {
    gsSPTexture(0x8000, 0x8000, 0, G_TX_RENDERTILE, G_OFF),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetDepthSource(G_ZS_PRIM),
    gsDPSetPrimDepth(0, 0),
    gsDPSetRenderMode(Z_UPD | IM_RD | CVG_DST_SAVE | ZMODE_OPA | FORCE_BL | GBL_c1(G_BL_CLR_MEM, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA), Z_UPD | IM_RD | CVG_DST_SAVE | ZMODE_OPA | FORCE_BL | GBL_c2(G_BL_CLR_MEM, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)),
    gsDPSetCombineMode(PM_CC_SCREEN_OVERLAY, PM_CC_SCREEN_OVERLAY),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsDPSetTextureConvert(G_TC_FILT),
    gsSPClearGeometryMode(G_CULL_BOTH | G_LIGHTING | G_SHADING_SMOOTH),
    gsSPSetGeometryMode(G_ZBUFFER | G_SHADE | G_CULL_BACK),
    gsSPVertex(&vtx_stencil1, 10, 0),
    gsSP2Triangles(0, 1, 2, 0, 3, 0, 2, 0),
    gsSP2Triangles(0, 4, 1, 0, 3, 5, 0, 0),
    gsSP2Triangles(1, 6, 2, 0, 7, 3, 2, 0),
    gsSP2Triangles(8, 7, 2, 0, 7, 9, 3, 0),
    gsDPPipeSync(),
    gsDPSetDepthSource(G_ZS_PIXEL),
    gsSPEndDisplayList()
};
Gfx D_8014E9A8[] = {
    gsDPPipeSync(),
    gsDPSetDepthSource(G_ZS_PRIM),
    gsDPSetPrimDepth(20, 0),
    gsDPSetRenderMode(G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF2),
    gsDPSetCombineMode(PM_CC_SCREEN_OVERLAY, PM_CC_SCREEN_OVERLAY),
    gsSPClearGeometryMode(G_CULL_BOTH | G_LIGHTING | G_SHADING_SMOOTH),
    gsSPSetGeometryMode(G_ZBUFFER | G_SHADE),
    gsDPSetColorDither(G_CD_MAGICSQ),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureConvert(G_TC_FILT),
    gsDPFillRectangle(0, 0, 320, 240),
    gsDPSetColorDither(G_CD_DISABLE),
    gsDPPipeSync(),
    gsDPSetDepthSource(G_ZS_PIXEL),
    gsSPEndDisplayList()
};
