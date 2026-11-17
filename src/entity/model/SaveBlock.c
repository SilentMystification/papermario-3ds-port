#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"
#include "include_asset.h"

#include "entity/model/SaveBlock_1.vtx.inc.c"
#include "entity/model/SaveBlock_2.vtx.inc.c"
#include "entity/model/SaveBlock_3.vtx.inc.c"

INCLUDE_IMG("entity/model/SaveBlock_star.png", D_0A000900_E4BBE0);
NDN_DECL(Entity_SaveBlock_LoadTextureStar);

INCLUDE_IMG("entity/model/SaveBlock_face.png", D_0A001170_E4C450);
NDN_DECL(Entity_SaveBlock_LoadTextureFace);

INCLUDE_IMG("entity/model/SaveBlock_letterS.png", D_0A0021E8_E4D4C8);

NDN_DECL(Entity_SaveBlock_LoadTextureLetterS);


Mtx Entity_SaveBlock_Mtx = RDP_MATRIX(
    1.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 1.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
);

NDN_DECL(Entity_SaveBlock_RenderLetterS);


NDN_DECL(Entity_SaveBlock_RenderFaces);

NDN_DECL(Entity_SaveBlock_RenderStar);

NDN_DECL(Entity_SaveBlock_RenderContent);

NDN_DECL(Entity_SaveBlock_RenderBlock);


NDN_DECL(Entity_SaveBlock_RenderNone);
