#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

INCLUDE_IMG("entity/model/CymbalPlant.png", D_0A000000_E992F0);
INCLUDE_PAL("entity/model/CymbalPlant.pal", D_0A000800_E99AF0);
NDN_DECL(Entity_CymbalPlant_LoadTexture);

#include "entity/model/CymbalPlant_1.vtx.inc.c"
#include "entity/model/CymbalPlant_2.vtx.inc.c"
#include "entity/model/CymbalPlant_3.vtx.inc.c"
#include "entity/model/CymbalPlant_4.vtx.inc.c"
#include "entity/model/CymbalPlant_5.vtx.inc.c"
#include "entity/model/CymbalPlant_6.vtx.inc.c"
#include "entity/model/CymbalPlant_7.vtx.inc.c"
#include "entity/model/CymbalPlant_8.vtx.inc.c"

Mtx Entity_CymbalPlant_RightMtx = RDP_MATRIX(
    1.000000, 0.000000, 0.000000,  2.668290,
    0.000000, 1.000000, 0.000000, 26.668290,
    0.000000, 0.000000, 1.000000,  0.000000,
    0.000000, 0.000000, 0.000000,  1.000000
);

Mtx Entity_CymbalPlant_LeftMtx = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, -2.331711,
    0.000000, 1.000000, 0.000000, 26.668290,
    0.000000, 0.000000, 1.000000,  0.000000,
    0.000000, 0.000000, 0.000000,  1.000000
);

NDN_DECL(Entity_CymbalPlant_RenderNode3);

NDN_DECL(Entity_CymbalPlant_RenderNode4);

NDN_DECL(Entity_CymbalPlant_RenderNode5);

NDN_DECL(D_0A001308_E9A5F8);

NDN_DECL(Entity_CymbalPlant_RenderNode7);

NDN_DECL(Entity_CymbalPlant_RenderNode9);

NDN_DECL(Entity_CymbalPlant_RenderNode8);

NDN_DECL(D_0A001540_E9A830);

NDN_DECL(D_0A001570_E9A860);

NDN_DECL(Entity_CymbalPlant_RenderNode10);

NDN_DECL(Entity_CymbalPlant_RenderNode1);

NDN_DECL(D_0A001658_E9A948);

NDN_DECL(D_0A001670_E9A960);

NDN_DECL(D_0A001688_E9A978);
