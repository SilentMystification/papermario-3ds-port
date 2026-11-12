#include "mac_01.h"
#include "model.h"
#include "nu/nusys.h"
#ifdef TARGET_3DS
#include "nd_scomp.h"
/* S6M1: the crystal ball custom gfx builders as compiled state ranges (nd_scomp.h). The units in the order of the N64 code in each builder
 * (PipeSync lines left out). The value units are placeholders of their kind; the record carries the per-frame values. */
static const Gfx cb_pre_u[] = {
    gsSPLookAtX(0),
    gsSPLookAtY(0),
    gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 0, 0),
    gsDPSetTileSize(G_TX_MIRROR, 0, 0, 0, 0),
    gsSPTexture(0x1000, 0x1000, 0, G_TX_RENDERTILE, G_ON),
    gsSPSetGeometryMode(G_TEXTURE_GEN),
};
static NdSDef cb_pre = { cb_pre_u, sizeof cb_pre_u / sizeof cb_pre_u[0] };
static const Gfx cb_post_u[] = {
    gsSPClearGeometryMode(G_TEXTURE_GEN),
};
static NdSDef cb_post = { cb_post_u, sizeof cb_post_u / sizeof cb_post_u[0] };
/* S6VORD: a value unit between two static units is refused: cb_r1 and cb_r2 are each split at their value unit (the record runs the halves in order). */
static const Gfx cb_r1a_u[] = {
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetRenderMode(Z_CMP | CVG_DST_CLAMP | ZMODE_OPA | FORCE_BL | G_RM_PASS, Z_CMP | CVG_DST_CLAMP | ZMODE_OPA | FORCE_BL | GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1)),
    gsDPSetColorImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, 0),
};
static NdSDef cb_r1a = { cb_r1a_u, sizeof cb_r1a_u / sizeof cb_r1a_u[0] };
static const Gfx cb_r1b_u[] = {
    gsDPSetCombineMode(PM_CC_PRIM_NO_ALPHA, PM_CC_PRIM_NO_ALPHA),
    gsDPSetPrimColor(0, 0, 0, 0, 0, 0),
};
static NdSDef cb_r1b = { cb_r1b_u, sizeof cb_r1b_u / sizeof cb_r1b_u[0] };
static const Gfx cb_r2a_u[] = {
    gsDPSetColorImage(G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, 0),
    gsDPSetDepthSource(G_ZS_PRIM),
    gsDPSetPrimDepth(0, 0),
};
static NdSDef cb_r2a = { cb_r2a_u, sizeof cb_r2a_u / sizeof cb_r2a_u[0] };
static const Gfx cb_r2b_u[] = {
    gsDPSetRenderMode(G_RM_ZB_OPA_DECAL, G_RM_ZB_OPA_DECAL2),
    gsDPSetTextureFilter(G_TF_POINT),
    gsDPSetTexturePersp(G_TP_NONE),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureLOD(G_TL_TILE),
};
static NdSDef cb_r2b = { cb_r2b_u, sizeof cb_r2b_u / sizeof cb_r2b_u[0] };
static const Gfx cb_r3_u[] = {
    gsDPSetDepthSource(G_ZS_PIXEL),
    gsDPSetRenderMode(G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF),
};
static NdSDef cb_r3 = { cb_r3_u, sizeof cb_r3_u / sizeof cb_r3_u[0] };
#endif

s32 N(CrystallBallRenderCounter) = 0;
s32 N(UnusedCrystalBallField) = 0;

void N(gfx_build_crystal_ball_pre)(void) {
    Camera* camera = &gCameras[gCurrentCameraID];
    Matrix4f sp50;
    LookAt sp90;
#ifdef TARGET_3DS
    Gfx v[4];
    Gfx* p = v;
#endif

    N(CrystallBallRenderCounter)++;
    guLookAtHiliteF(sp50, &sp90, &gDisplayContext->hilite,
                    camera->lookAt_eye.x, camera->lookAt_eye.y, camera->lookAt_eye.z,
                    camera->lookAt_obj.x, camera->lookAt_obj.y, camera->lookAt_obj.z,
                    0.0f, 1.0f, 0.0f,
                    -50.0f, 63.0f, 95.0f,
                    31.0f, 63.0f, 95.0f,
                    64, 64);
#ifdef TARGET_3DS
    gSPLookAtX(p++, &gDisplayContext->lookAt.l[0]);
    gSPLookAtY(p++, &gDisplayContext->lookAt.l[1]);
    gDPSetHilite1Tile(p++, G_TX_RENDERTILE, &gDisplayContext->hilite, 64, 64);
    gDPSetHilite2Tile(p++, G_TX_MIRROR, &gDisplayContext->hilite, 64, 64);
    nd_scomp_rec(&cb_pre, v, (unsigned)(p - v));
#else
    gSPLookAtX(gMainGfxPos++, &gDisplayContext->lookAt.l[0]);
    gSPLookAtY(gMainGfxPos++, &gDisplayContext->lookAt.l[1]);
    gDPSetHilite1Tile(gMainGfxPos++, G_TX_RENDERTILE, &gDisplayContext->hilite, 64, 64);
    gDPSetHilite2Tile(gMainGfxPos++, G_TX_MIRROR, &gDisplayContext->hilite, 64, 64);
    gSPTexture(gMainGfxPos++, 0x1000, 0x1000, 0, G_TX_RENDERTILE, G_ON);
    gSPSetGeometryMode(gMainGfxPos++, G_TEXTURE_GEN);
#endif
}

void N(gfx_build_crystal_ball_post)(void) {
#ifdef TARGET_3DS
    nd_scomp_rec(&cb_post, NULL, 0);
#else
    gSPClearGeometryMode(gMainGfxPos++, G_TEXTURE_GEN);
#endif
    gSPEndDisplayList(gMainGfxPos++);
}

void N(gfx_build_inside_crystal_ball)(void) {
    f32 x, y, z, s;
    f32 f20, f22;
    f32 f0, f2;
    s32 ulx, uly;
    Camera* camera = &gCameras[gCurrentCameraID];
    Model* model = get_model_from_list_index(get_model_list_index_from_tree_index(MODEL_tama));
#ifdef TARGET_3DS
    Gfx v1[2];
    Gfx v2[2];
    Gfx* p;
#endif

    transform_point(camera->mtxPerspective, model->center.x, model->center.y, model->center.z, 1.0f, &x, &y, &z, &s);
    s = 1.0f / s;
    x *= s;
    y *= -s;
    z = (z * s + 1.0f) * 0.5;

    if (!(z > 0.0f)) {
        return;
    }

    f20 = (x * camera->viewportW + camera->viewportW) * 0.5f;
    f22 = (y * camera->viewportH + camera->viewportH) * 0.5f;
    f0 = f20;
    f2 = f22;

    if (f20 < 0.0f) {
        f20 = 0.0f;
    }
    if (f0 < 4.0f) {
        f0 = 4.0f;
    }

    if (f22 < 0.0f) {
        f22 = 0.0f;
    }
    if (f2 < 4.0f) {
        f2 = 4.0f;
    }

    if (f20 >= camera->viewportW - 4) {
        f20 = camera->viewportW - 5;
    }
    if (f0 >= camera->viewportW - 4) {
        f0 = camera->viewportW - 5;
    }
    if (f22 >= camera->viewportH - 4) {
        f22 = camera->viewportH - 5;
    }
    if (f2 >= camera->viewportH - 4) {
        f2 = camera->viewportH - 5;
    }

    f20 += camera->viewportStartX;
#ifdef TARGET_3DS
    f20 = (f32) pm_wide_draw_x(camera, (s32) f20); /* MERLON: the 2D x of the ball (wide: the 3D draw is at 1.25 x, a rectangle and the grab window at x + 40) */
#endif
    f22 += camera->viewportStartY;

#ifdef TARGET_3DS
    p = v1;
    gDPSetColorImage(p++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, osVirtualToPhysical(nuGfxZBuffer));
    gDPSetPrimColor(p++, 0, 0, 248, 240, 240, 0);
    nd_scomp_rec(&cb_r1a, v1, 1);
    nd_scomp_rec(&cb_r1b, v1 + 1, 1);
#else
    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
    gDPSetRenderMode(gMainGfxPos++, Z_CMP | CVG_DST_CLAMP | ZMODE_OPA | FORCE_BL | G_RM_PASS, Z_CMP | CVG_DST_CLAMP | ZMODE_OPA | FORCE_BL | GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, osVirtualToPhysical(nuGfxZBuffer));
    gDPSetCombineMode(gMainGfxPos++, PM_CC_PRIM_NO_ALPHA, PM_CC_PRIM_NO_ALPHA);
    gDPSetPrimColor(gMainGfxPos++, 0, 0, 248, 240, 240, 0);
    gDPPipeSync(gMainGfxPos++);
#endif
    gSPDisplayList(gMainGfxPos++, model->modelNode->displayData->displayList);
#ifdef TARGET_3DS
    p = v2;
    gDPSetColorImage(p++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, osVirtualToPhysical(nuGfxCfb_ptr));
    gDPSetPrimDepth(p++, 32734, 0);
    nd_scomp_rec(&cb_r2a, v2, 2);
    nd_scomp_rec(&cb_r2b, NULL, 0);
#else
    gDPPipeSync(gMainGfxPos++);
    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, osVirtualToPhysical(nuGfxCfb_ptr));
    gDPSetDepthSource(gMainGfxPos++, G_ZS_PRIM);
    gDPSetPrimDepth(gMainGfxPos++, 32734, 0);
    gDPSetRenderMode(gMainGfxPos++, G_RM_ZB_OPA_DECAL, G_RM_ZB_OPA_DECAL2);
    gDPSetTextureFilter(gMainGfxPos++, G_TF_POINT);
    gDPSetTexturePersp(gMainGfxPos++, G_TP_NONE);
    gSPTexture(gMainGfxPos++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureLUT(gMainGfxPos++, G_TT_NONE);
    gDPSetTextureDetail(gMainGfxPos++, G_TD_CLAMP);
    gDPSetTextureLOD(gMainGfxPos++, G_TL_TILE);
    gDPPipeSync(gMainGfxPos++);
#endif

    ulx = f20 / 32.0f * 32.0f - 8.0f;
    uly = f22 / 32.0f * 32.0f - 8.0f;
    if (ulx >= 0 && uly >= 0 && ulx + 40 < SCREEN_WIDTH && uly + 40 < SCREEN_HEIGHT) {
        gDPLoadTextureTile(gMainGfxPos++, osVirtualToPhysical(nuGfxCfb_ptr), G_IM_FMT_RGBA, G_IM_SIZ_16b,
                           SCREEN_WIDTH, SCREEN_HEIGHT, ulx, uly, ulx + 31, uly + 31, 0,
                           G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
        gDPSetCombineMode(gMainGfxPos++, PM_CC_0E, PM_CC_0E);
        gSPScisTextureRectangle(gMainGfxPos++, (ulx - 8) * 4, (uly - 8) * 4, (ulx + 24) * 4, (uly + 24) * 4,
                                G_TX_RENDERTILE, (ulx & 0x1F) << 5, (uly & 0x1F) << 5, 700, 700);
    }

#ifdef TARGET_3DS
    nd_scomp_rec(&cb_r3, NULL, 0);
#else
    gDPPipeSync(gMainGfxPos++);
    gDPSetDepthSource(gMainGfxPos++, G_ZS_PIXEL);
    gDPSetRenderMode(gMainGfxPos++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
    gDPSetCombineMode(gMainGfxPos++, G_CC_SHADE, G_CC_SHADE);
    gDPSetTexturePersp(gMainGfxPos++, G_TP_PERSP);
    gSPTexture(gMainGfxPos++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
#endif
}

EvtScript N(EVS_SetupCrystalBallGfx) = {
    Wait(1)
    Call(SetCustomGfxBuilders, CUSTOM_GFX_2, Ref(N(gfx_build_crystal_ball_pre)), Ref(N(gfx_build_crystal_ball_post)))
    Call(SetModelCustomGfx, MODEL_mirrorball, CUSTOM_GFX_2, -1)
    Call(EnableModel, MODEL_mirrorball, false)
    Call(SetCustomGfxBuilders, CUSTOM_GFX_3, Ref(N(gfx_build_inside_crystal_ball)), nullptr)
    Call(SetModelCustomGfx, MODEL_tama, CUSTOM_GFX_3, -1)
    Call(SetModelFlags, MODEL_tama, MODEL_FLAG_BILLBOARD, true)
    Call(SetModelFlags, MODEL_ohosi, MODEL_FLAG_BILLBOARD, true)
    Return
    End
};
