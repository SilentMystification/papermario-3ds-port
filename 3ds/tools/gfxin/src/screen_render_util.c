#include "src/screen_render_util.c"
Gfx D_8014ED90[] = {
    gsSPEndDisplayList(),
};
Gfx Gfx_DarknessStencilQuad[] = {
    gsSPVertex(Vtx_DarknessStencil, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsDPPipeSync(),
    gsSPEndDisplayList(),
};
