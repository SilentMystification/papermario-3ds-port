#include "pm_wide.h"
#include "common.h"
#include "nu/nusys.h"
#include "nd_api.h"
#include "nd_st.h"
#include "nd_val.h"
#include "nd_xf.h"
extern u8 gPmStencilId;                      /* screen_overlays.c, NATIVEAPI B2a */
extern void pm_tsten_run(const uint32_t* u); /* screen_overlays.c, NATIVEAPI B2a */

Gfx D_8014ED90[] = {
    gsSPEndDisplayList(),
};

Vp DarknessStencilViewport = {
    .vp = {
        .vscale = { 128, 128, 511, 0 },
        .vtrans = { 128, 128, 511, 0 }
    }
};

Vtx Vtx_DarknessStencil[] = {
    {{{ -16,-16, 0 }, 0, { 0x0000, 0x0000 }, { 0, 0, 0, 255 }}},
    {{{  16,-16, 0 }, 0, { 0x1000, 0x0000 }, { 0, 0, 0, 255 }}},
    {{{  16, 16, 0 }, 0, { 0x1000, 0x1000 }, { 0, 0, 0, 255 }}},
    {{{ -16, 16, 0 }, 0, { 0x0000, 0x1000 }, { 0, 0, 0, 255 }}},
};

Gfx Gfx_DarknessStencilQuad[] = {
    gsSPVertex(Vtx_DarknessStencil, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsDPPipeSync(),
    gsSPEndDisplayList(),
};

void appendGfx_draw_prev_frame_buffer(s32 x1, s32 y1, s32 x2, s32 y2, f32 alpha) {
    s32 stripY, extraY;
    s32 i;
    u16* prevGfxCfb = nullptr;

    // round the x positions
    x1 = x1 - (x1 % 4);
    x2 = x2 - (x2 % 4) + 4;
    // can only load 6 rows of the color buffer at a time: 320*6*2 = 3840 bytes of the 4096 capacity
    stripY = (y2 - y1) / 6;
    extraY = (y2 - y1) % 6;

    // get previous color buffer
    for (i = 0; i < nuGfxCfbNum; i++) {
        if (nuGfxCfb[i] == nuGfxCfb_ptr) {
            prevGfxCfb = nuGfxCfb[(i + nuGfxCfbNum - 1) % nuGfxCfbNum];
        }
    }

    nd_pso_cyc(G_CYC_1CYCLE);
    nd_pso_cc(PM_CC_10, PM_CC_10);
    nd_pso_rm(G_RM_CLD_SURF, G_RM_CLD_SURF2);
    nd_pso_dither(G_CD_DISABLE);
    nd_pso_aldith(G_AD_NOISE);
    nd_pso_filt(G_TF_POINT);
    nd_pso_persp(G_TP_NONE);
    nd_tex_on(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    nd_pso_lut(G_TT_NONE);
    nd_pso_detail(G_TD_CLAMP);
    nd_pso_lod(G_TL_TILE);
    nd_val_prim(0, 0, 255, 255, 255, alpha);

    for (i = 0; i < stripY; i++) {
        gDPLoadTextureTile(gMainGfxPos++, osVirtualToPhysical(prevGfxCfb), G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, 6,
                           x1, y1 + i * 6, x2 - 1, y1 + i * 6 + 5, 0,
                           G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(gMainGfxPos++, x1 * 4, (y1 + i * 6) * 4, x2 * 4, (y1 + i * 6 + 6) * 4,
                            G_TX_RENDERTILE, x1 * 32, (y1 + i * 6) * 32, 1024, 1024);
    }

    if (extraY != 0) {
        gDPLoadTextureTile(gMainGfxPos++, osVirtualToPhysical(prevGfxCfb), G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, extraY,
                           x1, y1 + i * 6, x2 - 1, y1 + i * 6 + extraY - 1, 0,
                           G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(gMainGfxPos++, x1 * 4, (y1 + i * 6) * 4, x2 * 4, (y1 + i * 6 + extraY) * 4,
                            G_TX_RENDERTILE, x1 * 32, (y1 + i * 6) * 32, 1024, 1024);
    }
}

void draw_prev_frame_buffer_at_screen_pos(s32 x1, s32 y1, s32 x2, s32 y2, f32 alpha) {
    s32 minX;
    s32 minY;
    s32 maxX;
    s32 maxY;

    maxX = x2;
    maxY = y2;
    if (maxX < x1) {
        minX = maxX;
        maxX = x1;
    } else {
        minX = x1;
    }
    if (maxY < y1) {
        minY = maxY;
        maxY = y1;
    } else {
        minY = y1;
    }
    if (minX < maxX && minY < maxY) {
        if (minX < 0) {
            minX = 0;
        }
        if (minY < 0) {
            minY = 0;
        }
        if (maxX < 0) {
            maxX = 0;
        }
        if (maxY < 0) {
            maxY = 0;
        }
        if (minX >= SCREEN_WIDTH) {
            minX = SCREEN_WIDTH - 1;
        }
        if (minY >= SCREEN_HEIGHT) {
            minY = SCREEN_HEIGHT - 1;
        }
        if (maxX >= SCREEN_WIDTH) {
            maxX = SCREEN_WIDTH - 1;
        }
        if (maxY >= SCREEN_HEIGHT) {
            maxY = SCREEN_HEIGHT - 1;
        }
        if (minX != maxX && minY != maxY) {
            appendGfx_draw_prev_frame_buffer(minX, minY, maxX, maxY, alpha);
        }
    }
}

void appendGfx_darkness_stencil(bool isWorld, s32 posX, s32 posY, f32 alpha, f32 progress) {
    Camera* camera = &gCameras[gCurrentCameraID];
    f32 texScale, f24;
    f32 sShift;
    s32 rectLeft, rectRight;
    s32 i;

    if (alpha == 0.0f) {
        return;
    }

    if (!isWorld) {
        texScale = (255.0f - alpha) * 10.0f / 255.0f + 0.14;
    } else {
        texScale = 0.5f;
        texScale = (255.0f - alpha) * 9.5f / 255.0f + texScale;
        posX += (camera->viewportW / 2 - posX) * (255.0f - alpha) / 255.0f;
        posY += (camera->viewportH / 2 - posY) * (255.0f - alpha) / 255.0f;
    }

    guOrtho(&gDisplayContext->matrixStack[gMatrixListPos], -16.0f, 0.0f, 0.0f, 16.0f, -1000.0f, 1000.0f, 1.0f);
    nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    nd_pso_aldith(G_AD_PATTERN);
    nd_pso_filt(G_TF_POINT);
    nd_pso_persp(G_TP_PERSP);
    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_32b, 64, osVirtualToPhysical(nuGfxZBuffer));
    nd_pso_scis(G_SC_NON_INTERLACE, 0, 0, 64, 64);
    gSPViewport(gMainGfxPos++, &DarknessStencilViewport);
    nd_pso_cc(PM_CC_CONST_1, PM_CC_CONST_1);
    guRotate(&gDisplayContext->matrixStack[gMatrixListPos], 0.0f, 0.0f, 0.0f, 1.0f);
    nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(gMainGfxPos++, Gfx_DarknessStencilQuad);
    nd_xf_pop(G_MTX_MODELVIEW);
    nd_pso_cc(PM_CC_56, PM_CC_56);

    for (i = 0; i < 2; i++) {
        f32 f20;
        f32 f2;
        s32 s0 = 255;

        if (i == 0) {
            f20 = 0.9 - sin_deg(gGameStatusPtr->frameCounter) * 0.05;
        } else {
            f20 = 0.9 - sin_deg(gGameStatusPtr->frameCounter * 2 + i * 40) * 0.07;
        }
        f2 = 0;
        if (s0 > 255) {
            s0 = 255;
        }

        nd_val_prim(0, 0, 255, 255, 255, s0);
        guRotate(&gDisplayContext->matrixStack[gMatrixListPos], i * 70 + f2, 0.0f, 0.0f, 1.0f);
        nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        guScale(&gDisplayContext->matrixStack[gMatrixListPos], f20, f20, 1.0f);
        nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
        gSPDisplayList(gMainGfxPos++, Gfx_DarknessStencilQuad);
        nd_xf_pop(G_MTX_MODELVIEW);
    }

    nd_pso_persp(G_TP_NONE);
    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_I, G_IM_SIZ_8b, 64, osVirtualToPhysical(nuGfxZBuffer));
    nd_pso_scis(G_SC_NON_INTERLACE, 0, 0, 64, 64);
    nd_pso_cc(PM_CC_0F, PM_CC_0F);

    for (i = 0; i < 4; i++) {
        gDPLoadTextureTile(gMainGfxPos++, osVirtualToPhysical(&nuGfxZBuffer[i * 2048]), G_IM_FMT_RGBA, G_IM_SIZ_32b, 64, 16, 0, 0, 63, 15, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        gSPTextureRectangle(gMainGfxPos++, 0, (i * 16) * 4, 64 * 4, (i * 16 + 16) * 4, G_TX_RENDERTILE, 0, 0, 0x400, 0x400);
    }

    gDPLoadTextureTile(gMainGfxPos++, osVirtualToPhysical(nuGfxZBuffer), G_IM_FMT_I, G_IM_SIZ_8b, 64, 64, 0, 0, 63, 63,
                       0, G_TX_MIRROR | G_TX_CLAMP, G_TX_MIRROR | G_TX_CLAMP, 6, 6, 15, 15);
    nd_val_tile_size(G_TX_RENDERTILE, 0, 0, 0x01FC, 0x01FC);

    nd_pso_cyc(G_CYC_FILL);
    nd_pso_rm(G_RM_NOOP, G_RM_NOOP2);
    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_32b, 64, osVirtualToPhysical(nuGfxZBuffer));
    gDPSetFillColor(gMainGfxPos++, GPACK_ZDZ(G_MAXFBZ, 0)<<16 | GPACK_ZDZ(G_MAXFBZ, 0));
    gDPFillRectangle(gMainGfxPos++, 0, 0, 63, 63);

    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, osVirtualToPhysical(nuGfxCfb_ptr));
    nd_pso_scis(G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    nd_pso_cyc(G_CYC_1CYCLE);
    nd_pso_dither(G_CD_MAGICSQ);
    nd_pso_aldith(G_AD_PATTERN);
    nd_pso_filt(G_TF_AVERAGE);
    nd_pso_rm(G_RM_CLD_SURF, G_RM_CLD_SURF2);

    if (alpha != 255.0f || isWorld == 1) {
        nd_pso_cc(PM_CC_57, PM_CC_57);
    } else {
        nd_pso_cc(PM_CC_58, PM_CC_58);
    }
    alpha *= 0.5f;
    nd_val_prim(0, 0, 0, 0, 0, alpha * 0.5f * progress / 255.0f);
    nd_val_env(255, 255, 255, (255.0f - alpha * 0.5f) * progress / 255.0f);

    /* F07-6: with wide the rect reaches both screen edges. s moves by 32 / texScale per pixel, so it shifts with the left edge. */
    rectLeft = camera->viewportStartX;
    rectRight = camera->viewportStartX + camera->viewportW;
    if (pm_wide && (gCurrentCameraID == CAM_DEFAULT || gCurrentCameraID == CAM_BATTLE)) {
        rectLeft = pm_wide_left(0);
        rectRight = pm_wide_right(SCREEN_WIDTH);
    }
    sShift = (rectLeft - camera->viewportStartX) * 32.0f / texScale;

    if (!isWorld) {
        gSPTextureRectangle(gMainGfxPos++,
                            rectLeft * 4, camera->viewportStartY * 4,
                            rectRight * 4, (camera->viewportStartY + camera->viewportH) * 4,
                            G_TX_RENDERTILE,
                            (12 - posX) * 32.0f / texScale + 16.0f + 1024.0f + sShift, (19 - posY) * 32.0f / texScale + 16.0f + 1024.0f,
                            1024.0f / texScale, 1024.0f / texScale);
    } else {
        gSPTextureRectangle(gMainGfxPos++,
                            rectLeft * 4, camera->viewportStartY * 4,
                            rectRight * 4, (camera->viewportStartY + camera->viewportH) * 4,
                            G_TX_RENDERTILE,
                            (9 - posX) * 32.0f / texScale + 1024.0f + sShift, (32 - posY) * 32.0f / texScale + 1024.0f,
                            1024.0f / texScale, 1024.0f / texScale);
    }
}

void appendGfx_screen_transition_stencil(s32 arg0, s32 arg1, f32 progress, s32 primR, s32 primG, s32 primB, s32 primA, s32 camID) {
    s32 x1, y1, x2, y2, t5, t6;
    s32 rectLeft, rectRight; /* F07-3: the circle rect reaches both screen edges with wide */
    f32 texScale;

    if (camID >= 0) {
        x1 = gCameras[camID].viewportStartX;
        y1 = gCameras[camID].viewportStartY;
        x2 = gCameras[camID].viewportStartX + gCameras[camID].viewportW;
        y2 = gCameras[camID].viewportStartY + gCameras[camID].viewportH;
        if (pm_wide && (camID == CAM_DEFAULT || camID == CAM_BATTLE)) {
            x1 = 0;
            x2 = SCREEN_WIDTH;
        }
        rectLeft = (pm_wide && (camID == CAM_DEFAULT || camID == CAM_BATTLE)) ? pm_wide_left(0) : x1;
        rectRight = (pm_wide && (camID == CAM_DEFAULT || camID == CAM_BATTLE)) ? pm_wide_right(SCREEN_WIDTH) : x2;
        t5 = rectLeft;
        t6 = y1;
    } else {
        x1 = 0;
        y1 = 0;
        x2 = SCREEN_WIDTH;
        y2 = SCREEN_HEIGHT;
        rectLeft = pm_wide_left(0);
        rectRight = pm_wide_right(SCREEN_WIDTH);
        t5 = rectLeft;
        t6 = 0;
    }

    if (progress == 0.0f) {
        return;
    }

    nd_pso_scis(G_SC_NON_INTERLACE, x1, y1, x2, y2);
    texScale = (255.0f - progress) * 10.5f / 255.0f + 0.09; // range from
    /* NATIVEAPI B2a (S1TSTEN): the combine, the prim colour and the rectangle are one native record; pm_tsten_run (screen_overlays.c) draws it.
     * The record words: w0, w1 (the G_TEXRECT words), hs, hd (the RDPHALF words), prim, full | id << 1 (id: gPmStencilId, set by the caller). */
    {
        const u32 rec[6] = {
            ND_W0_TR(G_TEXRECT, rectRight * 4, y2 * 4),
            ND_W1_TR(G_TX_RENDERTILE, rectLeft * 4, y1 * 4),
            ND_TR_H1((t5 - arg0) * 32.0f / texScale + 16.0f + 1024.0f, (t6 - arg1) * 32.0f / texScale + 16.0f + 1024.0f),
            ND_TR_H2(1024.0f / texScale, 1024.0f / texScale),
            ((u32)(u8)primR << 24) | ((u32)(u8)primG << 16) | ((u32)(u8)primB << 8) | (u32)(u8)primA,
            (u32)(progress == 255.0f) | ((u32)gPmStencilId << 1),
        };
        nd_rec_napi(pm_tsten_run, rec, 3u);
    }
}
