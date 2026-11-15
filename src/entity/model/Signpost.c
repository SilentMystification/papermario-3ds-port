#include "common.h"
#include "nd_gtab.h"
#include "include_asset.h"

#include "entity/model/Signpost_1.vtx.inc.c"
#include "entity/model/Signpost_2.vtx.inc.c"

INCLUDE_IMG("entity/model/Signpost.png", D_0A000580_E59A00);
INCLUDE_PAL("entity/model/Signpost.pal", D_0A000780_E59C00);
NDN_DECL(Entity_Signpost_LoadTexture);

NDN_DECL(Entity_Signpost_RenderSign);

NDN_DECL(Entity_Signpost_RenderPole);

NDN_DECL(Entity_Signpost_RenderMain);

NDN_DECL(Entity_Signpost_Render);
