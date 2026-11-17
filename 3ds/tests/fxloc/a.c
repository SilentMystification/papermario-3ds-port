#include "common.h"

Gfx* hA[] = { D_1, D_2 };
Gfx* hB[] = { D_3 };

void f(int i) {
    Gfx* dl = hA[i];
    Gfx* other = hB[i];
    nd_draw_list(dl, ND_SELF); // hB in a comment
    gSPDisplayList(gMainGfxPos++, other);
    other += 1;
}
