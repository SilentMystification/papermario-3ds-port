#include "common.h"

void geo_f(void* x) {
    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, SpriteShadingPalette);
}
