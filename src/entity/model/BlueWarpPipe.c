#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/BlueWarpPipe_1.vtx.inc.c"
#include "entity/model/BlueWarpPipe_2.vtx.inc.c"
#include "entity/model/BlueWarpPipe_3.vtx.inc.c"

INCLUDE_IMG("entity/model/BlueWarpPipe_texBase.png", D_0A000320_E58FF0);
INCLUDE_PAL("entity/model/BlueWarpPipe_texBase.pal", D_0A0003A0_E59070);
NDN_DECL(Entity_BlueWarpPipe_LoadTextureBase);

INCLUDE_IMG("entity/model/BlueWarpPipe_texPipe.png", D_0A000470_E59140);
INCLUDE_PAL("entity/model/BlueWarpPipe_texPipe.pal", D_0A000570_E59240);
NDN_DECL(Entity_BlueWarpPipe_LoadTexturePipe);

Mtx Entity_BlueWarpPipe_mtx = RDP_MATRIX(
    1.000000, 0.000000, 0.000000,   0.000000,
    0.000000, 1.000000, 0.000000, -55.000000,
    0.000000, 0.000000, 1.000000,   0.000000,
    0.000000, 0.000000, 0.000000,   1.000000
);

NDN_DECL(Entity_BlueWarpPipe_RenderBase);

NDN_DECL(Entity_BlueWarpPipe_RenderPipe);
