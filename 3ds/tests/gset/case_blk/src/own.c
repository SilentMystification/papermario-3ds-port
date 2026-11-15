#include "common.h"
Gfx D[] = {
    gsSPEndDisplayList(),
};
Gfx E[] = {
    gsSPEndDisplayList(),
};
Gfx F[] = {
    gsSPEndDisplayList(),
};
void own(void) {
    gSPDisplayList(gMainGfxPos++, D);
    n = sizeof E;
    evt(SetCustomGfx, F);
}
