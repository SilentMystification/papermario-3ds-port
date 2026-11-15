#define NAMESPACE ns

#include "ast/F.gfx.inc.c"

void f(void) {
    gSPDisplayList(gMainGfxPos++, N(F_gfx));
}
