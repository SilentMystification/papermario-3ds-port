#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/Tweester_1.vtx.inc.c"
#include "entity/model/Tweester_2.vtx.inc.c"
#include "entity/model/Tweester_3.vtx.inc.c"
#include "entity/model/Tweester_4.vtx.inc.c"

INCLUDE_IMG("entity/model/Tweester_img1.png", D_0A000BF0_E57A50);
INCLUDE_IMG("entity/model/Tweester_img2.png", D_0A000FF0_E57E50);
NDN_DECL(Entity_Tweester_LoadTextures);

INCLUDE_IMG("entity/model/Tweester_img3.png", D_0A0014A0_E58300);
INCLUDE_PAL("entity/model/Tweester_img3.pal", D_0A0018A0_E58700);
NDN_DECL(Entity_Tweester_LoadTextureFace);

Mtx Entity_Tweester_mtxInnerWhirl = RDP_MATRIX(
    0.999848, 0.017457, 0.000000, 0.000000,
    -0.01746, 0.999848, 0.000000, 0.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

Mtx Entity_Tweester_mtxOuterWhirl = RDP_MATRIX(
    1.199997, 0.000000, 0.000000, 0.000000,
    0.000000, 0.899994, 0.000000, 0.000000,
    0.000000, 0.000000, 1.199997, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

NDN_DECL(D_0A0019E8_E58848);

NDN_DECL(D_0A001B10_E58970);

NDN_DECL(D_0A001B50_E589B0);

NDN_DECL(Entity_Tweester_RenderOuterWhirl);

NDN_DECL(D_0A001CD0_E58B30);

NDN_DECL(Entity_Tweester_RenderInnerWhirl);

NDN_DECL(Entity_Tweester_Render);
