#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/PushBlock.vtx.inc.c"

INCLUDE_IMG("entity/model/PushBlock_face.png", D_0A000120_E3DB80);
INCLUDE_PAL("entity/model/PushBlock_face.pal", D_0A0001A0_E3DC00);
NDN_DECL(Entity_PushBlock_LoadTexture);

Mtx Entity_PushBlock_mtx = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 1.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

NDN_DECL(Entity_PushBlock_RenderFaces);

NDN_DECL(Entity_PushBlock_RenderBlock);

NDN_DECL(Entity_PushBlock_Render);
