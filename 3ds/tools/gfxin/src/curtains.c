#include "src/curtains.c"
Gfx D_800746E0[] = {
    gsDPSetTextureLUT(G_TT_NONE),
    gsSPTexture(-1, -1, 0, G_TX_RENDERTILE, G_ON),
    gsDPLoadTextureBlock_4b(theater_walls_png, G_IM_FMT_I, 16, 32, 0, G_TX_NOMIRROR | G_TX_WRAP,
                            G_TX_NOMIRROR | G_TX_CLAMP, 4, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPEndDisplayList(),
};
Gfx D_80075730[] = {
    gsDPSetTextureLUT(G_TT_NONE),
    gsSPTexture(-1, -1, 0, G_TX_RENDERTILE, G_ON),
    gsDPLoadTextureBlock(theater_curtains_png, G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0, G_TX_MIRROR | G_TX_WRAP,
                         G_TX_NOMIRROR | G_TX_CLAMP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPEndDisplayList(),
};
Gfx D_800758A0[] = {
    gsDPSetTextureLUT(G_TT_RGBA16),
    gsDPLoadTLUT_pal16(0, theater_floor_pal),
    gsSPTexture(-1, -1, 0, G_TX_RENDERTILE, G_ON),
    gsDPLoadTextureBlock_4b(theater_floor_png, G_IM_FMT_CI, 32, 16, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR |
                            G_TX_WRAP, 5, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSPEndDisplayList(),
};
Gfx TheaterWallShadowsGfx[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_PASS, G_RM_CLD_SURF2),
    gsDPSetCombineMode(G_CC_MODULATEIA, PM_CC2_MULTIPLY_PRIM),
    gsSPDisplayList(D_80075730),
    gsSPVertex(theater_wall_shadows_vtx, 8, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 6, 7, 0),
    gsSPEndDisplayList(),
};
Gfx TheaterCurtainGfx[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_PASS, G_RM_XLU_SURF2),
    gsDPSetCombineMode(G_CC_MODULATEIDECALA, PM_CC2_MULTIPLY_PRIM),
    gsSPDisplayList(D_80075730),
    gsSPVertex(theater_curtain_vtx, 31, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 6, 7, 0),
    gsSP2Triangles(8, 9, 10, 0, 8, 10, 11, 0),
    gsSP2Triangles(12, 13, 14, 0, 12, 14, 15, 0),
    gsSP2Triangles(15, 16, 17, 0, 15, 17, 18, 0),
    gsSP2Triangles(19, 20, 21, 0, 19, 21, 22, 0),
    gsSP2Triangles(13, 23, 24, 0, 13, 24, 14, 0),
    gsSP2Triangles(23, 25, 26, 0, 23, 26, 24, 0),
    gsSP2Triangles(25, 27, 28, 0, 25, 28, 26, 0),
    gsSP2Triangles(27, 29, 30, 0, 27, 30, 28, 0),
    gsSPEndDisplayList(),
};
Gfx TheaterInsetShadowsGfx[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(G_RM_PASS, G_RM_XLU_SURF2),
    gsDPSetCombineMode(G_CC_SHADE, PM_CC2_MULTIPLY_PRIM),
    gsSPTexture(0x0080, 0x0080, 0, G_TX_RENDERTILE, G_OFF),
    gsSPVertex(theater_left_inset_shadow_vtx, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPVertex(theater_right_inset_shadow_vtx, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};
Gfx TheaterFloorGfx[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(CVG_DST_FULL | ZMODE_OPA | FORCE_BL | G_RM_PASS, CVG_DST_FULL | ZMODE_OPA | FORCE_BL |
                      GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)),
    gsDPSetCombineMode(G_CC_MODULATEIA, PM_CC2_MULTIPLY_PRIM),
    gsSPDisplayList(D_800746E0),
    gsSPVertex(theater_floor_vtx, 25, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(4, 5, 0, 0, 4, 0, 3, 0),
    gsSP2Triangles(1, 6, 7, 0, 1, 7, 2, 0),
    gsSP2Triangles(3, 8, 9, 0, 3, 9, 10, 0),
    gsSP2Triangles(11, 4, 3, 0, 11, 3, 10, 0),
    gsSP2Triangles(12, 13, 14, 0, 12, 14, 15, 0),
    gsSP2Triangles(8, 12, 15, 0, 8, 15, 9, 0),
    gsSP2Triangles(6, 16, 13, 0, 6, 13, 7, 0),
    gsSP2Triangles(14, 17, 18, 0, 19, 14, 18, 0),
    gsSP2Triangles(20, 11, 21, 0, 11, 22, 21, 0),
    gsSP2Triangles(23, 19, 18, 0, 23, 18, 24, 0),
    gsSP2Triangles(22, 23, 24, 0, 22, 24, 21, 0),
    gsSPEndDisplayList(),
};
Gfx TheaterRightWallGfx[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(CVG_DST_FULL | ZMODE_OPA | FORCE_BL | G_RM_PASS, CVG_DST_FULL | ZMODE_OPA | FORCE_BL |
                      GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)),
    gsDPSetCombineMode(G_CC_MODULATEIA, PM_CC2_MULTIPLY_PRIM),
    gsSPDisplayList(D_800758A0),
    gsSPVertex(theater_right_wall_vtx, 6, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(1, 4, 5, 0, 1, 5, 2, 0),
    gsSPEndDisplayList(),
};
Gfx TheaterLeftWallGfx[] = {
    gsDPPipeSync(),
    gsDPSetRenderMode(CVG_DST_FULL | ZMODE_OPA | FORCE_BL | G_RM_PASS, CVG_DST_FULL | ZMODE_OPA | FORCE_BL |
                      GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)),
    gsDPSetCombineMode(G_CC_MODULATEIA, PM_CC2_MULTIPLY_PRIM),
    gsSPDisplayList(D_800758A0),
    gsSPVertex(theater_left_wall_vtx, 6, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(1, 4, 5, 0, 1, 5, 2, 0),
    gsSPEndDisplayList(),
};
Gfx TheaterGfx[] = {
    gsSPDisplayList(TheaterFloorGfx),
    gsSPDisplayList(TheaterLeftWallGfx),
    gsSPDisplayList(TheaterRightWallGfx),
    gsSPDisplayList(TheaterInsetShadowsGfx),
    gsSPDisplayList(TheaterWallShadowsGfx),
    gsSPDisplayList(TheaterCurtainGfx),
    gsSPEndDisplayList(),
};
Gfx NoControllerSetupTexGfx[] = {
    gsDPPipeSync(),
    gsSPTexture(-1, -1, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureFilter(G_TF_POINT),
    gsDPSetTextureConvert(G_TC_FILT),
    gsDPSetCombineMode(G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPLoadTextureTile(ui_no_controller_png, G_IM_FMT_IA, G_IM_SIZ_8b, ui_no_controller_png_width,
                        ui_no_controller_png_height, 0, 0, ui_no_controller_png_width - 1,
                        ui_no_controller_png_height - 1, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 7,
                        5, G_TX_NOLOD, G_TX_NOLOD),
    gsSPClearGeometryMode(G_CULL_BOTH | G_LIGHTING),
    gsSPSetGeometryMode(G_SHADE | G_SHADING_SMOOTH),
    gsSPEndDisplayList(),
};
