#include "src/starpoint.c"
Gfx D_802A20D0_7A9A70[] = {
    gsSPSetLights1(starpoint_lights1),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    gsSPDisplayList(D_802A2018_7A99B8),
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
    gsSPVertex(D_802A2090_7A9A30, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};
Gfx D_802A2130_7A9AD0[] = {
    gsSPSetLights1(starpoint_lights1),
    gsSPDisplayList(D_802A20D0_7A9A70),
    gsSPEndDisplayList(),
};
Gfx D_802A2158_7A9AF8[] = {
    gsSPSetLights1(starpoint_lights1),
    gsSPDisplayList(D_802A2130_7A9AD0),
    gsSPEndDisplayList(),
};
Gfx D_802A3250_7AABF0[] = {
    gsSPSetLights1(starpoint_lights2),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    gsSPDisplayList(D_802A31D8_7AAB78),
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
    gsSPVertex(D_802A2198_7A9B38, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};
Gfx D_802A32B0_7AAC50[] = {
    gsSPSetLights1(starpoint_lights2),
    gsSPDisplayList(D_802A3250_7AABF0),
    gsSPEndDisplayList(),
};
Gfx D_802A32D8_7AAC78[] = {
    gsSPSetLights1(starpoint_lights2),
    gsSPDisplayList(D_802A32B0_7AAC50),
    gsSPEndDisplayList(),
};
Gfx D_802ADF90_7B5930[] = {
    gsSPEndDisplayList(),
};
