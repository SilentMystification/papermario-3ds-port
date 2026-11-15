#include "include_asset.h"

#if VERSION_JP
INCLUDE_IMG("effects/gfx/world/ch1.png", D_09000000_3F4120);
INCLUDE_IMG("effects/gfx/world/ch2.png", D_09001000_3F5120);
INCLUDE_IMG("effects/gfx/world/ch3.png", D_09002000_3F6120);
INCLUDE_IMG("effects/gfx/world/ch4.png", D_09003000_3F7120);
INCLUDE_IMG("effects/gfx/world/ch5.png", D_09004000_3F8120);
INCLUDE_IMG("effects/gfx/world/ch6.png", D_09005000_3F9120);
INCLUDE_IMG("effects/gfx/world/ch7.png", D_09006000_3FA120);
INCLUDE_IMG("effects/gfx/world/ch8.png", D_09007000_3FB120);
INCLUDE_IMG("effects/gfx/world/ch0.png", D_09008000_3FC120);
INCLUDE_IMG("effects/gfx/D_09000B40_3EC9A0.png", D_09008B40_3FCC60);
INCLUDE_IMG("effects/gfx/world/chapter_rainbow.png", D_09009680_3FD7A0);
INCLUDE_IMG("effects/gfx/world/text_stage.png", D_09009780_3FD8A0);
INCLUDE_IMG("effects/gfx/world/D_09009FA0_3FE0C0.png", D_09009FA0_3FE0C0);
INCLUDE_IMG("effects/gfx/world/text_clear.png", D_0900A7C0_3FE8E0);
INCLUDE_IMG("effects/gfx/world/D_0900AF40_3FF060.png", D_0900AF40_3FF060);
#else
INCLUDE_IMG("effects/gfx/world/ch0.png", D_09000000_3EBE60);
INCLUDE_IMG("effects/gfx/D_09000B40_3EC9A0.png", D_09000B40_3EC9A0);
INCLUDE_IMG("effects/gfx/world/text_chapter.png", D_09001680_3ED4E0);
INCLUDE_IMG("effects/gfx/D_09001B80_3ED9E0.png", D_09001B80_3ED9E0);
INCLUDE_IMG("effects/gfx/D_09002080_3EDEE0.png", D_09002080_3EDEE0);
INCLUDE_IMG("effects/gfx/D_09002580_3EE3E0.png", D_09002580_3EE3E0);
INCLUDE_IMG("effects/gfx/world/text_end_of.png", D_09002A80_3EE8E0);
INCLUDE_IMG("effects/gfx/world/exclamation_point.png", D_09003AC0_3EF920);
INCLUDE_IMG("effects/gfx/world/ch1.png", D_09003D40_3EFBA0);
INCLUDE_IMG("effects/gfx/world/ch2.png", D_09004D40_3F0BA0);
INCLUDE_IMG("effects/gfx/world/ch3.png", D_09005D40_3F1BA0);
INCLUDE_IMG("effects/gfx/world/ch4.png", D_09006D40_3F2BA0);
INCLUDE_IMG("effects/gfx/world/ch5.png", D_09007D40_3F3BA0);
INCLUDE_IMG("effects/gfx/world/ch6.png", D_09008D40_3F4BA0);
INCLUDE_IMG("effects/gfx/world/ch7.png", D_09009D40_3F5BA0);
INCLUDE_IMG("effects/gfx/world/ch8.png", D_0900AD40_3F6BA0);
INCLUDE_IMG("effects/gfx/world/chapter_rainbow.png", D_0900BD40_3F7BA0);
#endif


#if VERSION_JP
#include "effects/gfx/D_0900B900_3FFA20.gfx.inc.c"
#include "effects/gfx/D_0900B9E8_3FFB08.gfx.inc.c"
#include "effects/gfx/D_0900BA30_3FFB50.gfx.inc.c"
#include "effects/gfx/D_0900BB18_3FFC38.gfx.inc.c"
#else

// TODO allow splat to output the reference to halfway into the texture
//#include "effects/gfx/D_0900C328_3F8188.gfx.inc.c"
Gfx D_0900C328_3F8188[] = {
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPLoadTextureTile(&D_09002A80_3EE8E0[0x820], G_IM_FMT_IA, G_IM_SIZ_8b, 104, 0, 0, 0, 103, 19, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 7, 6, G_TX_NOLOD, G_TX_NOLOD),
    gsSPEndDisplayList(),
};

#endif

