#include "src/effects/gfx/snowfall.c"
#include "effects/gfx/D_09000D30_38DDA0.gfx.inc.c"
#include "effects/gfx/D_09000D50_38DDC0.gfx.inc.c"
#include "effects/gfx/D_09000C10_38DC80.gfx.inc.c"

// FX3d: in the ROM this 16 byte list (RDP pipe sync, G_TEXTURE s t 0xFFFF level 2 tile 0 on) is followed directly by the list above, so it runs the setup list by falling through. A separate host array does not fall through, so link it with a branch.
Gfx D_09000C00_38DC70[] = {
    gsDPPipeSync(),
    gsSPTexture(0xFFFF, 0xFFFF, 2, G_TX_RENDERTILE, G_ON),
    gsSPBranchList(D_09000C10_38DC80),
};
