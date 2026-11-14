#include "common.h"
#include "include_asset.h"

Vtx M_vtxA[] = {
    {{{   -10,   -10,     0 }, 0, {     0,     0 }, { 255, 255, 255, 255 }}},
};
Gfx A[] = {
    gsSPVertex(M_vtxA, 1, 0),
    }
    gsSPEndDisplayList(),
};
Gfx B[] = {
    gsSPEndDisplayList(),
};
Gfx C[] = {
    gsSPDisplayList(B),
    gsSPEndDisplayList(),
};
