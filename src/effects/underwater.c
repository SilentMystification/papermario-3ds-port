#include "common.h"
#include "effects_internal.h"
#include "nu/nusys.h"
#include "nd_gtab.h"

extern Vtx D_09000428_3B9E98[];
extern Gfx D_09000528_3B9F98[];
extern Gfx D_09000570_3B9FE0[];

void underwater_init(EffectInstance* effect);
void underwater_update(EffectInstance* effect);
void underwater_render(EffectInstance* effect);
void underwater_appendGfx(void* effect);

EffectInstance* underwater_main(s32 arg0, f32 arg1, f32 arg2, f32 arg3, f32 arg4, s32 arg5) {
    EffectBlueprint bp;
    EffectInstance* effect;
    UnderwaterFXData* data;
    s32 numParts = 1;
    s32 i;
    s32 j;

    bp.init = underwater_init;
    bp.update = underwater_update;
    bp.renderScene = underwater_render;
    bp.unk_00 = 0;
    bp.renderUI = nullptr;
    bp.effectID = EFFECT_UNDERWATER;

    effect = create_effect_instance(&bp);
    effect->numParts = numParts;
    data = effect->data.underwater = general_heap_malloc(numParts * sizeof(*data));
    ASSERT(effect->data.underwater != nullptr);

    data->unk_00 = arg0;
    data->lifeTime = 0;
    if (arg5 <= 0) {
        data->timeLeft = 1000;
    } else {
        data->timeLeft = arg5;
    }
    data->waterColor.a = 0;
    data->unk_04 = arg1;
    data->unk_08 = arg2;
    data->unk_0C = arg3;
    data->unk_18 = arg4;
    data->waterColor.r = 10;
    data->waterColor.g = 110;
    data->waterColor.b = 255;
    data->unk_20.r = 190;
    data->unk_20.g = 220;
    data->unk_20.b = 255;

    for (i = 0; i < ARRAY_COUNT(data->unk_23); i++) {
        for (j = 0; j < ARRAY_COUNT(data->unk_23[0]); j++) {
            data->unk_23[i][j] = 0;
            data->unk_11A[i][j] = 0;
        }
    }

    return effect;
}

void underwater_init(EffectInstance* effect) {
}

void underwater_update(EffectInstance* effect) {
    UnderwaterFXData* data = effect->data.underwater;
    s32 timeLeft;
    s32 lifeTime;
    s32 alpha;
    f32 factor;
    s32 i;
    s32 j;

    if (effect->flags & FX_INSTANCE_FLAG_DISMISS) {
        effect->flags &= ~FX_INSTANCE_FLAG_DISMISS;
        data->timeLeft = 32;
    }

    if (data->timeLeft < 1000) {
        data->timeLeft--;
    }

    data->lifeTime++;

    if (data->timeLeft < 0) {
        remove_effect(effect);
        return;
    }

    timeLeft = data->timeLeft;
    lifeTime = data->lifeTime;

    alpha = 255;
    if (timeLeft < 32) {
        alpha = timeLeft * 8;
    }
    if (lifeTime < 16) {
        alpha = lifeTime * 16 + 15;
    }

    data->waterColor.a = (f32) alpha;
    factor = (f32) alpha / 255.0f;

    for (i = 1; i < ARRAY_COUNT(data->unk_23) - 1; i++) {
        data->unk_23[i][6] = sin_deg(-((lifeTime - i) * 20)) * -64.0f * factor;
    }

    for (i = 1; i < ARRAY_COUNT(data->unk_23) - 1; i++) {
        for (j = 1; j < ARRAY_COUNT(data->unk_23[0]) - 1; j++) {
            f32 m11 = data->unk_23[i][j] * 4.0f;
            f32 m21 = data->unk_23[i + 1][j];
            f32 m01 = data->unk_23[i - 1][j];
            f32 m12 = data->unk_23[i][j + 1];
            f32 m10 = data->unk_23[i][j - 1];
            f32 m22 = data->unk_23[i + 1][j + 1];
            f32 m02 = data->unk_23[i - 1][j + 1];
            f32 m20 = data->unk_23[i + 1][j - 1];
            f32 m00 = data->unk_23[i - 1][j - 1];
            f32 temp1 = m21 + m01 + m12 + m10 - m11;
            f32 temp2 = (m22 + m02 + m20 + m00) * 0.5;

            data->unk_11A[i][j] += temp1 + temp2 - m11 * 0.7;
            data->unk_11A[i][j] *= 0.98;

            if (timeLeft < 32) {
                data->unk_23[i][j] *= factor;
            }
        }
    }

    for (i = 1; i < ARRAY_COUNT(data->unk_23) - 1; i++) {
        for (j = 1; j < ARRAY_COUNT(data->unk_23[0]) - 1; j++) {
            data->unk_23[i][j] += data->unk_11A[i][j] * 0.02;
        }
    }
}

void underwater_render(EffectInstance* effect) {
    RenderTask renderTask;
    RenderTask* retTask;

    renderTask.appendGfx = underwater_appendGfx;
    renderTask.appendGfxArg = effect;
    renderTask.dist = 100;
    renderTask.renderMode = RENDER_MODE_CLOUD_NO_ZCMP;

    retTask = queue_render_task(&renderTask);
}

void func_E00BA618(void) {
}

/* TIDAL try 2: the mesh grid. 19 x 13 vertices, row-major (b * 19 + a), in quarter pixels (the ModifyVertex unit, x and y
 * shifted by unk_23[a][b] on y only). Texture coordinates are the undisplaced screen position: s = x * 32, t = y * 32
 * (S10.5, texel = pixel). The mesh draws into the frame buffer and samples it: the grab hold (SBLK2 B1) gives one copy.
 * z is 1/3 for eye z -320 (every vertex slot of the mesh data is -320): under the ortho near -16384, far 16384 that is -5461. */
#define TIDAL_COLS 19
#define TIDAL_ROWS 13
#define TIDAL_Z (-5461)
static Vtx sTidalGrid[TIDAL_COLS * TIDAL_ROWS] __attribute__((aligned(16)));
static int tidal_logged;

void pm_log(const char* fmt, ...);

static void tidal_grid_fill(UnderwaterFXData* data) {
    s32 a, b, xs, ys;
    Vtx* v = sTidalGrid;

    for (b = 0; b < TIDAL_ROWS; b++) {
        ys = b * 16 + 24;
        if (b == 0) {
            ys -= 4;
        } else if (b == TIDAL_ROWS - 1) {
            ys += 4;
        }
        for (a = 0; a < TIDAL_COLS; a++, v++) {
            xs = a * 16 + 16;
            if (a == 0) {
                xs -= 4;
            } else if (a == TIDAL_COLS - 1) {
                xs += 4;
            }
            v->v.ob[0] = (s16) (xs * 4);
            v->v.ob[1] = (s16) (ys * 4 + data->unk_23[a][b]);
            v->v.ob[2] = TIDAL_Z;
            v->v.flag = 0;
            v->v.tc[0] = (s16) (xs * 32);
            v->v.tc[1] = (s16) (ys * 32);
            v->v.cn[0] = 0;
            v->v.cn[1] = 0;
            v->v.cn[2] = 0;
            v->v.cn[3] = 255;
        }
    }
}

void underwater_appendGfx(void* effect) {
    UnderwaterFXData* data = ((EffectInstance*)effect)->data.underwater;
    s32 alpha = data->waterColor.a;
    Matrix4f mtx;
    s32 b, c, k, i;

    gDPPipeSync(gMainGfxPos++);
    gSPSegment(gMainGfxPos++, 0x09, VIRTUAL_TO_PHYSICAL(((EffectInstance*)effect)->shared->graphics));
#ifdef TARGET_3DS
    nd_rec_dl(D_09000528_3B9F98);
#else
    gSPDisplayList(gMainGfxPos++, D_09000528_3B9F98);
#endif

    tidal_grid_fill(data);
    if (!tidal_logged) {
        tidal_logged = 1;
        pm_log("tidal: mesh tile 0..319 x 0..239 RGBA16 line 80, grab hold 1, tc unit x32 (S10.5); v0 ob %d %d %d tc %d %d; v18 ob %d %d tc %d %d; v234 tc %d %d alpha %d",
            sTidalGrid[0].v.ob[0], sTidalGrid[0].v.ob[1], sTidalGrid[0].v.ob[2], sTidalGrid[0].v.tc[0], sTidalGrid[0].v.tc[1],
            sTidalGrid[18].v.ob[0], sTidalGrid[18].v.ob[1], sTidalGrid[18].v.tc[0], sTidalGrid[18].v.tc[1],
            sTidalGrid[234].v.tc[0], sTidalGrid[234].v.tc[1], alpha);
    }

    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, nuGfxCfb_ptr);
    gDPSetPrimColor(gMainGfxPos++, 0, 0, data->waterColor.r, data->waterColor.g, data->waterColor.b, alpha >> 1);
    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
    gDPSetCombineMode(gMainGfxPos++, PM_CC_48, PM_CC_48);
    gDPSetTextureImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, OS_K0_TO_PHYSICAL(nuGfxCfb_ptr));
    gDPSetRenderMode(gMainGfxPos++, CVG_DST_SAVE | ZMODE_OPA | FORCE_BL | G_RM_PASS, CVG_DST_SAVE | ZMODE_OPA | FORCE_BL | GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
    gDPSetTexturePersp(gMainGfxPos++, G_TP_PERSP);
    gDPSetTextureFilter(gMainGfxPos++, G_TF_BILERP);
    gPmNdGrabHold(gMainGfxPos++, 1); /* SBLK2 B1: one frame copy for the mesh */

    guOrthoF(mtx, 0.0f, 1280.0f, 960.0f, 0.0f, -16384.0f, 16384.0f, 1.0f);
    guMtxF2L(mtx, &gDisplayContext->matrixStack[gMatrixListPos]);
    gSPMatrix(gMainGfxPos++, &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    gSPClearGeometryMode(gMainGfxPos++, G_CULL_BOTH | G_LIGHTING);
    gSPSetGeometryMode(gMainGfxPos++, G_SHADE | G_SHADING_SMOOTH);
    gSPTexture(gMainGfxPos++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);

    // one tile: the whole frame (320 x 240, RGBA16, line 80 words), clamp
    gDPSetTile(
        gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b,
        80, 0x0000, G_TX_LOADTILE, 0,
        G_TX_NOMIRROR | G_TX_CLAMP, 0, G_TX_NOLOD,
        G_TX_NOMIRROR | G_TX_CLAMP, 0, G_TX_NOLOD);
    gDPLoadSync(gMainGfxPos++);
    gDPLoadTile(gMainGfxPos++, G_TX_LOADTILE, 0, 0, (SCREEN_WIDTH - 1) * 4, (240 - 1) * 4);
    gDPPipeSync(gMainGfxPos++);
    gDPSetTile(
        gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b,
        80, 0x0000, G_TX_RENDERTILE, 0,
        G_TX_NOMIRROR | G_TX_CLAMP, 0, G_TX_NOLOD,
        G_TX_NOMIRROR | G_TX_CLAMP, 0, G_TX_NOLOD);
    gDPSetTileSize(gMainGfxPos++, G_TX_RENDERTILE, 0, 0, (SCREEN_WIDTH - 1) * 4, (240 - 1) * 4);

    // cells: a load holds the 20 + k contiguous grid vertices of a row pair (k cells, k <= 12); one triangle op per cell
    for (b = 0; b < TIDAL_ROWS - 1; b++) {
        for (c = 0; c < TIDAL_COLS - 1; c += 12) {
            k = (TIDAL_COLS - 1 - c) < 12 ? (TIDAL_COLS - 1 - c) : 12;
            gSPVertex(gMainGfxPos++, &sTidalGrid[b * TIDAL_COLS + c], k + 20, 0);
            for (i = 0; i < k; i++) {
                gSP2Triangles(gMainGfxPos++, i, 20 + i, i + 1, 0, i, 19 + i, 20 + i, 0);
            }
        }
    }

    gPmNdGrabHold(gMainGfxPos++, 0);
    gSPTexture(gMainGfxPos++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
    gDPPipeSync(gMainGfxPos++);
    gSPMatrix(gMainGfxPos++, &gDisplayContext->camPerspMatrix[gCurrentCameraID], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    gDPPipeSync(gMainGfxPos++);
}
