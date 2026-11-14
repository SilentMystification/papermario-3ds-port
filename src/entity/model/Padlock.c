#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/Padlock_1.vtx.inc.c"
#include "entity/model/Padlock_2.vtx.inc.c"
#include "entity/model/Padlock_3.vtx.inc.c"
#include "entity/model/Padlock_4.vtx.inc.c"

INCLUDE_IMG("entity/model/Padlock_face.png", D_0A000700_E5B250);
INCLUDE_PAL("entity/model/Padlock_face.pal", D_0A000B00_E5B650);
NDN_DECL(Entity_Padlock_LoadTextureFace);

INCLUDE_IMG("entity/model/Padlock_keyhole.png", D_0A000DA8_E5B8F8);
INCLUDE_PAL("entity/model/Padlock_keyhole.pal", D_0A0015A8_E5C0F8);
NDN_DECL(Entity_Padlock_LoadTextureKeyhole);

INCLUDE_IMG("entity/model/Padlock_body.png", D_0A001850_E5C3A0);
INCLUDE_PAL("entity/model/Padlock_body.pal", D_0A001C50_E5C7A0);
NDN_DECL(Entity_Padlock_LoadTextureBody);

Mtx Entity_Padlock_mtxShackle = RDP_MATRIX(
    1.000000, 0.000000, 0.000000,  0.000000,
    0.000000, 1.000000, 0.000000, 22.500000,
    0.000000, 0.000000, 1.000000,  0.000000,
    0.000000, 0.000000, 0.000000,  1.000000
);

NDN_DECL(Entity_Padlock_RenderShackleMain);

NDN_DECL(Entity_Padlock_RenderShackle);

NDN_DECL(Entity_Padlock_RenderKeyhole);

NDN_DECL(Entity_Padlock_RenderFace);

NDN_DECL(Entity_Padlock_RenderBody);

NDN_DECL(Entity_Padlock_Render);
