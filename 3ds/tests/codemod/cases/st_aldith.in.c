#include "common.h"

void st_f(void* x) {
    gDPSetAlphaDither(gMainGfxPos++, G_AD_NOISE);
}
