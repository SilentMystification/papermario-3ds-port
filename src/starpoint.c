#include "common.h"
#include "nd_gtab.h"
#include "entity.h"
#include "include_asset.h"

#if !VERSION_PAL

Lights1 starpoint_lights1 = gdSPDefLights1(0, 0, 0, 0, 0, 0, 0, 0, 0);

#if VERSION_JP
INCLUDE_IMG("starpoint/text_get.png", D_802A1018_7B0A88);
INCLUDE_IMG("starpoint/text_point.png", D_802A1818_7B1288);
INCLUDE_IMG("starpoint/text_star.png", D_802A2018_7B1A88);
#include "starpoint/load_text_get.gfx.inc.c"
#include "starpoint/load_text_point.gfx.inc.c"
#include "starpoint/load_text_star.gfx.inc.c"
#include "starpoint/text_star.vtx.inc.c"
#include "starpoint/text_point.vtx.inc.c"
#include "starpoint/text_get.vtx.inc.c"

Gfx D_802A2A40_7B24B0[] = {
    gsSPSetLights1(starpoint_lights1),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    gsSPDisplayList(D_802A2818_7B2288),
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
    gsSPVertex(D_802A2A00_7B2470, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList()
};
Gfx D_802A2AA0_7B2510[] = {
    gsSPSetLights1(starpoint_lights1),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    gsSPDisplayList(D_802A2890_7B2300),
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
    gsSPVertex(D_802A29C0_7B2430, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList()
};
Gfx D_802A2B00_7B2570[] = {
    gsSPSetLights1(starpoint_lights1),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(G_RM_XLU_SURF, G_RM_XLU_SURF2),
    gsSPDisplayList(D_802A2908_7B2378),
    gsSPClearGeometryMode(G_LIGHTING),
    gsSPSetGeometryMode(G_CULL_BACK | G_SHADING_SMOOTH),
    gsSPVertex(D_802A2980_7B23F0, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList()
};
Gfx D_802A2B60_7B25D0[] = {
    gsSPSetLights1(starpoint_lights1),
    gsSPDisplayList(D_802A2B00_7B2570),
    gsSPDisplayList(D_802A2AA0_7B2510),
    gsSPDisplayList(D_802A2A40_7B24B0),
    gsSPEndDisplayList()
};
Gfx D_802A2B98_7B2608[] = {
    gsSPSetLights1(starpoint_lights1),
    gsSPDisplayList(D_802A2B60_7B25D0),
    gsSPEndDisplayList()
};
#else
INCLUDE_IMG("starpoint/starpoint.png", D_802A1018_7A89B8);
NDN_DECL(D_802A2018_7A99B8);
#include "starpoint/starpoint.vtx.inc.c"
NDN_DECL(D_802A20D0_7A9A70);
NDN_DECL(D_802A2130_7A9AD0);
NDN_DECL(D_802A2158_7A9AF8);

Lights1 starpoint_lights2 = gdSPDefLights1(0, 0, 0, 0, 0, 0, 0, 0, 0);
#include "starpoint/starpoints.vtx.inc.c"
INCLUDE_IMG("starpoint/starpoints.png", D_802A21D8_7A9B78);
NDN_DECL(D_802A31D8_7AAB78);

NDN_DECL(D_802A3250_7AABF0);
NDN_DECL(D_802A32B0_7AAC50);
NDN_DECL(D_802A32D8_7AAC78);
#endif

#endif

#include "starpoint/digit_0.vtx.inc.c"
INCLUDE_IMG("starpoint/digit_0.png", D_802A3340_7AACE0);
NDN_DECL(D_802A4340_7ABCE0);
NDN_DECL(D_802A43B8_7ABD58);
NDN_DECL(D_802A4400_7ABDA0);
NDN_DECL(D_802A4410_7ABDB0);

#include "starpoint/digit_1.vtx.inc.c"
INCLUDE_IMG("starpoint/digit_1.png", D_802A4460_7ABE00);
NDN_DECL(D_802A5460_7ACE00);
NDN_DECL(D_802A54D8_7ACE78);
NDN_DECL(D_802A5520_7ACEC0);
NDN_DECL(D_802A5530_7ACED0);

#include "starpoint/digit_2.vtx.inc.c"
INCLUDE_IMG("starpoint/digit_2.png", D_802A5580_7ACF20);
NDN_DECL(D_802A6580_7ADF20);
NDN_DECL(D_802A65F8_7ADF98);
NDN_DECL(D_802A6640_7ADFE0);
NDN_DECL(D_802A6650_7ADFF0);

#include "starpoint/digit_3.vtx.inc.c"
INCLUDE_IMG("starpoint/digit_3.png", D_802A66A0_7AE040);
NDN_DECL(D_802A76A0_7AF040);
NDN_DECL(D_802A7718_7AF0B8);
NDN_DECL(D_802A7760_7AF100);
NDN_DECL(D_802A7770_7AF110);

#include "starpoint/digit_4.vtx.inc.c"
INCLUDE_IMG("starpoint/digit_4.png", D_802A77C0_7AF160);
NDN_DECL(D_802A87C0_7B0160);
NDN_DECL(D_802A8838_7B01D8);
NDN_DECL(D_802A8880_7B0220);
NDN_DECL(D_802A8890_7B0230);

#include "starpoint/digit_5.vtx.inc.c"
INCLUDE_IMG("starpoint/digit_5.png", D_802A88E0_7B0280);
NDN_DECL(D_802A98E0_7B1280);
NDN_DECL(D_802A9958_7B12F8);
NDN_DECL(D_802A99A0_7B1340);
NDN_DECL(D_802A99B0_7B1350);

#include "starpoint/digit_6.vtx.inc.c"
INCLUDE_IMG("starpoint/digit_6.png", D_802A9A00_7B13A0);
NDN_DECL(D_802AAA00_7B23A0);
NDN_DECL(D_802AAA78_7B2418);
NDN_DECL(D_802AAAC0_7B2460);
NDN_DECL(D_802AAAD0_7B2470);

#include "starpoint/digit_7.vtx.inc.c"
INCLUDE_IMG("starpoint/digit_7.png", D_802AAB20_7B24C0);
NDN_DECL(D_802ABB20_7B34C0);
NDN_DECL(D_802ABB98_7B3538);
NDN_DECL(D_802ABBE0_7B3580);
NDN_DECL(D_802ABBF0_7B3590);

#include "starpoint/digit_8.vtx.inc.c"
INCLUDE_IMG("starpoint/digit_8.png", D_802ABC40_7B35E0);
NDN_DECL(D_802ACC40_7B45E0);
NDN_DECL(D_802ACCB8_7B4658);
NDN_DECL(D_802ACD00_7B46A0);
NDN_DECL(D_802ACD10_7B46B0);

#include "starpoint/digit_9.vtx.inc.c"
INCLUDE_IMG("starpoint/digit_9.png", D_802ACD60_7B4700);
NDN_DECL(D_802ADD60_7B5700);
NDN_DECL(D_802ADDD8_7B5778);
NDN_DECL(D_802ADE20_7B57C0);
NDN_DECL(D_802ADE30_7B57D0);

#if !VERSION_PAL
#if VERSION_JP
EntityModelScript EMS_starpoint_starpoint = STANDARD_ENTITY_MODEL_SCRIPT(D_802A2B98_7B2608, RENDER_MODE_CLOUD_NO_ZCMP);
EntityModelScript EMS_starpoint_starpoints = STANDARD_ENTITY_MODEL_SCRIPT(D_802A2B98_7B2608, RENDER_MODE_CLOUD_NO_ZCMP);
#else
EntityModelScript EMS_starpoint_starpoint = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802A2158_7A9AF8), RENDER_MODE_CLOUD_NO_ZCMP);
EntityModelScript EMS_starpoint_starpoints = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802A32D8_7AAC78), RENDER_MODE_CLOUD_NO_ZCMP);
#endif
#endif
EntityModelScript EMS_starpoint_digit_0 = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802A4410_7ABDB0), RENDER_MODE_CLOUD_NO_ZCMP);
EntityModelScript EMS_starpoint_digit_1 = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802A5530_7ACED0), RENDER_MODE_CLOUD_NO_ZCMP);
EntityModelScript EMS_starpoint_digit_2 = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802A6650_7ADFF0), RENDER_MODE_CLOUD_NO_ZCMP);
EntityModelScript EMS_starpoint_digit_3 = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802A7770_7AF110), RENDER_MODE_CLOUD_NO_ZCMP);
EntityModelScript EMS_starpoint_digit_4 = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802A8890_7B0230), RENDER_MODE_CLOUD_NO_ZCMP);
EntityModelScript EMS_starpoint_digit_5 = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802A99B0_7B1350), RENDER_MODE_CLOUD_NO_ZCMP);
EntityModelScript EMS_starpoint_digit_6 = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802AAAD0_7B2470), RENDER_MODE_CLOUD_NO_ZCMP);
EntityModelScript EMS_starpoint_digit_7 = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802ABBF0_7B3590), RENDER_MODE_CLOUD_NO_ZCMP);
EntityModelScript EMS_starpoint_digit_8 = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802ACD10_7B46B0), RENDER_MODE_CLOUD_NO_ZCMP);
EntityModelScript EMS_starpoint_digit_9 = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802ADE30_7B57D0), RENDER_MODE_CLOUD_NO_ZCMP);

NDN_DECL(D_802ADF90_7B5930);
EntityModelScript EMS_starpoint_dummy = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802ADF90_7B5930), RENDER_MODE_CLOUD_NO_ZCMP);
