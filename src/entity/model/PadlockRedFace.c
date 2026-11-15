#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/PadlockRedFace_1.vtx.inc.c"
#include "entity/model/PadlockRedFace_2.vtx.inc.c"
#include "entity/model/PadlockRedFace_3.vtx.inc.c"
#include "entity/model/PadlockRedFace_4.vtx.inc.c"

INCLUDE_IMG("entity/model/PadlockRedFace_face.png", D_0A000700_E5EE30);
NDN_DECL(Entity_PadlockRedFace_LoadTextureFace);

INCLUDE_IMG("entity/model/PadlockRedFace_keyhole.png", D_0A000978_E5F0A8);
INCLUDE_PAL("entity/model/PadlockRedFace_keyhole.pal", D_0A001178_E5F8A8);
NDN_DECL(Entity_PadlockRedFace_LoadTextureKeyhole);

INCLUDE_IMG("entity/model/PadlockRedFace_body.png", D_0A001420_E5FB50);
INCLUDE_PAL("entity/model/PadlockRedFace_body.pal", D_0A001820_E5FF50);
NDN_DECL(Entity_PadlockRedFace_LoadTextureBody);

Mtx Entity_PadlockRedFace_mtxShackle = RDP_MATRIX(
    1.000000, 0.000000, 0.000000,  0.000000,
    0.000000, 1.000000, 0.000000, 22.500000,
    0.000000, 0.000000, 1.000000,  0.000000,
    0.000000, 0.000000, 0.000000,  1.000000
);

NDN_DECL(Entity_PadlockRedFace_RenderShackleMain);

NDN_DECL(Entity_PadlockRedFace_RenderShackle);

NDN_DECL(Entity_PadlockRedFace_RenderKeyhole);

NDN_DECL(Entity_PadlockRedFace_RenderFace);

NDN_DECL(Entity_PadlockRedFace_RenderBody);

NDN_DECL(Entity_PadlockRedFace_RenderMain);

NDN_DECL(Entity_PadlockRedFace_Render);
