#include "common.h"

void geo_f(void* x) {
    gDPSetDepthImage(gMainGfxPos++, zbuf);
}
