#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/GreenStompSwitch_1.vtx.inc.c"
#include "entity/model/GreenStompSwitch_2.vtx.inc.c"

INCLUDE_IMG("entity/model/GreenStompSwitch_tex.png", D_0A000280_E625F0);
INCLUDE_PAL("entity/model/GreenStompSwitch_tex.pal", D_0A000300_E62670);
NDN_DECL(Entity_GreenStompSwitch_LoadTexture);

INCLUDE_IMG("entity/model/GreenStompSwitch_exclamationMark.png", D_0A0003D8_E62748);
INCLUDE_PAL("entity/model/GreenStompSwitch_exclamationMark.pal", D_0A0004D8_E62848);
NDN_DECL(Entity_GreenStompSwitch_LoadExclMark);

Mtx Entity_GreenStompSwitch_mtx = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 1.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

NDN_DECL(Entity_GreenStompSwitch_RenderBase);

NDN_DECL(Entity_GreenStompSwitch_RenderExclMark);

NDN_DECL(Entity_GreenStompSwitch_RenderMain);

NDN_DECL(Entity_GreenStompSwitch_Render);
