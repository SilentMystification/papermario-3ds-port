#include "mac_01.h"
#include "model.h"
#include "nu/nusys.h"
#include "nd_geo.h"
#include "nd_val.h"
#include "nd_tx.h"
#include "nd_st.h"
#include "nd_at.h"

s32 N(CrystallBallRenderCounter) = 0;
s32 N(UnusedCrystalBallField) = 0;

void N(gfx_build_crystal_ball_pre)(void) {
    Camera* camera = &gCameras[gCurrentCameraID];
    Matrix4f sp50;
    LookAt sp90;

    N(CrystallBallRenderCounter)++;
    guLookAtHiliteF(sp50, &sp90, &gDisplayContext->hilite,
                    camera->lookAt_eye.x, camera->lookAt_eye.y, camera->lookAt_eye.z,
                    camera->lookAt_obj.x, camera->lookAt_obj.y, camera->lookAt_obj.z,
                    0.0f, 1.0f, 0.0f,
                    -50.0f, 63.0f, 95.0f,
                    31.0f, 63.0f, 95.0f,
                    64, 64);
    nd_lit_lookat(&gDisplayContext->lookAt.l[0]);
    nd_tex_hilite1(G_TX_RENDERTILE, &gDisplayContext->hilite, 64, 64);
    nd_tex_hilite2(G_TX_MIRROR, &gDisplayContext->hilite, 64, 64);
    nd_tex_on(0x1000, 0x1000, 0, G_TX_RENDERTILE, G_ON);
    nd_pso_geom_set(G_TEXTURE_GEN);
}

void N(gfx_build_crystal_ball_post)(void) {
    nd_pso_geom_clr(G_TEXTURE_GEN);
}

void N(gfx_build_inside_crystal_ball)(void) {
    f32 x, y, z, s;
    f32 f20, f22;
    f32 f0, f2;
    s32 ulx, uly;
    Camera* camera = &gCameras[gCurrentCameraID];
    Model* model = get_model_from_list_index(get_model_list_index_from_tree_index(MODEL_tama));

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
    f20 = (f32) pm_wide_draw_x(camera, (s32) f20); /* MERLON: the 2D x of the ball (wide: the 3D draw is at 1.25 x, a rectangle and the grab window at x + 40) */
    f22 += camera->viewportStartY;

    nd_pso_cyc(G_CYC_1CYCLE);
    nd_pso_rm(Z_CMP | CVG_DST_CLAMP | ZMODE_OPA | FORCE_BL | G_RM_PASS, Z_CMP | CVG_DST_CLAMP | ZMODE_OPA | FORCE_BL | GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
    nd_target_begin(osVirtualToPhysical(nuGfxZBuffer), G_IM_SIZ_16b, SCREEN_WIDTH);
    nd_pso_cc(PM_CC_PRIM_NO_ALPHA, PM_CC_PRIM_NO_ALPHA);
    nd_val_prim(0, 0, 248, 240, 240, 0);
    NDCTL_CALL(nd_nr, model->modelNode->displayData->displayList); /* NMDL7: the list call unit (nd_at.h:35) */
    nd_target_end();
    nd_pso_zsrc(G_ZS_PRIM);
    nd_val_primz(32734, 0);
    nd_pso_rm(G_RM_ZB_OPA_DECAL, G_RM_ZB_OPA_DECAL2);
    nd_pso_filt(G_TF_POINT);
    nd_pso_persp(G_TP_NONE);
    nd_tex_on(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    nd_pso_lut(G_TT_NONE);
    nd_pso_detail(G_TD_CLAMP);
    nd_pso_lod(G_TL_TILE);

    ulx = f20 / 32.0f * 32.0f - 8.0f;
    uly = f22 / 32.0f * 32.0f - 8.0f;
    if (ulx >= 0 && uly >= 0 && ulx + 40 < SCREEN_WIDTH && uly + 40 < SCREEN_HEIGHT) {
        nd_tex_load_tile(osVirtualToPhysical(nuGfxCfb_ptr), G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, SCREEN_HEIGHT, ulx, uly, ulx + 31, uly + 31, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
        nd_pso_cc(PM_CC_0E, PM_CC_0E);
        nd_rect_scis((ulx - 8) * 4, (uly - 8) * 4, (ulx + 24) * 4, (uly + 24) * 4, G_TX_RENDERTILE, (ulx & 0x1F) << 5, (uly & 0x1F) << 5, 700, 700);
    }

    nd_pso_zsrc(G_ZS_PIXEL);
    nd_pso_rm(G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
    nd_pso_cc(G_CC_SHADE, G_CC_SHADE);
    nd_pso_persp(G_TP_PERSP);
    nd_tex_on(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
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
