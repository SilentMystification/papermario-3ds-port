#include "src/level_up.c"
Gfx D_802A76E8_7A5628[] = {
    gsSPSetLights1(level_up_lights),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
#if VERSION_JP
    gsSPDisplayList(D_802A3C88_7A9508),
#else
    gsSPDisplayList(D_802A4508_7A2448),
#endif
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
#if VERSION_IQUE
    gsSPVertex(&D_802A1218_79F158[0], 8, 0),
#elif VERSION_JP
    gsSPVertex(&D_802A1318_7A6B98[0], 8, 0),
#else
    gsSPVertex(&D_802A1398_79F2D8[0], 8, 0),
#endif
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 6, 7, 0),
    gsSPEndDisplayList(),
};
Gfx D_802A7750_7A5690[] = {
    gsSPSetLights1(level_up_lights),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
#if VERSION_JP
    gsSPDisplayList(D_802A4D00_7AA580),
#else
    gsSPDisplayList(D_802A5580_7A34C0),
#endif
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
#if VERSION_JP
    gsSPVertex(&D_802A1298_7A6B18[0], 8, 0),
#else
    gsSPVertex(&D_802A1318_79F258[0], 8, 0),
#endif
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 6, 7, 0),
    gsSPEndDisplayList(),
};
Gfx D_802A77B8_7A56F8[] = {
    gsSPSetLights1(level_up_lights),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
#if VERSION_JP
    gsSPDisplayList(D_802A7E68_7AD6E8),
#else
    gsSPDisplayList(D_802A3490_7A13D0),
#endif
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
#if VERSION_JP
    gsSPVertex(&D_802A1218_7A6A98[0], 8, 0),
#else
    gsSPVertex(&D_802A1298_79F1D8[0], 8, 0),
#endif
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 6, 7, 0),
    gsSPEndDisplayList(),
};
Gfx D_802A7820_7A5760[] = {
    gsSPSetLights1(level_up_lights),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
#if VERSION_JP
    gsSPDisplayList(D_802A2398_7A7C18),
#else
    gsSPDisplayList(D_802A65F8_7A4538),
#endif
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
#if VERSION_JP
    gsSPVertex(&D_802A1198_7A6A18[0], 8, 0),
#else
    gsSPVertex(&D_802A1218_79F158[0], 8, 0),
#endif
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 6, 7, 0),
    gsSPEndDisplayList(),
};
Gfx D_802A7888_7A57C8[] = {
    gsSPSetLights1(level_up_lights),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
#if VERSION_JP
    gsSPDisplayList(D_802A6DF0_7AC670),
#else
    gsSPDisplayList(D_802A65F8_7A4538),
#endif
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
#if VERSION_IQUE
    gsSPVertex(&D_802A1098_79EFD8[0], 8, 0),
#elif VERSION_JP
    gsSPVertex(&D_802A1118_7A6998[0], 8, 0),
#else
    gsSPVertex(&D_802A1198_79F0D8[0], 8, 0),
#endif
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 6, 7, 0),
    gsSPEndDisplayList(),
};
Gfx D_802A78F0_7A5830[] = {
    gsSPSetLights1(level_up_lights),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
#if VERSION_JP
    gsSPDisplayList(D_802A3410_7A8C90),
#else
    gsSPDisplayList(D_802A7670_7A55B0),
#endif
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
#if VERSION_JP
    gsSPVertex(&D_802A1098_7A6918[0], 8, 0),
#else
    gsSPVertex(&D_802A1118_79F058[0], 8, 0),
#endif
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 6, 7, 0),
    gsSPEndDisplayList(),
};
Gfx D_802A7958_7A5898[] = {
    gsSPSetLights1(level_up_lights),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
#if VERSION_JP
    gsSPDisplayList(D_802A5D78_7AB5F8),
#else
    gsSPDisplayList(D_802A2418_7A0358),
#endif
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
#if VERSION_JP
    gsSPVertex(&D_802A1018_7A6898[0], 8, 0),
#else
    gsSPVertex(&D_802A1098_79EFD8[0], 8, 0),
#endif
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 6, 7, 0),
    gsSPEndDisplayList(),
};
Gfx D_802A79C0_7A5900[] = {
    gsSPSetLights1(level_up_lights),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    gsSPDisplayList(D_802A7670_7A55B0),
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
    gsSPVertex(&D_802A1018_79EF58[0], 8, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(4, 5, 6, 0, 4, 6, 7, 0),
    gsSPEndDisplayList(),
};
Gfx D_802A7A28_7A5968[] = {
    gsSPSetLights1(level_up_lights),
#if VERSION_JP
    gsSPDisplayList(D_802A7958_7A5898),
    gsSPDisplayList(D_802A78F0_7A5830),
    gsSPDisplayList(D_802A7888_7A57C8),
    gsSPDisplayList(D_802A7820_7A5760),
    gsSPDisplayList(D_802A77B8_7A56F8),
    gsSPDisplayList(D_802A7750_7A5690),
#elif VERSION_IQUE
    gsSPDisplayList(D_802A78F0_7A5830),
    gsSPDisplayList(D_802A7888_7A57C8),
#else
    gsSPDisplayList(D_802A79C0_7A5900),
    gsSPDisplayList(D_802A7958_7A5898),
    gsSPDisplayList(D_802A78F0_7A5830),
    gsSPDisplayList(D_802A7888_7A57C8),
    gsSPDisplayList(D_802A7820_7A5760),
    gsSPDisplayList(D_802A77B8_7A56F8),
    gsSPDisplayList(D_802A7750_7A5690),
#endif
    gsSPDisplayList(D_802A76E8_7A5628),
    gsSPEndDisplayList(),
};
Gfx D_802A7A88_7A59C8[] = {
    gsSPSetLights1(level_up_lights),
    gsSPDisplayList(D_802A7A28_7A5968),
    gsSPEndDisplayList(),
};
