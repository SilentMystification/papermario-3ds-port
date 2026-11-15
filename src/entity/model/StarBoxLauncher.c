#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/StarBoxLauncher_1.vtx.inc.c"
#include "entity/model/StarBoxLauncher_2.vtx.inc.c"
#include "entity/model/StarBoxLauncher_3.vtx.inc.c"
#include "entity/model/StarBoxLauncher_4.vtx.inc.c"

INCLUDE_IMG("entity/model/StarBoxLauncher_link.png", D_0A000290_E55CD0);
NDN_DECL(Entity_StarBoxLauncher_LoadTextureLink);

INCLUDE_IMG("entity/model/StarBoxLauncher_face.png", D_0A000508_E55F48);
INCLUDE_PAL("entity/model/StarBoxLauncher_face.pal", D_0A000D08_E56748);
NDN_DECL(Entity_StarBoxLauncher_LoadTextureFace);

INCLUDE_IMG("entity/model/StarBoxLauncher_box.png", D_0A000DD0_E56810);
INCLUDE_PAL("entity/model/StarBoxLauncher_box.pal", D_0A000ED0_E56910);
NDN_DECL(Entity_StarBoxLauncher_LoadTextureBox);

Mtx Entity_StarBoxLauncher_mtx = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 1.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

NDN_DECL(Entity_StarBoxLauncher_RenderChain);

NDN_DECL(Entity_StarBoxLauncher_RenderFace);

NDN_DECL(Entity_StarBoxLauncher_RenderBoxCover);

NDN_DECL(Entity_StarBoxLauncher_RenderTop);

NDN_DECL(Entity_StarBoxLauncher_RenderBox);

NDN_DECL(Entity_StarBoxLauncher_RenderBottom);

