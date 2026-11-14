#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/PinkFlower_1.vtx.inc.c"
#include "entity/model/PinkFlower_2.vtx.inc.c"
#include "entity/model/PinkFlower_3.vtx.inc.c"
#include "entity/model/PinkFlower_4.vtx.inc.c"
#include "entity/model/PinkFlower_5.vtx.inc.c"

INCLUDE_IMG("entity/model/PinkFlower_light.png", D_0A0005B0_E9BAB0);
Gfx D_0A0007B0_E9BCB0[] = {
    gsDPSetTextureLUT(G_TT_NONE),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPLoadTextureBlock(D_0A0005B0_E9BAB0, G_IM_FMT_IA, G_IM_SIZ_16b, 16, 16, 0, G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, 4, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsSPEndDisplayList(),
};

INCLUDE_IMG("entity/model/PinkFlower_tex.png", D_0A000820_E9BD20);
NDN_DECL(D_0A001020_E9C520);

Mtx D_0A001098_E9C598 = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, -10.000000,
    0.000000, 1.000000, 0.000000,  25.000000,
    0.000000, 0.000000, 1.000000,  13.000000,
    0.000000, 0.000000, 0.000000,   1.000000
);

NDN_DECL(Entity_PinkFlower_RenderNode3);

NDN_DECL(Entity_PinkFlower_RenderNode2);

NDN_DECL(Entity_PinkFlower_RenderNode1);

NDN_DECL(Entity_PinkFlower_RenderNode5);

Gfx D_0A0013B8_E9C8B8[] = {
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2),
    gsSPDisplayList(D_0A0007B0_E9BCB0),
    gsSPClearGeometryMode(G_CULL_BACK | G_LIGHTING),
    gsSPSetGeometryMode(G_SHADING_SMOOTH),
    gsSPVertex(D_0A000000_E9B500, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};
