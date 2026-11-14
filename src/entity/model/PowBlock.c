#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/PowBlock.vtx.inc.c"

INCLUDE_IMG("entity/model/PowBlock_face.png", D_0A000100_E43C20);
INCLUDE_PAL("entity/model/PowBlock_face.pal", D_0A000900_E44420);
NDN_DECL(Entity_PowBlock_LoadTexture);

Mtx Entity_PowBlock_mtx = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 1.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

NDN_DECL(Entity_PowBlock_RenderTopBottom);

NDN_DECL(Entity_PowBlock_RenderSides);

NDN_DECL(Entity_PowBlock_RenderFaces);

NDN_DECL(Entity_PowBlock_RenderBlock);

NDN_DECL(Entity_PowBlock_Render);
