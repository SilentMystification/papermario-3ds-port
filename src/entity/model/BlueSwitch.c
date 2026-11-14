#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/BlueSwitch_1.vtx.inc.c"
#include "entity/model/BlueSwitch_2.vtx.inc.c"
#include "entity/model/BlueSwitch_3.vtx.inc.c"
#include "entity/model/BlueSwitch_4.vtx.inc.c"

INCLUDE_IMG("entity/model/BlueSwitch_tex.png", D_0A0005D0_E63F90);
INCLUDE_PAL("entity/model/BlueSwitch_tex.pal", D_0A0006D0_E64090);
NDN_DECL(Entity_BlueSwitch_LoadTexture);

INCLUDE_IMG("entity/model/BlueSwitch_exclamationMark.png", D_0A000978_E64338);
NDN_DECL(Entity_BlueSwitch_LoadExclMark);

Mtx Entity_BlueSwitch_mtxExclMark = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 1.000000, 0.000000, 6.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

Mtx Entity_BlueSwitch_mtxBlueBubble = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, -0.026597,
    0.000000, 1.000000, 0.000000,  5.000000,
    0.000000, 0.000000, 1.000000,  0.609116,
    0.000000, 0.000000, 0.000000,  1.000000
);

NDN_DECL(Entity_BlueSwitch_RenderBlueBubble);

NDN_DECL(Entity_BlueSwitch_RenderRotatedBlueBubble);

NDN_DECL(Entity_BlueSwitch_RenderExclMark);

NDN_DECL(Entity_BlueSwitch_RenderRotatedExclMark);

NDN_DECL(Entity_BlueSwitch_RenderBase2);

NDN_DECL(Entity_BlueSwitch_RenderBase);

NDN_DECL(Entity_BlueSwitch_RenderBaseMain);

NDN_DECL(Entity_BlueSwitch_RenderMain);

NDN_DECL(Entity_BlueSwitch_Render);
