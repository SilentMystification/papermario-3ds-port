#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/PadlockBlueFace_1.vtx.inc.c"
#include "entity/model/PadlockBlueFace_2.vtx.inc.c"
#include "entity/model/PadlockBlueFace_3.vtx.inc.c"
#include "entity/model/PadlockBlueFace_4.vtx.inc.c"

INCLUDE_IMG("entity/model/PadlockBlueFace_face.png", D_0A000700_E60C50);
NDN_DECL(Entity_PadlockBlueFace_LoadTextureFace);

INCLUDE_IMG("entity/model/PadlockBlueFace_keyhole.png", D_0A000978_E60EC8);
INCLUDE_PAL("entity/model/PadlockBlueFace_keyhole.pal", D_0A001178_E616C8);
NDN_DECL(Entity_PadlockBlueFace_LoadTextureKeyhole);

INCLUDE_IMG("entity/model/PadlockBlueFace_body.png", D_0A001420_E61970);
INCLUDE_PAL("entity/model/PadlockBlueFace_body.pal", D_0A001820_E61D70);
NDN_DECL(Entity_PadlockBlueFace_LoadTextureBody);

Mtx Entity_PadlockBlueFace_mtxShackle = RDP_MATRIX(
    1.000000, 0.000000, 0.000000,  0.000000,
    0.000000, 1.000000, 0.000000, 22.500000,
    0.000000, 0.000000, 1.000000,  0.000000,
    0.000000, 0.000000, 0.000000,  1.000000
);

NDN_DECL(Entity_PadlockBlueFace_RenderShackleMain);

NDN_DECL(Entity_PadlockBlueFace_RenderShackle);

NDN_DECL(Entity_PadlockBlueFace_RenderKeyhole);

NDN_DECL(Entity_PadlockBlueFace_RenderFace);

NDN_DECL(Entity_PadlockBlueFace_RenderBody);

NDN_DECL(Entity_PadlockBlueFace_RenderMain);

NDN_DECL(Entity_PadlockBlueFace_Render);
