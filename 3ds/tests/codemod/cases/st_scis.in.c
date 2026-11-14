#include "common.h"

void st_f(void* x) {
    gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, 10, 20, 300, 200);
}
