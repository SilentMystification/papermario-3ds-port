#include "common.h"

#include "include_asset.h"

INCLUDE_IMG("effects/gfx/D_09000000_3733E0.png", D_09000000_3733E0);
INCLUDE_IMG("effects/gfx/D_09000800_373BE0.png", D_09000800_373BE0);
#include "effects/gfx/D_09000A00_373DE0.gfx.inc.c"
#include "effects/gfx/D_09000AE8_373EC8.vtx.inc.c"
#include "effects/gfx/D_09000B38_373F18.vtx.inc.c"
extern Gfx D_09000BA8_373F88[];
#define D_09000B88_373F68 D_09000B88_373F68_body
#include "effects/gfx/D_09000B88_373F68.gfx.inc.c"
#undef D_09000B88_373F68
Gfx D_09000B88_373F68[] = {
    gsSPDisplayList(D_09000B88_373F68_body),
    gsSPBranchList(D_09000BA8_373F88),
};
#include "effects/gfx/D_09000BA8_373F88.gfx.inc.c"
#include "effects/gfx/D_09000BC0_373FA0.vtx.inc.c"
#include "effects/gfx/D_09000C00_373FE0.gfx.inc.c"
#include "effects/gfx/D_09000C20_374000.gfx.inc.c"
