#include "common.h"

void st_f(void* x) {
    gDPSetAlphaCompare(gMainGfxPos++, G_AC_THRESHOLD);
}
