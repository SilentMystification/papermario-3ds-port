#include "common.h"

void tex_f(void* x) {
    gDPSetFillColor(gMainGfxPos++, PACK_FILL_COLOR(224, 224, 224, 1));
    gDPFillRectangle(gMainGfxPos++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
}
