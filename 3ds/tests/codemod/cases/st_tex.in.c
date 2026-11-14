#include "common.h"

void st_f(void* x) {
    gSPTexture(gMainGfxPos++, 0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON);
}
