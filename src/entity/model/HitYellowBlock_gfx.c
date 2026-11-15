#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/HitYellowBlock_1.vtx.inc.c"
#include "entity/model/HitYellowBlock_2.vtx.inc.c"

extern unsigned char D_0A000420_E45E10[];
extern unsigned char D_0A000C20_E46610[];

NDN_DECL(Entity_HitYellowBlock_RenderBlock2);

NDN_DECL(Entity_HitYellowBlock_RenderBlock1);

INCLUDE_IMG("entity/model/HitYellowBlock_face1.png", D_0A000420_E45E10);
INCLUDE_IMG("entity/model/HitYellowBlock_face2.png", D_0A000C20_E46610);
