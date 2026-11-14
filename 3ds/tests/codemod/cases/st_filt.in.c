#include "common.h"

void st_f(void* x) {
    gDPSetTextureFilter(gMainGfxPos++, G_TF_BILERP);
}
