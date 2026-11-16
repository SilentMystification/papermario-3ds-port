#include "common.h"

Gfx* hA[] = { D_1, D_2 };
Gfx* hB[] = { D_3 };

void f(int i) {
    gSPDisplayList(gMainGfxPos++, hA[i]);
    Gfx* dl = hB[i];
    gSPDisplayList(gMainGfxPos++, dl);
}
