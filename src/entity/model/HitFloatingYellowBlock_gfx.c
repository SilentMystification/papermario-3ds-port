#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/HitFloatingYellowBlock_1.vtx.inc.c"
#include "entity/model/HitFloatingYellowBlock_2.vtx.inc.c"

extern unsigned char D_0A000C40_E47CF0[];
extern unsigned char D_0A000440_E474F0[];

NDN_DECL(Entity_HitFloatingYellowBlock_RenderBlock2);

NDN_DECL(Entity_HitFloatingYellowBlock_RenderBlock1);

INCLUDE_IMG("entity/model/HitFloatingYellowBlock_face1.png", D_0A000440_E474F0);
INCLUDE_IMG("entity/model/HitFloatingYellowBlock_face2.png", D_0A000C40_E47CF0);
