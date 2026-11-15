#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/PinkFlower_1.vtx.inc.c"
#include "entity/model/PinkFlower_2.vtx.inc.c"
#include "entity/model/PinkFlower_3.vtx.inc.c"
#include "entity/model/PinkFlower_4.vtx.inc.c"
#include "entity/model/PinkFlower_5.vtx.inc.c"

INCLUDE_IMG("entity/model/PinkFlower_light.png", D_0A0005B0_E9BAB0);
NDN_DECL(D_0A0007B0_E9BCB0);

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

NDN_DECL(D_0A0013B8_E9C8B8);
