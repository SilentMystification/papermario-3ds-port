#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/SuperBlock.vtx.inc.c"

INCLUDE_IMG("entity/model/SuperBlock_tex.png", D_0A000480_E44C80);
INCLUDE_PAL("entity/model/SuperBlock_tex.pal", D_0A000580_E44D80);
NDN_DECL(Entity_SuperBlock_LoadTexture);

Mtx Entity_SuperBlock_mtx = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 1.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

NDN_DECL(Entity_SuperBlock_RenderTop);

NDN_DECL(Entity_SuperBlock_RenderRightFrontEdge);

NDN_DECL(Entity_SuperBlock_RenderLeftFrontEdge);

NDN_DECL(Entity_SuperBlock_RenderRightBackEdge);

NDN_DECL(Entity_SuperBlock_RenderLeftBackEdge);

NDN_DECL(Entity_SuperBlock_RenderBottom);

NDN_DECL(Entity_SuperBlock_RenderBlock);

NDN_DECL(Entity_SuperBlock_Render);
