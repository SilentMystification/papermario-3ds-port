#include "src/effects/gfx/fire_breath.c"
#include "effects/gfx/D_09000A00_373DE0.gfx.inc.c"
#include "effects/gfx/D_09000C20_374000.gfx.inc.c"
#include "effects/gfx/D_09000C00_373FE0.gfx.inc.c"
extern Gfx D_09000BA8_373F88[];
#define D_09000B88_373F68 D_09000B88_373F68_body
#include "effects/gfx/D_09000B88_373F68.gfx.inc.c"
#undef D_09000B88_373F68
Gfx D_09000B88_373F68[] = {
    gsSPDisplayList(D_09000B88_373F68_body),
    gsSPBranchList(D_09000BA8_373F88),
};
#include "effects/gfx/D_09000BA8_373F88.gfx.inc.c"
