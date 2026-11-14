#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/Chest_1.vtx.inc.c"
#include "entity/model/Chest_2.vtx.inc.c"
#include "entity/model/Chest_3.vtx.inc.c"
#include "entity/model/Chest_4.vtx.inc.c"

INCLUDE_IMG("entity/model/Chest_tex1.png", D_0A000410_E64CE0);
NDN_DECL(Entity_Chest_LoadTexture1);

INCLUDE_IMG("entity/model/Chest_tex2.png", D_0A000898_E65168);
INCLUDE_PAL("entity/model/Chest_tex2.pal", D_0A000998_E65268);
NDN_DECL(Entity_Chest_LoadTexture2);

INCLUDE_IMG("entity/model/Chest_lock.png", D_0A000A70_E65340);
INCLUDE_PAL("entity/model/Chest_lock.pal", D_0A000B70_E65440);
NDN_DECL(Entity_Chest_LoadTextureLock);

Mtx Entity_Chest_LidMtx = RDP_MATRIX(
    1.000000, 0.000000, 0.000000,   0.000000,
    0.000000, 1.000000, 0.000000,  29.000000,
    0.000000, 0.000000, 1.000000, -23.000000,
    0.000000, 0.000000, 0.000000,   1.000000
);

NDN_DECL(Entity_Chest_RenderLidLock);

NDN_DECL(Entity_Chest_RenderLidWooden);

NDN_DECL(Entity_Chest_RenderLid);

NDN_DECL(Entity_Chest_RenderBoxLock);

NDN_DECL(Entity_Chest_RenderBoxWooden);

NDN_DECL(Entity_Chest_RenderBox);
