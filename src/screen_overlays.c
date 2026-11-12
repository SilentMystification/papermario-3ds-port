#include "common.h"
#include "overlay.h"
#include "include_asset.h"
#include "nd_api.h"
#include "pm_gpu.h"

extern s32 pm_wide_cam(Camera* c);

#define OVERLAY_RENDER_OFF (-1.0f)

BSS s32 screen_overlay_frontType;
BSS f32 screen_overlay_frontProgress;
BSS s32 screen_overlay_backType;
BSS f32 screen_overlay_backProgress;
BSS s32 D_80156910;
ScreenOverlay ScreenOverlays[2];

ScreenTransition CurrentScreenTransition = TRANSITION_END_DEMO_SCENE_BLACK;

// padding?
s32 D_8014C6F4[] = { 0x00000000, 0x00000000, 0x00000000 };

INCLUDE_IMG("ui/stencil/star.png", ui_stencil_star_png);

INCLUDE_IMG("ui/stencil/mario.png", ui_stencil_mario_png);

INCLUDE_IMG("ui/stencil/sharp_circle.png", ui_stencil_sharp_circle_png);

INCLUDE_IMG("ui/stencil/blurry_circle.png", ui_stencil_blurry_circle_png);

#include "vtx/stencil1.vtx.inc.c"

Gfx Gfx_LoadStencilTex_CommonParams[] = {
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureFilter(G_TF_AVERAGE),
    gsDPSetTextureConvert(G_TC_FILT),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetColorDither(G_CD_MAGICSQ),
    gsDPSetAlphaDither(G_AD_PATTERN),
    gsDPSetCombineMode(PM_CC_3A, PM_CC_3A),
    gsDPSetRenderMode(G_RM_CLD_SURF, G_RM_CLD_SURF2),
    gsSPClearGeometryMode(G_ZBUFFER | G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH | G_CLIPPING | 0x0040F9FA),
    gsSPSetGeometryMode(G_SHADE | G_SHADING_SMOOTH),
    gsSPEndDisplayList()
};

Gfx Gfx_LoadStencilTex_SharpCircle[] = {
    gsSPDisplayList(Gfx_LoadStencilTex_CommonParams),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPLoadTextureTile_4b(ui_stencil_sharp_circle_png, G_IM_FMT_I, 32, 0, 0, 0, 31, 31, 0, G_TX_MIRROR | G_TX_CLAMP, G_TX_MIRROR | G_TX_CLAMP, 5, 5, G_TX_NOLOD, G_TX_NOLOD),
    gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 0x00FC, 0x00FC),
    gsSPEndDisplayList()
};

Gfx Gfx_LoadStencilTex_Mario[] = {
    gsSPDisplayList(Gfx_LoadStencilTex_CommonParams),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPLoadTextureTile_4b(ui_stencil_mario_png, G_IM_FMT_I, 64, 0, 0, 0, 63, 63, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 6, 6, G_TX_NOLOD, G_TX_NOLOD),
    gsSPEndDisplayList()
};

Gfx Gfx_LoadStencilTex_Star[] = {
    gsSPDisplayList(Gfx_LoadStencilTex_CommonParams),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPLoadTextureTile_4b(ui_stencil_star_png, G_IM_FMT_I, 32, 0, 0, 0, 31, 63, 0, G_TX_MIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 5, 6, G_TX_NOLOD, G_TX_NOLOD),
    gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 0x00FC, 0x00FC),
    gsSPEndDisplayList(),
};

Gfx Gfx_LoadStencilTex_BlurryCircle[] = {
    gsSPDisplayList(Gfx_LoadStencilTex_CommonParams),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetTextureImage(G_IM_FMT_I, G_IM_SIZ_8b, 64, ui_stencil_blurry_circle_png),
    gsDPSetTile(G_IM_FMT_I, G_IM_SIZ_8b, 8, 0x0000, G_TX_LOADTILE, 0, G_TX_MIRROR | G_TX_CLAMP, 6, G_TX_NOLOD, G_TX_MIRROR | G_TX_CLAMP, 6, G_TX_NOLOD),
    gsDPLoadSync(),
    gsDPLoadTile(G_TX_LOADTILE, 0, 0, 0x00FC, 0x00FC),
    gsDPPipeSync(),
    gsDPSetTile(G_IM_FMT_I, G_IM_SIZ_8b, 8, 0x0000, G_TX_RENDERTILE, 0, G_TX_MIRROR | G_TX_CLAMP, 6, G_TX_NOLOD, G_TX_MIRROR | G_TX_CLAMP, 6, G_TX_NOLOD),
    gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 0x00FC, 0x00FC),
    gsDPSetTileSize(G_TX_RENDERTILE, 0, 0, 0x01FC, 0x01FC),
    gsSPEndDisplayList()
};

#include "vtx/stencil2.vtx.inc.c"

Gfx D_8014E8F0[] = {
    gsSPTexture(0x8000, 0x8000, 0, G_TX_RENDERTILE, G_OFF),
    gsDPPipeSync(),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetDepthSource(G_ZS_PRIM),
    gsDPSetPrimDepth(0, 0),
    gsDPSetRenderMode(Z_UPD | IM_RD | CVG_DST_SAVE | ZMODE_OPA | FORCE_BL | GBL_c1(G_BL_CLR_MEM, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA), Z_UPD | IM_RD | CVG_DST_SAVE | ZMODE_OPA | FORCE_BL | GBL_c2(G_BL_CLR_MEM, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)),
    gsDPSetCombineMode(PM_CC_SCREEN_OVERLAY, PM_CC_SCREEN_OVERLAY),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsDPSetTextureConvert(G_TC_FILT),
    gsSPClearGeometryMode(G_CULL_BOTH | G_LIGHTING | G_SHADING_SMOOTH),
    gsSPSetGeometryMode(G_ZBUFFER | G_SHADE | G_CULL_BACK),
    gsSPVertex(&vtx_stencil1, 10, 0),
    gsSP2Triangles(0, 1, 2, 0, 3, 0, 2, 0),
    gsSP2Triangles(0, 4, 1, 0, 3, 5, 0, 0),
    gsSP2Triangles(1, 6, 2, 0, 7, 3, 2, 0),
    gsSP2Triangles(8, 7, 2, 0, 7, 9, 3, 0),
    gsDPPipeSync(),
    gsDPSetDepthSource(G_ZS_PIXEL),
    gsSPEndDisplayList()
};

Gfx D_8014E9A8[] = {
    gsDPPipeSync(),
    gsDPSetDepthSource(G_ZS_PRIM),
    gsDPSetPrimDepth(20, 0),
    gsDPSetRenderMode(G_RM_ZB_XLU_SURF, G_RM_ZB_XLU_SURF2),
    gsDPSetCombineMode(PM_CC_SCREEN_OVERLAY, PM_CC_SCREEN_OVERLAY),
    gsSPClearGeometryMode(G_CULL_BOTH | G_LIGHTING | G_SHADING_SMOOTH),
    gsSPSetGeometryMode(G_ZBUFFER | G_SHADE),
    gsDPSetColorDither(G_CD_MAGICSQ),
    gsDPSetCycleType(G_CYC_1CYCLE),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsDPSetTexturePersp(G_TP_NONE),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureConvert(G_TC_FILT),
    gsDPFillRectangle(0, 0, 320, 240),
    gsDPSetColorDither(G_CD_DISABLE),
    gsDPPipeSync(),
    gsDPSetDepthSource(G_ZS_PIXEL),
    gsSPEndDisplayList()
};

Gfx D_8014EA48[] = {
    gsDPSetDepthSource(G_ZS_PRIM),
    gsDPSetPrimDepth(20, 0),
    gsDPSetRenderMode(Z_CMP | CVG_DST_FULL | ZMODE_OPA | ALPHA_CVG_SEL | GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM), Z_CMP | CVG_DST_FULL | ZMODE_OPA | ALPHA_CVG_SEL | GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM)),
    gsSPClearGeometryMode(G_CULL_BOTH | G_LIGHTING),
    gsSPClearGeometryMode(G_SHADING_SMOOTH),
    gsSPTexture(0x8000, 0x8000, 0, G_TX_RENDERTILE, G_ON),
    gsDPPipeSync(),
    gsDPSetCombineMode(G_CC_MODULATEIA, G_CC_MODULATEIA),
    gsDPSetTexturePersp(G_TP_PERSP),
    gsDPSetTextureDetail(G_TD_CLAMP),
    gsDPSetTextureLOD(G_TL_TILE),
    gsDPSetTextureLUT(G_TT_NONE),
    gsDPSetTextureFilter(G_TF_BILERP),
    gsDPSetTextureConvert(G_TC_FILT),
    gsSPVertex(&vtx_stencil2, 24, 0),
    gsDPLoadTextureTile(&D_80156910, G_IM_FMT_RGBA, G_IM_SIZ_16b, 160, 0, 0, 0, 159, 11, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 8, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSP2Triangles(0, 2, 1, 0, 3, 1, 2, 0),
    gsDPLoadTextureTile(&D_80156910, G_IM_FMT_RGBA, G_IM_SIZ_16b, 160, 0, 0, 11, 159, 22, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 8, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSP2Triangles(2, 4, 3, 0, 5, 3, 4, 0),
    gsDPLoadTextureTile(&D_80156910, G_IM_FMT_RGBA, G_IM_SIZ_16b, 160, 0, 0, 22, 159, 33, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 8, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSP2Triangles(4, 6, 5, 0, 7, 5, 6, 0),
    gsDPLoadTextureTile(&D_80156910, G_IM_FMT_RGBA, G_IM_SIZ_16b, 160, 0, 0, 33, 159, 44, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 8, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSP2Triangles(6, 8, 7, 0, 9, 7, 8, 0),
    gsDPLoadTextureTile(&D_80156910, G_IM_FMT_RGBA, G_IM_SIZ_16b, 160, 0, 0, 44, 159, 55, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 8, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSP2Triangles(8, 10, 9, 0, 11, 9, 10, 0),
    gsDPLoadTextureTile(&D_80156910, G_IM_FMT_RGBA, G_IM_SIZ_16b, 160, 0, 0, 55, 159, 66, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 8, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSP2Triangles(10, 12, 11, 0, 13, 11, 12, 0),
    gsDPLoadTextureTile(&D_80156910, G_IM_FMT_RGBA, G_IM_SIZ_16b, 160, 0, 0, 66, 159, 77, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 8, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSP2Triangles(12, 14, 13, 0, 15, 13, 14, 0),
    gsDPLoadTextureTile(&D_80156910, G_IM_FMT_RGBA, G_IM_SIZ_16b, 160, 0, 0, 77, 159, 88, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 8, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSP2Triangles(14, 16, 15, 0, 17, 15, 16, 0),
    gsDPLoadTextureTile(&D_80156910, G_IM_FMT_RGBA, G_IM_SIZ_16b, 160, 0, 0, 88, 159, 99, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 8, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSP2Triangles(16, 18, 17, 0, 19, 17, 18, 0),
    gsDPLoadTextureTile(&D_80156910, G_IM_FMT_RGBA, G_IM_SIZ_16b, 160, 0, 0, 99, 159, 110, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 8, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSP2Triangles(18, 20, 19, 0, 21, 19, 20, 0),
    gsDPLoadTextureTile(&D_80156910, G_IM_FMT_RGBA, G_IM_SIZ_16b, 160, 0, 0, 110, 159, 119, 0, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, 8, 4, G_TX_NOLOD, G_TX_NOLOD),
    gsSP2Triangles(20, 22, 21, 0, 23, 21, 22, 0),
    gsDPPipeSync(),
    gsDPSetDepthSource(G_ZS_PIXEL),
    gsSPEndDisplayList() // ROM has df000000 at 0xE5490, the decomp lacks it (V22b)
};

#ifdef TARGET_3DS
/* NATIVEAPI B1 (S1FILL): a screen or viewport colour overlay is one native fill record (nd_api.h). rgba is the primitive colour, 0xRRGGBBAA. */
typedef struct {
    s32 x0, y0, x1, y1;
    u32 rgba, opaque;
} NdFillRec;

/* core 0: the 1-cycle fill of an overlay, as the walker's fill (nd_rect.c nd_fillrect): primitive colour, lower right exclusive. */
static void fill_run(const uint32_t* u) {
    const NdFillRec* f = (const NdFillRec*)u;
    Gfx cmb[1];
    NdPsoN64 d;
    const int x1 = f->x1 - 1, y1 = f->y1 - 1;
    gDPSetCombineMode(&cmb[0], G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    d.cw0 = cmb[0].words.w0;
    d.cw1 = cmb[0].words.w1;
    d.oml = f->opaque ? ((CVG_DST_SAVE | G_RM_OPA_SURF) | (CVG_DST_SAVE | G_RM_OPA_SURF2)) : (G_RM_CLD_SURF | G_RM_CLD_SURF2);
    d.omh = G_CYC_1CYCLE;
    d.omh_m = 3u << G_MDSFT_CYCLETYPE;
    nd_direct_bind(nd_pso_n64(&d), f->rgba, nd_direct_env());
    if (x1 >= f->x0 && y1 >= f->y0) {
        pm_gpu_set_replace(0);
        pm_gpu_fill_rect(f->x0, f->y0, x1, y1, f->rgba);
    }
    nd_direct_end();
}
#endif

#ifdef TARGET_3DS
/* NATIVEAPI B2a (S1TSTEN): the transition stencil rectangle is one native record (screen_render_util.c writes it). full_and_id = full | id << 1,
 * id 0 Mario, 1 Star, 2 SharpCircle (gPmStencilId, set by _render_transition_stencil on core 1). */
u8 gPmStencilId;

typedef struct {
    u32 w0, w1, hs, hd, prim, full_and_id;
} NdTStenRec;

/* the combine words of a combiner pair, as hud_element.c HUD_CW (two-level so PM_CC_* expands before the 16 arguments are counted) */
#define TST_CW_(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
    { ND_W0_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1), ND_W1_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) }
#define TST_CW(a, b) TST_CW_(a, b)

/* core 0: the transition stencil rectangle. Direct when the tile 0 window of the image and the walker state allow it (the pso is the
 * combine of PM_CC_3A, or PM_CC_PRIM_FULL_ALPHA when full, with the stencil render mode); else the old ops run (combine, prim colour, rectangle). */
void pm_tsten_run(const uint32_t* u) {
    static const u32 cw_3a[2] = TST_CW(PM_CC_3A, PM_CC_3A), cw_full[2] = TST_CW(PM_CC_PRIM_FULL_ALPHA, PM_CC_PRIM_FULL_ALPHA);
    static const unsigned wrapT[3][4] = { { 2, 2, 6, 6 }, { 3, 2, 5, 6 }, { 3, 3, 5, 5 } };
    static const int strideT[3] = { 32, 16, 16 }, ldwT[3] = { 32, 16, 16 }, ldhT[3] = { 64, 64, 32 };
    static const int sizeT[3] = { 0xFC, 0xFC, 0xFC };
    static Gfx buf[8];
    static unsigned n;
    const NdTStenRec* r = (const NdTStenRec*)u;
    const int id = (int)((r->full_and_id >> 1) & 3u), full = (int)(r->full_and_id & 1u);
    const void* img = id == 0 ? (const void*)ui_stencil_mario_png : id == 1 ? (const void*)ui_stencil_star_png : (const void*)ui_stencil_sharp_circle_png;
    NdBindDesc d;
    NdPsoN64 pd;
    Gfx* p = buf;
    unsigned wrap[4];
    int ok = 0, k;

    for (k = 0; k < 4; k++) wrap[k] = wrapT[id][k];
    pd.cw0 = (full ? cw_full : cw_3a)[0];
    pd.cw1 = (full ? cw_full : cw_3a)[1];
    pd.oml = G_RM_CLD_SURF | G_RM_CLD_SURF2;
    pd.omh = G_TT_NONE | G_TF_AVERAGE;
    pd.omh_m = (3u << G_MDSFT_TEXTLUT) | (3u << G_MDSFT_TEXTFILT);
    if (nd_tex_win_desc(&d, img, strideT[id], G_IM_FMT_I, G_IM_SIZ_4b, wrap, 0, 0, ldwT[id], ldhT[id], 0, 0, sizeT[id], sizeT[id], NULL, 0) &&
        nd_direct_ready()) {
        NdPso pid = nd_pso_n64(&pd);
        if (nd_direct_bind(pid, r->prim, nd_direct_env())) {
            nd_direct_tex(&d);
            nd_direct_rect(r->w0, r->w1, r->hs, r->hd);
            nd_direct_end();
            ok = 1;
        }
    }
    if (n++ < 40) {
        extern void pm_log_d(const char* fmt, ...);
        extern unsigned pm_frame_count(void);
        pm_log_d("tsten f%u id %u direct %d", pm_frame_count(), id, ok);
    }
    if (ok) return;

    if (full) gDPSetCombineMode(p++, PM_CC_PRIM_FULL_ALPHA, PM_CC_PRIM_FULL_ALPHA);
    gDPSetPrimColor(p++, 0, 0, (int)(r->prim >> 24), (int)((r->prim >> 16) & 0xFFu), (int)((r->prim >> 8) & 0xFFu), (int)(r->prim & 0xFFu));
    ND_GX(p++, r->w0, r->w1);
    gImmp1(p++, G_RDPHALF_1, r->hs);
    gImmp1(p++, G_RDPHALF_2, r->hd);
    nd_walk_state((const uint32_t*)buf, (unsigned)(p - buf));
}
#endif

void _render_transition_stencil(u8 stencilType, f32 progress, ScreenOverlay* overlay) {
    Camera* camera = &gCameras[gCurrentCameraID];
    u8 colR, colG, colB;
    s32 x1, y1, x2, y2;
    f32 alpha;
    s16 v0;
    s16 s0;
    Mtx* matrixStack = gDisplayContext->matrixStack;

    if (progress == 0.0f) {
        return;
    }

    if (overlay != nullptr) {
        colR = overlay->color.r;
        colG = overlay->color.g;
        colB = overlay->color.b;
        x1 = overlay->screenPos[0][0];
        y1 = overlay->screenPos[0][1];
        x2 = overlay->screenPos[1][0];
        y2 = overlay->screenPos[1][1];
        alpha = overlay->alpha;
    } else {
        colR = colG = colB = 0;
        x1 = y1 = x2 = y2 = 0;
        alpha = 0.0f;
    }

    switch (stencilType) {
        case OVERLAY_SCREEN_COLOR:
#ifdef TARGET_3DS
            /* the scissor stays a stream op before the record: later walker draws may inherit it */
            gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            /* 1-cycle fill: the lower right is exclusive (HUDB1). The N64 overscan hides the last column and row, the port shows them. */
            {
                const NdFillRec rec = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ((u32)colR << 24) | ((u32)colG << 16) | ((u32)colB << 8) | (u32)(u8)progress,
                                        progress == 255.0f };
                nd_rec_napi(fill_run, &rec, 3u);
            }
#else
            gDPPipeSync(gMainGfxPos++);
            gDPSetColorDither(gMainGfxPos++, G_CD_MAGICSQ);
            gDPSetAlphaDither(gMainGfxPos++, G_AD_PATTERN);
            gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
            if (progress == 255.0f) {
                gDPSetRenderMode(gMainGfxPos++, CVG_DST_SAVE | G_RM_OPA_SURF, CVG_DST_SAVE | G_RM_OPA_SURF2);
            } else {
                gDPSetRenderMode(gMainGfxPos++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
            }
            gDPSetCombineMode(gMainGfxPos++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
            gDPSetPrimColor(gMainGfxPos++, 0, 0, colR, colG, colB, progress);
            gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            gDPFillRectangle(gMainGfxPos++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
            gDPSetColorDither(gMainGfxPos++, G_CD_DISABLE);
#endif
            return;
        case OVERLAY_VIEWPORT_COLOR:
#ifdef TARGET_3DS
            gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            {
                const s32 vx = camera->viewportStartX, vy = camera->viewportStartY;
                const NdFillRec rec = { vx, vy, vx + camera->viewportW, vy + camera->viewportH,
                                        ((u32)colR << 24) | ((u32)colG << 16) | ((u32)colB << 8) | (u32)(u8)progress, progress == 255.0f };
                nd_rec_napi(fill_run, &rec, 3u);
            }
#else
            gDPPipeSync(gMainGfxPos++);
            gDPSetColorDither(gMainGfxPos++, G_CD_MAGICSQ);
            gDPSetAlphaDither(gMainGfxPos++, G_AD_PATTERN);
            gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
            if (progress == 255.0f) {
                gDPSetRenderMode(gMainGfxPos++, CVG_DST_SAVE | G_RM_OPA_SURF, CVG_DST_SAVE | G_RM_OPA_SURF2);
            } else {
                gDPSetRenderMode(gMainGfxPos++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
            }
            gDPSetCombineMode(gMainGfxPos++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
            gDPSetPrimColor(gMainGfxPos++, 0, 0, colR, colG, colB, progress);
            gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            gDPFillRectangle(gMainGfxPos++, camera->viewportStartX, camera->viewportStartY,
                             camera->viewportStartX + camera->viewportW, camera->viewportStartY + camera->viewportH);
            gDPSetColorDither(gMainGfxPos++, G_CD_DISABLE);
#endif
            return;
    }

    guOrtho(&matrixStack[gMatrixListPos], 0.0f, 320.0f, 0.0f, 240.0f, -1000.0f, 1000.0f, 1.0f);
    gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);

    switch (stencilType) {
        case OVERLAY_VIEWPORT_MARIO:
            gSPDisplayList(gMainGfxPos++, Gfx_LoadStencilTex_Mario);
#ifdef TARGET_3DS
            gPmStencilId = 0;
#endif
            appendGfx_screen_transition_stencil(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, progress, colR, colG, colB, progress * alpha / 255.0f, gCurrentCameraID);
            break;
        case OVERLAY_SCREEN_MARIO:
            gSPDisplayList(gMainGfxPos++, Gfx_LoadStencilTex_Mario);
#ifdef TARGET_3DS
            gPmStencilId = 0;
#endif
            appendGfx_screen_transition_stencil(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, progress, colR, colG, colB, progress * alpha / 255.0f, -1);
            break;
        case OVERLAY_VIEWPORT_STAR:
            gSPDisplayList(gMainGfxPos++, Gfx_LoadStencilTex_Star);
#ifdef TARGET_3DS
            gPmStencilId = 1;
#endif
            appendGfx_screen_transition_stencil(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, progress, colR, colG, colB, progress * alpha / 255.0f, gCurrentCameraID);
            break;
        case OVERLAY_SCREEN_STAR:
            gSPDisplayList(gMainGfxPos++, Gfx_LoadStencilTex_Star);
#ifdef TARGET_3DS
            gPmStencilId = 1;
#endif
            appendGfx_screen_transition_stencil(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, progress, colR, colG, colB, progress * alpha / 255.0f, -1);
            break;
        case OVERLAY_VIEWPORT_SPOTLIGHT:
            gSPDisplayList(gMainGfxPos++, Gfx_LoadStencilTex_SharpCircle);
#ifdef TARGET_3DS
            gPmStencilId = 2;
#endif
            appendGfx_screen_transition_stencil(x1, y1, progress, 0, 0, 0, 0, gCurrentCameraID);
            break;
        case OVERLAY_SCREEN_SPOTLIGHT:
            gSPDisplayList(gMainGfxPos++, Gfx_LoadStencilTex_SharpCircle);
#ifdef TARGET_3DS
            gPmStencilId = 2;
#endif
            appendGfx_screen_transition_stencil(x1, y1, progress, 0, 0, 0, 0, -1);
            break;
        case OVERLAY_TYPE_2:
            s0 = progress;
            guTranslate(&matrixStack[gMatrixListPos], 80.0f, 120.0f, 0.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            guScale(&matrixStack[gMatrixListPos], (1.0f - s0 / 255.0f) * 0.8, (1.0f - s0 / 255.0f) * 0.8, 1.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            guRotate(&matrixStack[gMatrixListPos], s0 * 0.5f, 0.0f, 0.0f, 1.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            gSPDisplayList(gMainGfxPos++, D_8014E8F0);
            gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);

            guTranslate(&matrixStack[gMatrixListPos], 240.0f, 120.0f, 0.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            guScale(&matrixStack[gMatrixListPos], (1.0f - s0 / 255.0f) * 0.8, (1.0f - s0 / 255.0f) * 0.8, 1.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            guRotate(&matrixStack[gMatrixListPos], s0 * 0.5f, 0.0f, 0.0f, 1.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            gSPDisplayList(gMainGfxPos++, D_8014E8F0);
            gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);

            guTranslate(&matrixStack[gMatrixListPos], 0.0f, 0.0f, 0.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gSPDisplayList(gMainGfxPos++, D_8014EA48);
            gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);
            break;
        case OVERLAY_TYPE_9:
            s0 = progress;
            gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 0, 0);
            guTranslate(&matrixStack[gMatrixListPos], x1, SCREEN_HEIGHT - y1, 0.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            guScale(&matrixStack[gMatrixListPos], (1.0f - s0 / 255.0f) * 0.8, (1.0f - s0 / 255.0f) * 0.8, 1.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            guRotate(&matrixStack[gMatrixListPos], -s0, 0.0f, 0.0f, 1.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            gSPDisplayList(gMainGfxPos++, D_8014E8F0);
            gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);

            guTranslate(&matrixStack[gMatrixListPos], x2, SCREEN_HEIGHT - y2, 0.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            guScale(&matrixStack[gMatrixListPos], (1.0f - s0 / 255.0f) * 0.8, (1.0f - s0 / 255.0f) * 0.8, 1.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            guRotate(&matrixStack[gMatrixListPos], -s0, 0.0f, 0.0f, 1.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            gSPDisplayList(gMainGfxPos++, D_8014E8F0);
            gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);

            guTranslate(&matrixStack[gMatrixListPos], 0.0f, 0.0f, 0.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 0, alpha);
            gSPDisplayList(gMainGfxPos++, D_8014E9A8);
            gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);
            break;
        case OVERLAY_START_BATTLE:
            s0 = progress;
            gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 0, 0);
            guTranslate(&matrixStack[gMatrixListPos], x1, SCREEN_HEIGHT - y1, 0.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            guScale(&matrixStack[gMatrixListPos], (1.0f - s0 / 255.0f) * 0.8, (1.0f - s0 / 255.0f) * 0.8, 1.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            guRotate(&matrixStack[gMatrixListPos], (f32)(-s0) * 0.5, 0.0f, 0.0f, 1.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            gSPDisplayList(gMainGfxPos++, D_8014E8F0);
            gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);

            guTranslate(&matrixStack[gMatrixListPos], 0.0f, 0.0f, 0.0f);
            gSPMatrix(gMainGfxPos++, &matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 0, alpha);
            gSPDisplayList(gMainGfxPos++, D_8014E9A8);
            gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);
            v0 = progress + 40;
            if (progress > 170) {
                v0 = 170;
            }
            draw_prev_frame_buffer_at_screen_pos(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, v0);
            break;
        case OVERLAY_WORLD_DARKNESS:
            gSPDisplayList(gMainGfxPos++, Gfx_LoadStencilTex_BlurryCircle);
            appendGfx_darkness_stencil(true, x2, y2, alpha, progress);
            break;
        case OVERLAY_BLUR:
            draw_prev_frame_buffer_at_screen_pos(x1, y1, x2, y2, progress);
            break;
        case OVERLAY_BATTLE_DARKNESS:
            gSPDisplayList(gMainGfxPos++, Gfx_LoadStencilTex_BlurryCircle);
            appendGfx_darkness_stencil(false, x1, y1, alpha, progress);
            break;
        case OVERLAY_INTRO_1:
        case OVERLAY_INTRO_2:
            break;
    }

    gSPMatrix(gMainGfxPos++, &gDisplayContext->camPerspMatrix[gCurrentCameraID], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
}

void set_screen_overlay_params_front(u8 type, f32 zoom) {
    screen_overlay_frontType = type;
    screen_overlay_frontProgress = zoom;
}

void set_screen_overlay_params_back(u8 type, f32 zoom) {
    screen_overlay_backType = type;
    screen_overlay_backProgress = zoom;
}

void get_screen_overlay_params(s32 layer, u8* type, f32* zoom) {
    switch (layer) {
        case SCREEN_LAYER_FRONT:
            *type = screen_overlay_frontType;
            *zoom = screen_overlay_frontProgress;
            break;
        case SCREEN_LAYER_BACK:
            *type = screen_overlay_backType;
            *zoom = screen_overlay_backProgress;
            break;
    }
}

void set_screen_overlay_color(s32 layer, u8 r, u8 g, u8 b) {
    switch (layer) {
        case SCREEN_LAYER_FRONT:
        case SCREEN_LAYER_BACK:
            ScreenOverlays[layer].color.r = r;
            ScreenOverlays[layer].color.g = g;
            ScreenOverlays[layer].color.b = b;
            break;
    }
}

void set_screen_overlay_center(s32 layer, s32 arg1, s32 screenPosX, s32 screenPosY) {
    switch (layer) {
        case SCREEN_LAYER_FRONT:
        case SCREEN_LAYER_BACK:
            switch (arg1) {
                case 0:
                    ScreenOverlays[layer].screenPos[0][0] = screenPosX;
                    ScreenOverlays[layer].screenPos[0][1] = screenPosY;
                    break;
                case 1:
                    ScreenOverlays[layer].screenPos[1][0] = screenPosX;
                    ScreenOverlays[layer].screenPos[1][1] = screenPosY;
                    break;
            }
            break;
    }
}

void set_screen_overlay_center_worldpos(s32 layer, s32 posIdx, s32 worldPosX, s32 worldPosY, s32 worldPosZ) {
    Camera* camera = &gCameras[gCurrentCameraID];
    f32 tx, ty, tz, tw;

    switch (layer) {
        case SCREEN_LAYER_FRONT:
        case SCREEN_LAYER_BACK:
            transform_point(camera->mtxPerspective, worldPosX, worldPosY, worldPosZ, 1.0f, &tx, &ty, &tz, &tw);
            tw = 1.0f / tw;
            tx *= tw;
            ty *= -tw;
            tz *= tw;
            tx = (((tx * camera->viewportW) + camera->viewportW) * 0.5) + camera->viewportStartX;
            ty = (((ty * camera->viewportH) + camera->viewportH) * 0.5) + camera->viewportStartY;
            if (pm_wide_cam(camera)) { /* UA1: same x rule as get_screen_coords (3D x 1.25, 2D + 40 px) */
                tx = tx * 1.25f - 40.0f;
            }

            switch (posIdx) {
                case 0:
                    ScreenOverlays[layer].screenPos[0][0] = tx;
                    ScreenOverlays[layer].screenPos[0][1] = ty;
                    break;
                case 1:
                    ScreenOverlays[layer].screenPos[1][0] = tx;
                    ScreenOverlays[layer].screenPos[1][1] = ty;
                    break;
            }
            break;
    }
}

void set_screen_overlay_alpha(s32 layer, f32 alpha) {
    switch (layer) {
        case SCREEN_LAYER_FRONT:
        case SCREEN_LAYER_BACK:
            ScreenOverlays[layer].alpha = alpha;
            break;
    }
}

void clear_screen_overlays(void) {
    ScreenOverlay* it;
    s32 i;

    screen_overlay_frontType = OVERLAY_NONE;
    screen_overlay_backType  = OVERLAY_NONE;
    screen_overlay_frontProgress = OVERLAY_RENDER_OFF;
    screen_overlay_backProgress  = OVERLAY_RENDER_OFF;

    for (it = &ScreenOverlays[0], i = 0; i < ARRAY_COUNT(ScreenOverlays); i++, it++) {
        it->color.b = 0;
        it->color.g = 0;
        it->color.r = 0;
        it->screenPos[1][1] = 0;
        it->screenPos[1][0] = 0;
        it->screenPos[0][1] = 0;
        it->screenPos[0][0] = 0;
        it->alpha = 255.0f;
    }
}

void reset_back_screen_overlay_progress(void) {
    screen_overlay_backProgress = 0;
}

void func_80138198(void) {
}

void render_screen_overlay_frontUI(void) {
    if (screen_overlay_frontType != OVERLAY_NONE
        && screen_overlay_frontProgress != OVERLAY_RENDER_OFF
        && gGameStatusPtr->context != CONTEXT_PAUSE
    ) {
        _render_transition_stencil(screen_overlay_frontType, screen_overlay_frontProgress, &ScreenOverlays[0]);
    }
}

void render_screen_overlay_backUI(void) {
    if (screen_overlay_backType != OVERLAY_NONE
        && screen_overlay_backProgress != OVERLAY_RENDER_OFF
        && gGameStatusPtr->context != CONTEXT_PAUSE
    ) {
        _render_transition_stencil(screen_overlay_backType, screen_overlay_backProgress, &ScreenOverlays[1]);
    }
}

void set_map_transition_effect(ScreenTransition transition) {
    CurrentScreenTransition = transition;
}

enum ScreenFadeRates {
    VERY_SLOW_FADE_RATE = 2,
    SLOW_FADE_RATE      = 7,
    SLOWER_FADE_RATE    = 10,
    STANDARD_FADE_RATE  = 20,
    FAST_FADE_RATE      = 50,
};

s16 update_exit_map_screen_overlay(s16* progress) {
    u8 overlayColor;
    u8 fadeOutType = OVERLAY_SCREEN_COLOR;
    u8 r = 0;
    u8 g = 0;
    u8 b = 0;
    s16 t = 0;
    s32 fadeRate = STANDARD_FADE_RATE;

    switch (CurrentScreenTransition) {
        case TRANSITION_STANDARD:
            fadeOutType = OVERLAY_VIEWPORT_COLOR;
            break;
        case TRANSITION_TOY_TRAIN:
            fadeOutType = OVERLAY_SCREEN_COLOR;
            break;
        case TRANSITION_END_DEMO_SCENE_BLACK:
            fadeOutType = OVERLAY_VIEWPORT_COLOR;
            fadeRate = FAST_FADE_RATE;
            break;
        case TRANSITION_END_DEMO_SCENE_WHITE:
            r = g = b = 208;
            fadeOutType = OVERLAY_VIEWPORT_COLOR;
            fadeRate = SLOWER_FADE_RATE;
            if (gGameStatusPtr->demoState == DEMO_STATE_CHANGE_MAP) {
                gGameStatusPtr->nextDemoScene = LAST_DEMO_SCENE_IDX;
            }
            break;
        case TRANSITION_BEGIN_OR_END_GAME:
        case TRANSITION_OUTRO_END_SCENE:
            fadeOutType = OVERLAY_VIEWPORT_COLOR;
            fadeRate = SLOW_FADE_RATE;
            break;
        case TRANSITION_BEGIN_OR_END_CHAPTER:
            r = g = b = 208;
            fadeOutType = OVERLAY_VIEWPORT_COLOR;
            break;
        case TRANSITION_SLOW_FADE_TO_WHITE:
            if (gGameStatusPtr->demoState == DEMO_STATE_CHANGE_MAP) {
                gGameStatusPtr->nextDemoScene = LAST_DEMO_SCENE_IDX;
            }
            r = g = b = 208;
            fadeOutType = OVERLAY_VIEWPORT_COLOR;
            fadeRate = SLOW_FADE_RATE;
            break;
        case TRANSITION_AFTER_SAVE_PROMPT:
        case TRANSITION_END_CHAPTER_INTERRUPTED:
            r = g = b = 208;
            fadeOutType = OVERLAY_VIEWPORT_COLOR;
            fadeRate = SLOW_FADE_RATE;
            break;
        case TRANSITION_GET_STAR_CARD:
            r = g = b = 208;
            fadeOutType = OVERLAY_VIEWPORT_COLOR;
            fadeRate = FAST_FADE_RATE;
            break;
        case TRANSITION_ENTER_WORLD:
            set_screen_overlay_alpha(SCREEN_LAYER_FRONT, 0.0f);
            fadeOutType = OVERLAY_VIEWPORT_MARIO;
            break;
        case TRANSITION_MARIO_WHITE:
            r = g = b = 208;
            set_screen_overlay_alpha(SCREEN_LAYER_FRONT, 0.0f);
            fadeOutType = OVERLAY_VIEWPORT_MARIO;
            fadeRate = SLOW_FADE_RATE;
            break;
        case TRANSITION_MARIO_BLACK:
            set_screen_overlay_alpha(SCREEN_LAYER_FRONT, 0.0f);
            fadeOutType = OVERLAY_VIEWPORT_MARIO;
            fadeRate = SLOW_FADE_RATE;
            break;
        case TRANSITION_END_PEACH_INTERLUDE:
            set_screen_overlay_alpha(SCREEN_LAYER_FRONT, 160.0f);
            r = g = b = 208;
            fadeOutType = OVERLAY_VIEWPORT_STAR;
            fadeRate = SLOW_FADE_RATE;
            break;
        case TRANSITION_PEACH_CAPTURED:
            set_screen_overlay_alpha(SCREEN_LAYER_FRONT, 0.0f);
            fadeOutType = OVERLAY_VIEWPORT_STAR;
            fadeRate = SLOW_FADE_RATE;
            break;
        case TRANSITION_SLOW_BLUR_MOTION:
            set_screen_overlay_center(SCREEN_LAYER_FRONT, 0, 15, 28);
            set_screen_overlay_center(SCREEN_LAYER_FRONT, 1, 305, 156);
            set_screen_overlay_params_front(OVERLAY_BLUR, 255.0f);
            *progress = 255;
            return 1;
    }

    if (CurrentScreenTransition == TRANSITION_OUTRO_END_SCENE) {
        overlayColor = ((255 - *progress) * 208) / 255;
        set_screen_overlay_color(SCREEN_LAYER_FRONT, overlayColor, overlayColor, overlayColor);
        set_screen_overlay_params_front(fadeOutType, 255.0f);
        if (*progress == 255) {
            return 1;
        }

        *progress += fadeRate;
        if (*progress > 255) {
            *progress = 255;
        }
    } else {
        set_screen_overlay_color(t, r, g, b);

        if (t == 0) {
            set_screen_overlay_params_front(fadeOutType, *progress);
        } else {
            set_screen_overlay_params_back(fadeOutType, *progress);
        }

        if (*progress == 255) {
            return 1;
        }

        *progress += fadeRate;
        if (*progress > 255) {
            *progress = 255;
        }
    }
    return 0;
}

s16 update_enter_map_screen_overlay(s16* progress) {
    u8 fadeInType = OVERLAY_SCREEN_COLOR;
    s32 fadeRate = STANDARD_FADE_RATE;
    u8 ret = false;

    switch (CurrentScreenTransition) {
        case TRANSITION_END_DEMO_SCENE_WHITE:
            set_screen_overlay_color(SCREEN_LAYER_FRONT, 208, 208, 208);
            fadeRate = FAST_FADE_RATE;
            break;
        case TRANSITION_END_DEMO_SCENE_BLACK:
            fadeInType = OVERLAY_VIEWPORT_COLOR;
            fadeRate = FAST_FADE_RATE;
            break;
        case TRANSITION_BEGIN_OR_END_GAME:
        case TRANSITION_OUTRO_END_SCENE:
        case TRANSITION_SLOW_FADE_TO_WHITE:
        case TRANSITION_MARIO_WHITE:
        case TRANSITION_MARIO_BLACK:
            fadeInType = OVERLAY_VIEWPORT_COLOR;
            fadeRate = SLOW_FADE_RATE;
            break;
        case TRANSITION_STANDARD:
        case TRANSITION_TOY_TRAIN:
        case TRANSITION_BEGIN_OR_END_CHAPTER:
        case TRANSITION_PEACH_CAPTURED:
        case TRANSITION_GET_STAR_CARD:
            fadeInType = OVERLAY_VIEWPORT_COLOR;
            break;
        case TRANSITION_ENTER_WORLD:
        case TRANSITION_END_CHAPTER_INTERRUPTED:
            fadeInType = OVERLAY_VIEWPORT_MARIO;
            break;
        case TRANSITION_AFTER_SAVE_PROMPT:
            fadeInType = OVERLAY_VIEWPORT_STAR;
            fadeRate = SLOW_FADE_RATE;
            break;
        case TRANSITION_END_PEACH_INTERLUDE:
            fadeInType = OVERLAY_VIEWPORT_MARIO;
            fadeRate = SLOW_FADE_RATE;
            break;
        case TRANSITION_SLOW_BLUR_MOTION:
            set_screen_overlay_center(SCREEN_LAYER_FRONT, 0, 15, 28);
            set_screen_overlay_center(SCREEN_LAYER_FRONT, 1, 305, 156);
            set_screen_overlay_params_front(OVERLAY_BLUR, *progress);
            fadeRate = VERY_SLOW_FADE_RATE;
            break;
    }

    if (CurrentScreenTransition != TRANSITION_SLOW_BLUR_MOTION) {
        set_screen_overlay_params_front(fadeInType, *progress);
    }

    if (*progress == 0) {
        ret = true;
    }

    *progress -= fadeRate;

    if (*progress < 0) {
        *progress = 0;
    }

    return ret;
}
