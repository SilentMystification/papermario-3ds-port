#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/RedSwitch_1.vtx.inc.c"
#include "entity/model/RedSwitch_2.vtx.inc.c"
#include "entity/model/RedSwitch_3.vtx.inc.c"

INCLUDE_IMG("entity/model/RedSwitch_tex.png", D_0A0005D0_E63090);
INCLUDE_PAL("entity/model/RedSwitch_tex.pal", D_0A0006D0_E63190);
NDN_DECL(Entity_RedSwitch_LoadTexture);

INCLUDE_IMG("entity/model/RedSwitch_exclamationMark.png", D_0A000978_E63438);
NDN_DECL(Entity_RedSwitch_LoadExclMark);

Mtx Entity_RedSwitch_mtxExclMark = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 1.000000, 0.000000, 6.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

Mtx Entity_RedSwitch_mtxRedBubble = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, -0.026600,
    0.000000, 1.000000, 0.000000,  5.000000,
    0.000000, 0.000000, 1.000000,  0.609116,
    0.000000, 0.000000, 0.000000,  1.000000
);

NDN_DECL(Entity_RedSwitch_RenderRedBubble);

NDN_DECL(Entity_RedSwitch_RenderRotatedRedBubble);

NDN_DECL(Entity_RedSwitch_RenderExclMark);

NDN_DECL(Entity_RedSwitch_RenderRotatedExclMark);

NDN_DECL(Entity_RedSwitch_RenderBase);

NDN_DECL(Entity_RedSwitch_RenderBaseMain);

NDN_DECL(Entity_RedSwitch_RenderMain);

NDN_DECL(Entity_RedSwitch_Render);
