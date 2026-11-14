#include "common.h"

void tex_f(void* x) {
    gDPLoadSync(gMainGfxPos++);
    gDPTileSync(gMainGfxPos++);
    gDPPipeSync(gMainGfxPos++);
    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
}
