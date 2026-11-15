#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/PadlockRedFrame_1.vtx.inc.c"
#include "entity/model/PadlockRedFrame_2.vtx.inc.c"
#include "entity/model/PadlockRedFrame_3.vtx.inc.c"
#include "entity/model/PadlockRedFrame_4.vtx.inc.c"

INCLUDE_IMG("entity/model/PadlockRedFrame_keyhole.png", D_0A000540_E5D2C0);
INCLUDE_PAL("entity/model/PadlockRedFrame_keyhole.pal", D_0A000D40_E5DAC0);
NDN_DECL(Entity_PadlockRedFrame_LoadTextureKeyhole);

INCLUDE_IMG("entity/model/PadlockRedFrame_face.png", D_0A000FE8_E5DD68);
INCLUDE_PAL("entity/model/PadlockRedFrame_face.pal", D_0A0013E8_E5E168);
NDN_DECL(Entity_PadlockRedFrame_LoadTextureFace);

Mtx Entity_PadlockRedFrame_mtxShackle = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 1.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

NDN_DECL(Entity_PadlockRedFrame_RenderShackleMain);

NDN_DECL(Entity_PadlockRedFrame_RenderShackle);

NDN_DECL(Entity_PadlockRedFrame_RenderKeyhole);

NDN_DECL(Entity_PadlockRedFrame_RenderFace);

NDN_DECL(Entity_PadlockRedFrame_RenderBody);

NDN_DECL(Entity_PadlockRedFrame_RenderMain);

NDN_DECL(Entity_PadlockRedFrame_Render);
