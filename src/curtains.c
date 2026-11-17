#include "common.h"
#include "nd_gtab.h"
ND_PSO_DESC(pe_curtains_151, 0xfcffffff, 0xfffe793c, 0x1e182c00, 0x0f0a4000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0); /* ENTCAPR2: the first of 3 captured word sets; TheaterFloorGfx sets the combine and render mode before its first draw */
#include "nd_val.h"
#include "nd_xf.h"
#include "nd_geo.h"
extern int pm_wide; /* F07-4 */
#include "theater/walls.png.h"
#include "theater/curtains.png.h"
#include "theater/floor.png.h"
#include "ui/no_controller.png.h"
#include "ld_addrs.h"
#include "nu/nusys.h"
#include "game_modes.h"
#include "include_asset.h"

INCLUDE_IMG("theater/walls.png", theater_walls_png);

NDN_DECL(D_800746E0);

INCLUDE_IMG("theater/curtains.png", theater_curtains_png);

NDN_DECL(D_80075730);

INCLUDE_IMG("theater/floor.png", theater_floor_png);
INCLUDE_PAL("theater/floor.pal", theater_floor_pal);

NDN_DECL(D_800758A0);

#include "theater/left_wall_vtx.vtx.inc.c"
#include "theater/right_wall_vtx.vtx.inc.c"
#include "theater/floor_vtx.vtx.inc.c"
#include "theater/left_inset_shadow_vtx.vtx.inc.c"
#include "theater/right_inset_shadow_vtx.vtx.inc.c"
#include "theater/curtain_vtx.vtx.inc.c"
#include "theater/wall_shadows_vtx.vtx.inc.c"

NDN_DECL(TheaterWallShadowsGfx);

NDN_DECL(TheaterCurtainGfx);

NDN_DECL(TheaterInsetShadowsGfx);

NDN_DECL(TheaterFloorGfx);

NDN_DECL(TheaterRightWallGfx);

NDN_DECL(TheaterLeftWallGfx);

NDN_DECL(TheaterGfx);

Vp TheaterViewport = {
    {
        {(SCREEN_WIDTH/2)*4, (SCREEN_HEIGHT/2)*4, 0x200 - 1, 0},
        {(SCREEN_WIDTH/2)*4, (SCREEN_HEIGHT/2)*4, 0x200 - 1, 0},
    }
};

Gfx TheaterInitGfx[] = {
    gsSPViewport(&TheaterViewport),
    gsDPSetCycleType(G_CYC_2CYCLE),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureConvert(G_TC_FILT),
    gsDPSetCombineKey(G_CK_NONE),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, 320, 240),
    gsDPSetColorDither(G_CD_MAGICSQ),
    gsDPSetAlphaDither(G_AD_PATTERN),
    gsSPClearGeometryMode(G_ZBUFFER | G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN |
                          G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH | G_CLIPPING | 0x0040F9FA),
    gsSPSetGeometryMode(G_SHADE | G_SHADING_SMOOTH),
    gsSPPerspNormalize(0x0014),
    gsSPEndDisplayList(),
};

#if VERSION_PAL
BSS IMG_BIN noControllerImgBuf[0x1000] ALIGNED(16);
#define ui_no_controller_png noControllerImgBuf
#else
INCLUDE_IMG("ui/no_controller.png", ui_no_controller_png);
#endif

NDN_DECL(NoControllerSetupTexGfx);

NDN_DECL(NoControllerGfx);

BSS f32 gCurtainScale;
BSS f32 gCurtainScaleGoal;
BSS f32 gCurtainFade;
BSS f32 gCurtainFadeGoal;
BSS UNK_FUN_PTR(gCurtainDrawCallback);
#if VERSION_PAL
BSS s32 D_PAL_8009A204;
BSS s32 D_PAL_8009A208;
#endif
BSS Mtx D_8009BAA8[2];

void initialize_curtains(void) {
    gCurtainDrawCallback = nullptr;
    gCurtainScale = 2.0f;
    gCurtainScaleGoal = 2.0f;
    gCurtainFade = 0.0f;
    gCurtainFadeGoal = 0.0f;
#if VERSION_PAL
    D_PAL_8009A204 = 6;
    D_PAL_8009A208 = 0;
#endif
}

void update_curtains(void) {
}

#define UI_NO_CONTROLLER_SIZE (ui_no_controller_png_width * ui_no_controller_png_height)

void render_curtains(void) {
    if (gCurtainScaleGoal != gCurtainScale) {
        gCurtainScale += (gCurtainScaleGoal - gCurtainScale) * 0.1;
    }

    if (gCurtainFadeGoal != gCurtainFade) {
        gCurtainFade += (gCurtainFadeGoal - gCurtainFade) * 0.03;
    }

    if (gCurtainScale < 1.9) {
        Matrix4f m;
        f32 scale;
        s8 rgb;

        nd_target_end();
        gSPDisplayList(gMainGfxPos++, TheaterInitGfx);

        guFrustumF(m, -80.0f * (pm_wide ? 1.25f : 1.0f), 80.0f * (pm_wide ? 1.25f : 1.0f), -60.0f, 60.0f, 160.0f, 640.0f, 1.0f);
        guMtxF2L(m, &D_8009BAA8[0]);

        nd_xf_push(&D_8009BAA8[0], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);

        scale = gCurtainScale - 0.01;
        if (scale < 1.0f) {
            scale = 1.0f;
        }

        guPositionF(m, 0.0f, 0.0f, 0.0f, scale * 0.1, 0.0f, 0.0f, -320.0f);

        guMtxF2L(m, &D_8009BAA8[1]);

        nd_xf_push(&D_8009BAA8[1], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        rgb = 255.0f - (gCurtainFade * 255.0f);
        nd_val_prim(0, 0, rgb, rgb, rgb, 255);
        nd_draw_list(NDN(TheaterGfx), pe_curtains_151);
        nd_xf_pop(G_MTX_MODELVIEW);
    }

    if (gCurtainDrawCallback != nullptr) {
        gCurtainDrawCallback();
    }

    if (!(gGameStatusPtr->contBitPattern & 1)) {
        if ((get_game_mode() == GAME_MODE_INTRO)
                || (get_game_mode() == GAME_MODE_TITLE_SCREEN)
                || (gGameStatusPtr->demoState != DEMO_STATE_NONE)) {
            s32 alpha = ((gGameStatusPtr->frameCounter) % 0x18) << 5;

            if (alpha > 255) {
                alpha = 255;
            }

#if VERSION_PAL
            if (alpha == 0) {
                D_PAL_8009A204 = 6;
            }

            if (D_PAL_8009A204 == 0) {
#endif
                gSPDisplayList(gMainGfxPos++, TheaterInitGfx);
                nd_draw_list(NDN(NoControllerSetupTexGfx), ND_SELF);
                nd_val_prim(0, 0, 0xFF, 0x20, 0x10, alpha);
                nd_draw_list(NDN(NoControllerGfx), ND_SELF);
#if VERSION_PAL
            }
            
            if (D_PAL_8009A204 == 3) {
                u8* dmaStart = ui_no_controller_ROM_START + (D_PAL_8009A208 / 2) * UI_NO_CONTROLLER_SIZE;
                u8* dmaEnd = ui_no_controller_ROM_START + (D_PAL_8009A208 / 2) * UI_NO_CONTROLLER_SIZE + UI_NO_CONTROLLER_SIZE;
                dma_copy(dmaStart, dmaEnd, &noControllerImgBuf);

                D_PAL_8009A208++;
                if (D_PAL_8009A208 >= 8) {
                    D_PAL_8009A208 = 0;
                }
            }
            if (D_PAL_8009A204 != 0) {
                D_PAL_8009A204--;
            }
#endif
        }
    }
}

void set_curtain_scale_goal(f32 scale) {
    gCurtainScaleGoal = scale;
}

void set_curtain_scale(f32 scale) {
    gCurtainScaleGoal = scale;
    gCurtainScale = scale;
}

void set_curtain_draw_callback(UNK_FUN_PTR(callback)) {
    gCurtainDrawCallback = callback;
}

void set_curtain_fade_goal(f32 fade) {
    gCurtainFadeGoal = fade;
}

void set_curtain_fade(f32 fade) {
    gCurtainFadeGoal = fade;
    gCurtainFade = fade;
}
