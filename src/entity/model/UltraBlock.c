#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/UltraBlock.vtx.inc.c"

INCLUDE_IMG("entity/model/UltraBlock_tex.png", D_0A000480_E45590);
INCLUDE_PAL("entity/model/UltraBlock_tex.pal", D_0A000580_E45690);
NDN_DECL(Entity_UltraBlock_LoadTexture);

Mtx Entity_UltraBlock_mtx = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 1.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

NDN_DECL(Entity_UltraBlock_RenderTop);

NDN_DECL(Entity_UltraBlock_RenderRightFrontEdge);

NDN_DECL(Entity_UltraBlock_RenderLeftFrontEdge);

NDN_DECL(Entity_UltraBlock_RenderRightBackEdge);

NDN_DECL(Entity_UltraBlock_RenderLeftBackEdge);

NDN_DECL(Entity_UltraBlock_RenderBottom);

NDN_DECL(Entity_UltraBlock_RenderBlock);

NDN_DECL(Entity_UltraBlock_Render);
