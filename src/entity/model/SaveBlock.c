#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"
#include "include_asset.h"

#include "entity/model/SaveBlock_1.vtx.inc.c"
#include "entity/model/SaveBlock_2.vtx.inc.c"
#include "entity/model/SaveBlock_3.vtx.inc.c"

INCLUDE_IMG("entity/model/SaveBlock_star.png", D_0A000900_E4BBE0);
NDN_DECL(Entity_SaveBlock_LoadTextureStar);

INCLUDE_IMG("entity/model/SaveBlock_face.png", D_0A001170_E4C450);
NDN_DECL(Entity_SaveBlock_LoadTextureFace);

INCLUDE_IMG("entity/model/SaveBlock_letterS.png", D_0A0021E8_E4D4C8);

NDN_DECL(Entity_SaveBlock_LoadTextureLetterS);


Mtx Entity_SaveBlock_Mtx = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 1.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

NDN_DECL(Entity_SaveBlock_RenderLetterS);


Gfx Entity_SaveBlock_RenderFaces[] = {
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPTexture(0x0080, 0x0080, 0, G_TX_RENDERTILE, G_OFF),
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
    gsSPVertex(D_0A000370_E4B650, 30, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(2, 4, 5, 0, 2, 5, 3, 0),
    gsSP2Triangles(1, 6, 7, 0, 1, 7, 2, 0),
    gsSP2Triangles(7, 8, 4, 0, 7, 4, 2, 0),
    gsSP2Triangles(9, 10, 11, 0, 9, 11, 12, 0),
    gsSP2Triangles(11, 13, 14, 0, 11, 14, 12, 0),
    gsSP2Triangles(15, 16, 12, 0, 15, 12, 14, 0),
    gsSP2Triangles(16, 17, 9, 0, 16, 9, 12, 0),
    gsSP2Triangles(18, 16, 19, 0, 18, 19, 20, 0),
    gsSP2Triangles(16, 21, 22, 0, 16, 22, 19, 0),
    gsSP2Triangles(22, 23, 24, 0, 22, 24, 19, 0),
    gsSP2Triangles(24, 25, 20, 0, 24, 20, 19, 0),
    gsSP2Triangles(26, 27, 28, 0, 26, 28, 29, 0),
    gsSPVertex(&D_0A000370_E4B650[27], 26, 0),
    gsSP2Triangles(0, 3, 4, 0, 0, 4, 1, 0),
    gsSP2Triangles(4, 14, 5, 0, 4, 5, 1, 0),
    gsSP2Triangles(5, 15, 2, 0, 5, 2, 1, 0),
    gsSP2Triangles(6, 16, 7, 0, 6, 7, 8, 0),
    gsSP2Triangles(7, 17, 18, 0, 7, 18, 8, 0),
    gsSP2Triangles(8, 18, 19, 0, 8, 19, 20, 0),
    gsSP2Triangles(9, 6, 8, 0, 9, 8, 20, 0),
    gsSP2Triangles(10, 21, 22, 0, 10, 22, 23, 0),
    gsSP2Triangles(11, 24, 10, 0, 11, 10, 23, 0),
    gsSP2Triangles(12, 13, 23, 0, 12, 23, 22, 0),
    gsSP2Triangles(13, 25, 11, 0, 13, 11, 23, 0),
    gsSPEndDisplayList(),
};

NDN_DECL(Entity_SaveBlock_RenderStar);

NDN_DECL(Entity_SaveBlock_RenderContent);

NDN_DECL(Entity_SaveBlock_RenderBlock);


NDN_DECL(Entity_SaveBlock_RenderNone);
