#include "pm_wide.h"
#include "common.h"
#include "hud_element.h"
#include "nu/nusys.h"
#include "ld_addrs.h"
#include "pm_interp_fi.h"
#include "nd_api.h"
#ifdef TARGET_3DS
#endif

#define MAX_HUD_CACHE_ENTRIES 192

typedef struct HudElementSize {
    s16 width;
    s16 height;
    s16 size;
} HudElementSize;

u8* HudElemAuxCache = nullptr;
s32 HudElemCacheCapacity = 0x11000;

HudScript HES_Empty = {
    hs_End
};

HudElementSize HudElemSizes[] = {
    { 8, 8, 32 },
    { 16, 16, 128 },
    { 24, 24, 288 },
    { 32, 32, 512 },
    { 48, 48, 1152 },
    { 64, 64, 2048 },
    { 8, 16, 64 },
    { 16, 8, 64 },
    { 16, 24, 192 },
    { 16, 32, 256 },
    { 64, 32, 1024 },
    { 32, 16, 256 },
    { 12, 12, 72 },
    { 48, 24, 576 },
    { 32, 8, 128 },
    { 24, 8, 96 },
    { 64, 16, 512 },
    { 16, 64, 512 },
    { 192, 32, 3072 },
    { 40, 40, 800 },
    { 24, 16, 192 },
    { 32, 40, 640 },
    { 40, 16, 320 },
    { 40, 24, 480 },
    { 32, 24, 384 },
    { 20, 32, 0 }
};

s16 D_8014F068[] = { 34, 32, 0, 40, 40, 968, 0, 0 };
Gfx HudElemBackInitGfx[] = {
    gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
    gsDPPipeSync(),
    gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_ALPHADITHER, 18, G_AD_DISABLE | G_CD_DISABLE | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsSPEndDisplayList()
};
Gfx HudElemFrontInitGfx[] = {
    gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT),
    gsDPPipeSync(),
    gsSPSetOtherMode(G_SETOTHERMODE_H, G_MDSFT_ALPHADITHER, 18, G_AD_DISABLE | G_CD_DISABLE | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_NONE | G_CYC_1CYCLE),
    gsDPSetAlphaCompare(G_AC_NONE),
    gsSPEndDisplayList()
};

Vtx HudElemTemplateQuad[] = {
    {
        .v = {
            .ob = { -12, 0, 0},
            .flag = 0,
            .tc = { 0x800, 0x800 },
            .cn = { 0, 0, 0, 255}
        }
    },
    {
        .v = {
            .ob = { 51, 0, 0},
            .flag = 0,
            .tc = { 0, 0x800 },
            .cn = { 0, 0, 0, 255}
        }
    },
    {
        .v = {
            .ob = { 51, 63, 0},
            .flag = 0,
            .tc = { 0, 0 },
            .cn = { 0, 0, 0, 255}
        }
    },
    {
        .v = {
            .ob = { -12, 63, 0},
            .flag = 0,
            .tc = { 0x800, 0 },
            .cn = { 0, 0, 0, 255}
        }
    }
};

Lights1 HudElemLights = gdSPDefLights1(255, 255, 255, 0, 0, 0, 0, 0, 0);

s32 HudElemCount;
HudCacheEntry* gHudElementCacheTableRaster;
HudCacheEntry* gHudElementCacheTablePalette;
s32* gHudElementCacheSize;
u8* gHudElementCacheBuffer;

BSS HudElementList gHudElementsWorld;
BSS HudElementList gHudElementsBattle;
BSS HudElementList* gHudElements;
BSS u8* gHudElementCacheBufferWorld;
BSS s32 gHudElementCacheSizeWorld;
BSS s32 D_8015796C;
BSS HudCacheEntry gHudElementCacheTableRasterWorld[192];
BSS HudCacheEntry gHudElementCacheTablePaletteWorld[192];
BSS u8* gHudElementCacheBufferBattle;
BSS s32 gHudElementCacheSizeBattle;
BSS s32 D_80158578[2];
BSS HudCacheEntry gHudElementCacheTableRasterBattle[192];
BSS HudCacheEntry gHudElementCacheTablePaletteBattle[192];
BSS s32 FrameQuadIndex;

void hud_element_setup_cam(void);

void hud_element_load_script(HudElement* hudElement, HudScript* anim) {
    s32* pos = (s32*)anim;
    s32 raster;
    s32 palette;
    s32 preset;
    Vec3s* size;
    HudCacheEntry* entry;
    s32 i;
    s32 capacity;

    if (pos == nullptr) {
        return;
    }

    preset = 0;

    while (true) {
        switch (*pos++) {
            case HUD_ELEMENT_OP_End:
                return;
            case HUD_ELEMENT_OP_SetCI:
                pos += 3;
                break;
            case HUD_ELEMENT_OP_SetTileSize:
                preset = *pos++;
                hudElement->drawSizePreset = hudElement->tileSizePreset = preset;
                break;
            case HUD_ELEMENT_OP_SetSizesAutoScale:
            case HUD_ELEMENT_OP_SetSizesFixedScale:
                preset = *pos;
                pos += 2;
                hudElement->drawSizePreset = hudElement->tileSizePreset = preset;
                break;
            case HUD_ELEMENT_OP_SetCustomSize:
                hudElement->customDrawSize.x = hudElement->customImageSize.x = *pos++;
                hudElement->customDrawSize.y = hudElement->customImageSize.y = *pos++;
                hudElement->flags |= HUD_ELEMENT_FLAG_CUSTOM_SIZE;
                break;
            case HUD_ELEMENT_OP_AddTexelOffsetX:
            case HUD_ELEMENT_OP_AddTexelOffsetY:
            case HUD_ELEMENT_OP_SetScale:
            case HUD_ELEMENT_OP_SetAlpha:
            case HUD_ELEMENT_OP_SetVariable:
            case HUD_ELEMENT_OP_RandomBranch:
            case HUD_ELEMENT_OP_SetFlags:
            case HUD_ELEMENT_OP_ClearFlags:
            case HUD_ELEMENT_OP_PlaySound:
                pos++;
                break;
            case HUD_ELEMENT_OP_SetRGBA:
            case HUD_ELEMENT_OP_SetTexelOffset:
            case HUD_ELEMENT_OP_RandomDelay:
            case HUD_ELEMENT_OP_RandomRestart:
            case HUD_ELEMENT_OP_SetPivot:
                pos += 2;
                break;
            case HUD_ELEMENT_OP_SetImage:
                pos++;
                raster = *pos++;
                palette = *pos++;

                i = 0;
                entry = gHudElementCacheTableRaster;
                while (true) {
                    if (entry->id == -1) {
                        entry->id = raster;
                        entry->data = &gHudElementCacheBuffer[*gHudElementCacheSize];
                        if (gGameStatusPtr->context == CONTEXT_WORLD) {
                            capacity = HudElemCacheCapacity;
                        } else {
                            capacity = HudElemCacheCapacity / 2;
                        }
                        ASSERT(capacity > *gHudElementCacheSize + HudElemSizes[preset].size);
                        nuPiReadRom((s32)icon_ROM_START + raster, entry->data, HudElemSizes[preset].size);
                        *gHudElementCacheSize += HudElemSizes[preset].size;
                        if (gGameStatusPtr->context == CONTEXT_WORLD) {
                            *pos = i;
                        } else {
                            *pos = (u16)(*pos) | (i << 16);
                        }
                        i++;
                        break;
                    } else if (entry->id == raster) {
                        if (gGameStatusPtr->context == CONTEXT_WORLD) {
                            *pos = i;
                        } else {
                            *pos = (u16)(*pos) | (i << 16);
                        }
                        break;
                    }
                    entry++;
                    i++;
                }

                pos++;
                ASSERT(i < MAX_HUD_CACHE_ENTRIES);

                entry = gHudElementCacheTablePalette;
                i = 0;
                while (true) {
                    if (entry->id == -1) {
                        entry->id = palette;
                        entry->data = &gHudElementCacheBuffer[*gHudElementCacheSize];
                        if (gGameStatusPtr->context == CONTEXT_WORLD) {
                            capacity = HudElemCacheCapacity;
                        } else {
                            capacity = HudElemCacheCapacity / 2;
                        }
                        ASSERT(capacity > *gHudElementCacheSize + 32);
                        nuPiReadRom((s32)icon_ROM_START + palette, entry->data, 32);
                        *gHudElementCacheSize += 32;
                        if (gGameStatusPtr->context == CONTEXT_WORLD) {
                            *pos = i;
                        } else {
                            *pos = (u16)(*pos) | (i << 16);
                        }
                        i++;
                        break;
                    } else if (entry->id == palette) {
                        if (gGameStatusPtr->context == CONTEXT_WORLD) {
                            *pos = i;
                        } else {
                            *pos = (u16)(*pos) | (i << 16);
                        }
                        break;
                    }
                    entry++;
                    i++;
                }

                pos++;
                ASSERT(i < MAX_HUD_CACHE_ENTRIES);
                break;
        }
    }
}

/* NAPI2A: the HUD rectangle record (plan_nativeapi S2a). hud_element_draw_rect (core 1) keeps its arithmetic and records one
 * NdHudPiece per texture tile. hud_rect_emit writes the gDP and gSP words of a record: core 1 runs it on gMainGfxPos (fps60 anchors
 * stay here); core 0 (hud_rect_run) runs it into a local list and walks that list with nd_walk_state. */
#define ND_HUD_PC 32 /* texture sizes are at most 255 x 255: 4 x 8 pieces of 64 x 32 texels */
void pm_fatal_stop(const char* msg);
int pm_anchor_is_open(void);
typedef struct NdHudPiece {
    s32 uls, ult, lrs, lrt, ulx, uly, lrx, lry, texStartX, texStartY, tileMode, pad;
} NdHudPiece;
typedef struct NdHudRec {
    u32 flags;
    IMG_PTR imageAddr;
    PAL_PTR paletteAddr;
    s32 widthScale, heightScale, fmt, masks, maskt, flipX, flipY, clamp, dropShadow, npc;
    s16 texSizeX, texSizeY;
    u8 opacity;
    Color_RGB8 tint;
    s32 pad;
    NdHudPiece pc[ND_HUD_PC];
} NdHudRec;
_Static_assert(sizeof(NdHudPiece) == 48 && __builtin_offsetof(NdHudRec, pc) == 64, "NdHudRec: whole 8-byte units");

#define gMainGfxPos (*gp)
static void hud_rect_emit(const NdHudRec* hudElement, Gfx** gp, s32 anchor) {
    s32 fmt, masks, maskt, flipX, flipY, clamp, dropShadow, widthScale, heightScale;
    s16 texSizeX, texSizeY;
    IMG_PTR imageAddr;
    PAL_PTR paletteAddr;
    s32 k;

    fmt = hudElement->fmt;
    masks = hudElement->masks;
    maskt = hudElement->maskt;
    flipX = hudElement->flipX;
    flipY = hudElement->flipY;
    clamp = hudElement->clamp;
    dropShadow = hudElement->dropShadow;
    widthScale = hudElement->widthScale;
    heightScale = hudElement->heightScale;
    texSizeX = hudElement->texSizeX;
    texSizeY = hudElement->texSizeY;
    imageAddr = hudElement->imageAddr;
    paletteAddr = hudElement->paletteAddr;


    switch (fmt) {
        case 0:
            gDPSetRenderMode(gMainGfxPos++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
            gDPSetTextureLUT(gMainGfxPos++, G_TT_NONE);
            break;
        case 1:
            if (!(hudElement->flags & HUD_ELEMENT_FLAG_TRANSPARENT)) {
                if (!(hudElement->flags & HUD_ELEMENT_FLAG_ANTIALIASING)) {
                    gDPSetRenderMode(gMainGfxPos++, G_RM_TEX_EDGE, G_RM_TEX_EDGE2);
                } else {
                    gDPSetRenderMode(gMainGfxPos++, G_RM_AA_TEX_EDGE, G_RM_AA_TEX_EDGE2);
                }
            } else {
                if (!(hudElement->flags & HUD_ELEMENT_FLAG_ANTIALIASING)) {
                    gDPSetRenderMode(gMainGfxPos++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
                } else {
                    gDPSetRenderMode(gMainGfxPos++, G_RM_CLD_SURF | AA_EN, G_RM_CLD_SURF2 | AA_EN);
                }
            }
            gDPSetTextureLUT(gMainGfxPos++, G_TT_RGBA16);
            gDPLoadTLUT_pal16(gMainGfxPos++, 0, paletteAddr);
            break;
        case 2:
            gDPSetRenderMode(gMainGfxPos++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
            gDPSetTextureLUT(gMainGfxPos++, G_TT_NONE);
            break;
    }

    if (hudElement->flags & HUD_ELEMENT_FLAG_FILTER_TEX) {
        gDPSetTextureFilter(gMainGfxPos++, G_TF_AVERAGE);
    } else {
        gDPSetTextureFilter(gMainGfxPos++, G_TF_POINT);
    }

    for (k = 0; k < hudElement->npc; k++) {
        const NdHudPiece* pc = &hudElement->pc[k];
        s32 uls = pc->uls, ult = pc->ult, lrs = pc->lrs, lrt = pc->lrt;
        s32 ulx = pc->ulx, uly = pc->uly, lrx = pc->lrx, lry = pc->lry;
        s32 texStartX = pc->texStartX, texStartY = pc->texStartY, tileMode = pc->tileMode;

        gDPPipeSync(gMainGfxPos++);

            switch (fmt) {
                case 0:
                    gDPSetCombineMode(gMainGfxPos++, G_CC_DECALRGBA, G_CC_DECALRGBA);
                    if (!(hudElement->flags & HUD_ELEMENT_FLAG_TRANSPARENT)) {
                        gDPSetCombineMode(gMainGfxPos++, G_CC_DECALRGBA, G_CC_DECALRGBA);
                    } else {
                        gDPSetCombineMode(gMainGfxPos++, PM_CC_01, PM_CC_02);
                    }

                    if (hudElement->flags & HUD_ELEMENT_FLAG_TRANSPARENT) {
                        gDPSetPrimColor(gMainGfxPos++, 0, 0, 0, 0, 0, hudElement->opacity);
                    }

                    if (!flipX && !flipY) {
                        gDPLoadTextureTile(gMainGfxPos++, imageAddr, G_IM_FMT_RGBA, G_IM_SIZ_32b, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                    } else {
                        gDPLoadTextureTile(gMainGfxPos++, imageAddr, G_IM_FMT_RGBA, G_IM_SIZ_32b, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                    }
                    break;
                case 1:
                    if (!dropShadow) {
                        if (hudElement->flags & HUD_ELEMENT_FLAG_TRANSPARENT) {
                            gDPSetCombineMode(gMainGfxPos++, PM_CC_2F, PM_CC_32);
                        } else {
                            gDPSetCombineMode(gMainGfxPos++, PM_CC_2F, PM_CC_32);
                        }

                        if (hudElement->flags & HUD_ELEMENT_FLAG_TRANSPARENT) {
                            gDPSetPrimColor(gMainGfxPos++, 0, 0, hudElement->tint.r, hudElement->tint.g, hudElement->tint.b, hudElement->opacity);
                        } else {
                            gDPSetPrimColor(gMainGfxPos++, 0, 0, hudElement->tint.r, hudElement->tint.g, hudElement->tint.b, 255);
                        }
                    } else {
                        gDPSetRenderMode(gMainGfxPos++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
                        gDPSetCombineMode(gMainGfxPos++, PM_CC_07, PM_CC_07);
                        gDPSetPrimColor(gMainGfxPos++, 0, 0, 40, 40, 40, 72);
                    }

                    if (!flipX && !flipY) {
                        if (!clamp) {
                            gDPLoadTextureTile_4b(gMainGfxPos++, imageAddr, G_IM_FMT_CI, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                        } else {
                            switch (tileMode) {
                                case 0:
                                    gDPLoadTextureTile_4b(gMainGfxPos++, imageAddr, G_IM_FMT_CI, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                                    break;
                                case 1:
                                    gDPLoadTextureTile_4b(gMainGfxPos++, imageAddr, G_IM_FMT_CI, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                                    break;
                                case 2:
                                    gDPLoadTextureTile_4b(gMainGfxPos++, imageAddr, G_IM_FMT_CI, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                                    break;
                                case 3:
                                    gDPLoadTextureTile_4b(gMainGfxPos++, imageAddr, G_IM_FMT_CI, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                                    break;
                            }
                        }
                    } else {
                        gDPLoadTextureTile_4b(gMainGfxPos++, imageAddr, G_IM_FMT_CI, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                    }
                    break;
                case 2:
                    gDPSetCombineMode(gMainGfxPos++, PM_CC_47, PM_CC_47);
                    gDPSetPrimColor(gMainGfxPos++, 0, 0, hudElement->tint.r, hudElement->tint.g, hudElement->tint.b, hudElement->opacity);

                    if (!flipX && !flipY) {
                        if (!clamp) {
                            switch (tileMode) {
                                case 0:
                                    gDPLoadTextureTile(gMainGfxPos++, imageAddr, G_IM_FMT_IA, G_IM_SIZ_8b, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                                    break;
                                case 1:
                                    gDPLoadTextureTile(gMainGfxPos++, imageAddr, G_IM_FMT_IA, G_IM_SIZ_8b, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                                    break;
                                case 2:
                                    gDPLoadTextureTile(gMainGfxPos++, imageAddr, G_IM_FMT_IA, G_IM_SIZ_8b, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                                    break;
                                case 3:
                                    gDPLoadTextureTile(gMainGfxPos++, imageAddr, G_IM_FMT_IA, G_IM_SIZ_8b, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                                    break;
                            }
                        } else {
                            gDPLoadTextureTile(gMainGfxPos++, imageAddr, G_IM_FMT_IA, G_IM_SIZ_8b, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                        }
                    } else {
                        gDPLoadTextureTile(gMainGfxPos++, imageAddr, G_IM_FMT_IA, G_IM_SIZ_8b, texSizeX, texSizeY, uls, ult, lrs, lrt, 0,
                                            G_TX_MIRROR | G_TX_WRAP, G_TX_MIRROR | G_TX_WRAP, masks, maskt, G_TX_NOLOD, G_TX_NOLOD);
                    }
                    break;
            }

#ifdef TARGET_3DS
#endif
            if (hudElement->flags & HUD_ELEMENT_FLAG_FILTER_TEX) {
                if (!(anchor && pm_anchor_rect(ulx * 4, uly * 4, lrx * 4, lry * 4, 0, texStartX * 32 + 16, texStartY * 32 + 16, widthScale, heightScale))) {
                    gSPScisTextureRectangle(gMainGfxPos++, ulx * 4, uly * 4, lrx * 4, lry * 4, 0, texStartX * 32 + 16, texStartY * 32 + 16, widthScale, heightScale);
                }
            } else {
                if (!(anchor && pm_anchor_rect(ulx * 4, uly * 4, lrx * 4, lry * 4, 0, texStartX * 32, texStartY * 32, widthScale, heightScale))) {
                    gSPScisTextureRectangle(gMainGfxPos++, ulx * 4, uly * 4, lrx * 4, lry * 4, 0, texStartX * 32, texStartY * 32, widthScale, heightScale);
                }
            }
    }
    gDPPipeSync(gMainGfxPos++);
}
#undef gMainGfxPos

/* NAPI2B (NATIVEAPI A1): the direct executor of a HUD element record. The record states what hud_rect_emit writes as ops: a pso (the combine and
 * render mode words of its fmt, flags and drop shadow), the texture window of each piece (nd_tex_win_desc: the tile 0 that the load ops give the
 * walker) and the rectangle words (the gSPScisTextureRectangle arithmetic). Returns 0 with the old path still to run when the record or the walker
 * state is not one it can state. The pso table is written here on core 0: no other producer of it runs on core 1 yet. */
#define HUD_CW_(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
    { ND_W0_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1), ND_W1_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) }
#define HUD_CW(a, b) HUD_CW_(a, b) /* a and b are combiner macros of 8 terms: the argument pass expands them before HUD_CW_ counts its 16 */
static int hud_rect_direct(const NdHudRec* r) { /* 0 drawn, else why the old path runs: 1 shape, 2 window, 3 walker state, 4 pso */
    static const u32 cw_decal[2] = HUD_CW(G_CC_DECALRGBA, G_CC_DECALRGBA), cw_01_02[2] = HUD_CW(PM_CC_01, PM_CC_02), cw_2f_32[2] = HUD_CW(PM_CC_2F, PM_CC_32),
                     cw_07[2] = HUD_CW(PM_CC_07, PM_CC_07), cw_47[2] = HUD_CW(PM_CC_47, PM_CC_47);
    NdBindDesc ds[ND_HUD_PC];
    NdPsoN64 pd;
    NdPso id;
    const u32* cw;
    u32 prim;
    unsigned tfmt, tsiz, wrap[4];
    s32 k, sh, stride;
    const s32 fmt = r->fmt;
    const int filt = (r->flags & HUD_ELEMENT_FLAG_FILTER_TEX) != 0, flip = r->flipX || r->flipY;
    const void* pal = NULL;
    int pal_n = 0;

    if (r->npc <= 0 || r->npc > ND_HUD_PC || fmt < 0 || fmt > 2) return 1;
    switch (fmt) {
        case 0:
            pd.oml = G_RM_XLU_SURF | G_RM_XLU_SURF2; pd.omh = G_TT_NONE;
            cw = (r->flags & HUD_ELEMENT_FLAG_TRANSPARENT) ? cw_01_02 : cw_decal;
            prim = (r->flags & HUD_ELEMENT_FLAG_TRANSPARENT) ? (u32)r->opacity : nd_direct_prim();
            tfmt = G_IM_FMT_RGBA; tsiz = G_IM_SIZ_32b; sh = 2; stride = r->texSizeX;
            wrap[0] = wrap[1] = flip ? 1u : 0u;
            break;
        case 1:
            if (r->dropShadow) {
                pd.oml = G_RM_XLU_SURF | G_RM_XLU_SURF2; cw = cw_07; prim = (40u << 24) | (40u << 16) | (40u << 8) | 72u;
            } else {
                const int aa = (r->flags & HUD_ELEMENT_FLAG_ANTIALIASING) != 0;
                if (!(r->flags & HUD_ELEMENT_FLAG_TRANSPARENT)) pd.oml = aa ? (G_RM_AA_TEX_EDGE | G_RM_AA_TEX_EDGE2) : (G_RM_TEX_EDGE | G_RM_TEX_EDGE2);
                else pd.oml = aa ? ((G_RM_CLD_SURF | AA_EN) | (G_RM_CLD_SURF2 | AA_EN)) : (G_RM_CLD_SURF | G_RM_CLD_SURF2);
                cw = cw_2f_32;
                prim = ((u32)r->tint.r << 24) | ((u32)r->tint.g << 16) | ((u32)r->tint.b << 8) | ((r->flags & HUD_ELEMENT_FLAG_TRANSPARENT) ? (u32)r->opacity : 255u);
            }
            pd.omh = G_TT_RGBA16;
            tfmt = G_IM_FMT_CI; tsiz = G_IM_SIZ_4b; sh = 1; stride = r->texSizeX >> 1;
            wrap[0] = wrap[1] = flip ? 1u : 0u;
            pal = r->paletteAddr; pal_n = 16;
            break;
        default:
            pd.oml = G_RM_XLU_SURF | G_RM_XLU_SURF2; pd.omh = G_TT_NONE; cw = cw_47;
            prim = ((u32)r->tint.r << 24) | ((u32)r->tint.g << 16) | ((u32)r->tint.b << 8) | (u32)r->opacity;
            tfmt = G_IM_FMT_IA; tsiz = G_IM_SIZ_8b; sh = 2; stride = r->texSizeX;
            wrap[0] = wrap[1] = flip ? 1u : (r->clamp ? 2u : 0u);
            break;
    }
    wrap[2] = (unsigned)r->masks; wrap[3] = (unsigned)r->maskt;
    for (k = 0; k < r->npc; k++) { /* every window first: a window the walker would leave invalid sends the record to the old path before any state is written */
        const NdHudPiece* pc = &r->pc[k];
        const s32 lu = pc->uls << sh, ll = pc->lrs << sh;
        if (!nd_tex_win_desc(&ds[k], r->imageAddr, stride, tfmt, tsiz, wrap, lu >> 2, pc->ult, ((ll - lu) >> 2) + 1, pc->lrt - pc->ult + 1,
                             pc->uls << 2, pc->ult << 2, pc->lrs << 2, pc->lrt << 2, pal, pal_n)) return 2;
    }
    pd.cw0 = cw[0]; pd.cw1 = cw[1];
    pd.omh |= filt ? G_TF_AVERAGE : G_TF_POINT;
    pd.omh_m = (3u << G_MDSFT_TEXTLUT) | (3u << G_MDSFT_TEXTFILT);
    if (!nd_direct_ready()) return 3;
    id = nd_pso_n64(&pd);
    if (!nd_direct_bind(id, prim, nd_direct_env())) return 4;
    for (k = 0; k < r->npc; k++) {
        const NdHudPiece* pc = &r->pc[k];
        const s32 xl = pc->ulx * 4, yl = pc->uly * 4, xh = pc->lrx * 4, yh = pc->lry * 4;
        const s32 s = pc->texStartX * 32 + (filt ? 16 : 0), t = pc->texStartY * 32 + (filt ? 16 : 0);
        const s32 dsdx = r->widthScale, dtdy = r->heightScale;
        /* the words of gSPScisTextureRectangle (include/PR/gbi.h) for these arguments */
        const u32 w0 = _SHIFTL(PM_SCIS_XH(xh), 12, 12) | _SHIFTL(MAX((s16)(yh), 0), 0, 12);
        const u32 w1 = _SHIFTL(PM_SCIS_XL(xl), 12, 12) | _SHIFTL(MAX((s16)(yl), 0), 0, 12);
        const u32 hs = _SHIFTL(((s) - (PM_SCIS_CLIP(xl) ? (((s16)(dsdx) < 0) ? (MAX((((s16)(xl) * (s16)(dsdx)) >> 7), 0)) : (MIN((((s16)(xl) * (s16)(dsdx)) >> 7), 0))) : 0)), 16, 16) |
                       _SHIFTL(((t) - (((yl) < 0) ? (((s16)(dtdy) < 0) ? (MAX((((s16)(yl) * (s16)(dtdy)) >> 7), 0)) : (MIN((((s16)(yl) * (s16)(dtdy)) >> 7), 0))) : 0)), 0, 16);
        const u32 hd = _SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16);
        nd_direct_tex(&ds[k]);
        nd_direct_rect(w0, w1, hs, hd);
    }
    nd_direct_end();
    return 0;
}

/* core 0: one HUD element record, drawn directly (hud_rect_direct) or, when it cannot state the record, emitted into a local list and walked as state ops
 * (at most 9 prologue units + 32 pieces of 14 + 1 = 458 units). */
static void hud_rect_run(const uint32_t* u) {
    static Gfx buf[512];
    Gfx* p = buf;
    { const int why = hud_rect_direct((const NdHudRec*)u); if (!why) return; nd_direct_fallback((unsigned)why); }
    hud_rect_emit((const NdHudRec*)u, &p, 0);
    nd_walk_state((const uint32_t*)buf, (unsigned)(p - buf));
}

void hud_element_draw_rect(HudElement* hudElement, s16 texSizeX, s16 texSizeY, s16 drawSizeX, s16 drawSizeY,
                           s16 offsetX, s16 offsetY, s32 clamp, s32 dropShadow) {
    u32 isFmtCI4;
    u32 isFmtIA8;
    s32 flipX, flipY;
    s32 fmt;
    s32 widthScale, heightScale;
    s32 texStartX, texStartY;
    u32 isLastTileX, isLastTileY;
    s32 uls, ult, lrs, lrt;
    s32 uly, lry, ulx, lrx;
    s32 masks, maskt;
    s32 screenPosOffsetScaledX, screenPosOffsetScaledY;
    IMG_PTR imageAddr;
    PAL_PTR paletteAddr;
    s16 baseX, baseY;
    s32 tileMode;
    u32 flags1, flags2;
    u16 renderPosX, renderPosY;
    s16 tempX, tempY;
    NdHudRec rec;

    imageAddr = hudElement->imageAddr;
    paletteAddr = (u16*) hudElement->paletteAddr;

    screenPosOffsetScaledX = hudElement->screenPosOffset.x * 1024;
    screenPosOffsetScaledY = hudElement->screenPosOffset.y * 1024;
    widthScale = hudElement->widthScale;
    screenPosOffsetScaledX /= widthScale;
    heightScale = hudElement->heightScale;
    screenPosOffsetScaledY /= heightScale;

    renderPosX = hudElement->worldPosOffset.x;
    renderPosY = hudElement->worldPosOffset.y;
    renderPosX += hudElement->renderPosX + screenPosOffsetScaledX;
    renderPosY += hudElement->renderPosY + screenPosOffsetScaledY;
    tempX = offsetX;
    tempY = offsetY;
    tempX += renderPosX;
    tempY += renderPosY;

    baseX = tempX;
    baseY = tempY;

    if (dropShadow) {
        baseX = tempX + 2;
        baseY = tempY + 2;
    }

    flags1 = (hudElement->flags & HUD_ELEMENT_FLAG_FMT_CI4);
    isFmtCI4 = flags1 != 0;
    flags1 = (hudElement->flags & HUD_ELEMENT_FLAG_FMT_IA8);
    isFmtIA8 = flags1 != 0;
    flags1 = (hudElement->flags & HUD_ELEMENT_FLAG_FLIPX);
    flipX = flags1 != 0;
    flags2 = (hudElement->flags & HUD_ELEMENT_FLAG_FLIPY);
    flipY = flags2 != 0;

    fmt = 0;
    if (isFmtCI4 == false && isFmtIA8 == false) {
        fmt = 0; // RGBA
    }
    if (isFmtCI4 == true && isFmtIA8 == false) {
        fmt = 1; // CI
    }
    if (isFmtCI4 == false && isFmtIA8 == true) {
        fmt = 2; // IA
    }
    if (isFmtCI4 == true && isFmtIA8 == true) {
        fmt = 2; // IA
    }

    masks = 6;
    maskt = 5;
    if (!(hudElement->flags & HUD_ELEMENT_FLAG_SCALED)) {
        switch (drawSizeX) {
            case 8:
                masks = 3;
                break;
            case 16:
                masks = 4;
                break;
            case 32:
                masks = 5;
                break;
        }

        switch (drawSizeY) {
            case 8:
                maskt = 3;
                break;
            case 16:
                maskt = 4;
                break;
            case 32:
                maskt = 5;
                break;
        }
    }
    rec.flags = hudElement->flags;
    rec.imageAddr = imageAddr;
    rec.paletteAddr = paletteAddr;
    rec.widthScale = widthScale;
    rec.heightScale = heightScale;
    rec.fmt = fmt;
    rec.masks = masks;
    rec.maskt = maskt;
    rec.flipX = flipX;
    rec.flipY = flipY;
    rec.clamp = clamp;
    rec.dropShadow = dropShadow;
    rec.npc = 0;
    rec.texSizeX = texSizeX;
    rec.texSizeY = texSizeY;
    rec.opacity = hudElement->opacity;
    rec.tint = hudElement->tint;
    rec.pad = 0;



    ult = 0;
    isLastTileY = false;
    uly = baseY;
    while (true) {
        lry = uly + 1024.0 / heightScale * 32.0;
        lrt = ult + 31;
        if (flipY) {
            texStartY = texSizeY;
        } else {
            texStartY = 0;
        }

        if (lry < 0 || uly > SCREEN_HEIGHT) {
            break;
        }

        if (lry >= SCREEN_HEIGHT) {
            s32 temp = ult + SCREEN_HEIGHT + 31;
            temp -= baseY + lrt;
            lrt = temp - 1;

            lry = SCREEN_HEIGHT;
            isLastTileY = true;
        }

        if (lrt + 1 >= texSizeY) {
            lrt = texSizeY - 1;
            if (texSizeY >= 16) { /* UI2: a 16 row image draws 16 rows (was 17) */
                lry = baseY + drawSizeY - 1;
            } else {
                lry = baseY + drawSizeY;
            }
            isLastTileY = true;
        }

        isLastTileX = false;
        uls = 0;
        ulx = baseX;
        while (true) {
            lrx = ulx + 1024.0 / widthScale * 64.0;
            lrs = uls + 63;
            if (flipX) {
                texStartX = texSizeX;
            } else {
                texStartX = 0;
            }

            if (lrx < pm_wide_left(0)  || ulx > pm_wide_right(SCREEN_WIDTH)) {
                break;
            }

            if (lrx >= pm_wide_right(SCREEN_WIDTH)) {
                s32 temp = uls + pm_wide_right(SCREEN_WIDTH) + 63;
                temp -= baseX + lrs;
                lrs = temp - 1;

                lrx = pm_wide_right(SCREEN_WIDTH);
                isLastTileX = true;
            }

            if (lrs + 1 >= texSizeX) {
                lrs = texSizeX - 1;
                if (texSizeX > 16) {
                    lrx = baseX + drawSizeX - 1;
                } else {
                    lrx = baseX + drawSizeX;
                }
                isLastTileX = true;
            }


            if (isLastTileX ) {
                tileMode = !isLastTileY;
            }
            if (!isLastTileX && !isLastTileY) {
                tileMode = 0;
            }
            if (!isLastTileX && isLastTileY) {
                tileMode = 2;
            }
            if (isLastTileX && isLastTileY) {
                tileMode = 3;
            }

            if (rec.npc >= ND_HUD_PC) {
                pm_fatal_stop("hud: element over 32 pieces");
            }
            rec.pc[rec.npc].uls = uls;
            rec.pc[rec.npc].ult = ult;
            rec.pc[rec.npc].lrs = lrs;
            rec.pc[rec.npc].lrt = lrt;
            rec.pc[rec.npc].ulx = ulx;
            rec.pc[rec.npc].uly = uly;
            rec.pc[rec.npc].lrx = lrx;
            rec.pc[rec.npc].lry = lry;
            rec.pc[rec.npc].texStartX = texStartX;
            rec.pc[rec.npc].texStartY = texStartY;
            rec.pc[rec.npc].tileMode = tileMode;
            rec.pc[rec.npc].pad = 0;
            rec.npc++;

            if (isLastTileX) {
                break;
            }
            ulx += 1024.0 / widthScale * 64.0;
            uls += 64;
        }

        if (isLastTileY) {
            break;
        }

        ult += 32;
        uly += 1024.0 / heightScale * 32.0;
    }

    if (pm_anchor_is_open()) {
        hud_rect_emit(&rec, &gMainGfxPos, 1);
    } else {
        nd_rec_napi(hud_rect_run, &rec, 8u + 6u * (unsigned)rec.npc);
    }

}

void hud_element_clear_cache(void) {
    s32 i;
    HudCacheEntry* entryRaster;
    HudCacheEntry* entryPalette;

    if (gGameStatusPtr->context == CONTEXT_WORLD) {
        gHudElements = &gHudElementsWorld;
        gHudElementCacheSize = &gHudElementCacheSizeWorld;
        gHudElementCacheTableRaster = gHudElementCacheTableRasterWorld;
        gHudElementCacheTablePalette = gHudElementCacheTablePaletteWorld;
    } else {
        gHudElements = &gHudElementsBattle;
        gHudElementCacheSize = &gHudElementCacheSizeBattle;
        gHudElementCacheTableRaster = gHudElementCacheTableRasterBattle;
        gHudElementCacheTablePalette = gHudElementCacheTablePaletteBattle;
    }

    if (gGameStatusPtr->context == CONTEXT_WORLD) {
        gHudElementCacheBuffer = general_heap_malloc(HudElemCacheCapacity);
        ASSERT(gHudElementCacheBuffer);
        gHudElementCacheBufferWorld = gHudElementCacheBuffer;
        *gHudElementCacheSize = 0;
        entryRaster = gHudElementCacheTableRaster;
        entryPalette = gHudElementCacheTablePalette;
        for (i = 0; i < MAX_HUD_CACHE_ENTRIES; i++) {
            entryRaster[i].id = -1;
            entryPalette[i].id = -1;
        }
    } else {
        if (HudElemAuxCache == nullptr) {
            gHudElementCacheBuffer = general_heap_malloc(HudElemCacheCapacity / 2);
            ASSERT(gHudElementCacheBuffer);
        } else {
            gHudElementCacheBuffer = HudElemAuxCache;
        }
        gHudElementCacheBufferBattle = gHudElementCacheBuffer;
        *gHudElementCacheSize = 0;
        entryRaster = gHudElementCacheTableRaster;
        entryPalette = gHudElementCacheTablePalette;
        for (i = 0; i < MAX_HUD_CACHE_ENTRIES; i++) {
            entryRaster[i].id = -1;
            entryPalette[i].id = -1;
        }
    }

    for (i = 0; i < ARRAY_COUNT(*gHudElements); i++) {
        (*gHudElements)[i] = nullptr;
    }

    HudElemCount = 0;
    FrameQuadIndex = 0;
    hud_element_setup_cam();
}

#if VERSION_PAL
extern Addr D_80200000;
#endif

void init_hud_element_list(void) {
    if (gGameStatusPtr->context == CONTEXT_WORLD) {
        if (gHudElementCacheBufferBattle != nullptr) {
#if VERSION_PAL
            if (gHudElementCacheBufferBattle != D_80200000) {
                general_heap_free(gHudElementCacheBufferBattle);
            }
#else
            general_heap_free(gHudElementCacheBufferBattle);
#endif
            gHudElementCacheBufferBattle = nullptr;
        }

        gHudElements = &gHudElementsWorld;
        gHudElementCacheSize = &gHudElementCacheSizeWorld;
        gHudElementCacheTableRaster = gHudElementCacheTableRasterWorld;
        gHudElementCacheTablePalette = gHudElementCacheTablePaletteWorld;
        gHudElementCacheBuffer = gHudElementCacheBufferWorld;
    } else {
        gHudElements = &gHudElementsBattle;
        gHudElementCacheSize = &gHudElementCacheSizeBattle;
        gHudElementCacheTableRaster = gHudElementCacheTableRasterBattle;
        gHudElementCacheTablePalette = gHudElementCacheTablePaletteBattle;
        gHudElementCacheBuffer = gHudElementCacheBufferBattle;
    }

    HudElemCount = 0;
    FrameQuadIndex = 0;
}

void hud_element_setup_cam(void) {
    set_cam_viewport(CAM_HUD, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    gCameras[CAM_HUD].updateMode = CAM_UPDATE_INTERP_POS;
    gCameras[CAM_HUD].needsInit = true;
    gCameras[CAM_HUD].params.interp.dist = 15551;
    gCameras[CAM_HUD].nearClip = CAM_NEAR_CLIP;
    gCameras[CAM_HUD].params.interp.pitch = 0;
    gCameras[CAM_HUD].params.interp.yaw = 0;
    gCameras[CAM_HUD].params.interp.offsetY = 0;
    gCameras[CAM_HUD].lookAt_obj_target.z = 0;
    gCameras[CAM_HUD].farClip = 0x4000;
    gCameras[CAM_HUD].bgColor[0] = 0;
    gCameras[CAM_HUD].bgColor[1] = 0;
    gCameras[CAM_HUD].bgColor[2] = 0;
    gCameras[CAM_HUD].lookAt_obj_target.x = 160.0f;
    gCameras[CAM_HUD].lookAt_obj_target.y = -120.0f;
    gCameras[CAM_HUD].vfov = 1.0f;
    gCameras[CAM_HUD].flags &= ~(CAMERA_FLAG_DISABLED | CAMERA_FLAG_LEAD_PLAYER);
}

s32 hud_element_create(HudScript* anim) {
    HudElement* hudElement;
    s32 id;

    for (id = 0; id < ARRAY_COUNT(*gHudElements); id++) {
        if ((*gHudElements)[id] == nullptr) {
            break;
        }
    }

    ASSERT(id < ARRAY_COUNT(*gHudElements));

    (*gHudElements)[id] = hudElement = heap_malloc(sizeof(*hudElement));
    HudElemCount++;

    ASSERT(hudElement != nullptr);

    hudElement->flags = HUD_ELEMENT_FLAG_INITIALIZED;
    hudElement->readPos = anim;
    if (anim == nullptr) {
        hudElement->readPos = &HES_Empty;
    }
    hudElement->updateTimer = 1;
    hudElement->drawSizePreset = -1;
    hudElement->tileSizePreset = -1;
    hudElement->renderPosX = 0;
    hudElement->renderPosY = 0;
    hudElement->loopStartPos = anim;
    hudElement->widthScale = X10(1.0f);
    hudElement->heightScale = X10(1.0f);
    hudElement->anim = hudElement->readPos;
    hudElement->uniformScale = 1.0f;
    hudElement->screenPosOffset.x = 0;
    hudElement->screenPosOffset.y = 0;
    hudElement->worldPosOffset.x = 0;
    hudElement->worldPosOffset.y = 0;
    hudElement->worldPosOffset.z = 0;
    hudElement->opacity = 255;
    hudElement->tint.r = 255;
    hudElement->tint.g = 255;
    hudElement->tint.b = 255;

    if (gGameStatusPtr->context != CONTEXT_WORLD) {
        hudElement->flags |= HUD_ELEMENT_FLAG_BATTLE;
        id |= HUD_ELEMENT_BATTLE_ID_MASK;
    }

    hud_element_load_script(hudElement, hudElement->readPos);
    while (hud_element_update(hudElement) != 0);

    return id;
}

// TODO not ideal match - should be able to put loop iterator in the loop def, but can't
void update_hud_elements(void) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(*gHudElements);) {
        HudElement* elem = (*gHudElements)[i];

        if (elem != nullptr && elem->flags && !(elem->flags & HUD_ELEMENT_FLAG_DISABLED)) {
            if (elem->flags & HUD_ELEMENT_FLAG_DELETE) {
                hud_element_free(i);
                i++;
            } else if (elem->readPos != nullptr) {
                elem->updateTimer--;
                if (elem->updateTimer == 0) {
                    while (hud_element_update(elem) != 0);
                }
                if (elem->flags & HUD_ELEMENT_FLAG_RESIZING) {
                    elem->dynamicSize[0] += elem->deltaSizeX;
                    elem->dynamicSize[1] += elem->deltaSizeY;
                }
                i++;
            } else {
                break;
            }
        } else {
            i++;
        }
    }
}

s32 hud_element_update(HudElement* hudElement) {
    HudCacheEntry* entryRaster;
    HudCacheEntry* entryPalette;
    s32 i;
    s32 drawSizePreset;
    s32 tileSizePreset;
    u8 sizePreset;
    f32 xScaled, yScaled;
    s32 imageWidth, imageHeight, drawWidth, drawHeight;
    u32 min, max;
    u32 flags;
    s32 raster, palette;
    s32 s1, s2;
    s32 arg1, arg2;
    f32 uniformScale;
    HudScript* newReadPos;

    HudTransform* hudTransform = hudElement->hudTransform;
    s32* nextPos = (s32*)hudElement->readPos;

    switch (*nextPos++) {
        case HUD_ELEMENT_OP_End:
            hudElement->updateTimer = 60;
            flags = hudElement->flags;
            hudElement->flags = flags | HUD_ELEMENT_FLAG_ANIMATION_FINISHED;
            break;
        case HUD_ELEMENT_OP_Delete:
            hudElement->updateTimer = 60;
            hudElement->flags |= HUD_ELEMENT_FLAG_DELETE;
            break;
        case HUD_ELEMENT_OP_UseIA8:
            hudElement->readPos = (HudScript*)nextPos;
            hudElement->flags |= HUD_ELEMENT_FLAG_FMT_IA8;
            return true;
        case HUD_ELEMENT_OP_SetVisible:
            hudElement->readPos = (HudScript*)nextPos;
            hudElement->flags |= HUD_ELEMENT_FLAG_FMT_CI4;
            return true;
        case HUD_ELEMENT_OP_SetHidden:
            hudElement->readPos = (HudScript*)nextPos;
            hudElement->flags &= ~HUD_ELEMENT_FLAG_FMT_CI4;
            return true;
        case HUD_ELEMENT_OP_SetFlags:
            s1 = *nextPos++;
            hudElement->readPos = (HudScript*)nextPos;
            hudElement->flags |= s1;
            return true;
        case HUD_ELEMENT_OP_ClearFlags:
            s1 = *nextPos++;
            hudElement->readPos = (HudScript*)nextPos;
            hudElement->flags &= ~s1;
            return true;
        case HUD_ELEMENT_OP_SetRGBA:
            hudElement->updateTimer = *nextPos++;
            hudElement->imageAddr = (u8*)*nextPos++;
            hudElement->readPos = (HudScript*)nextPos;

            if (hudElement->flags & HUD_ELEMENT_FLAG_MEMOFFSET) {
                hudElement->imageAddr += hudElement->memOffset;
            }

            if (hudElement->flags & HUD_ELEMENT_FLAG_RESIZING) {
                if (!(hudElement->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                    tileSizePreset = hudElement->tileSizePreset;
                    drawSizePreset = hudElement->drawSizePreset;
                    imageWidth = HudElemSizes[tileSizePreset].width;
                    imageHeight = HudElemSizes[tileSizePreset].height;
                    drawWidth = HudElemSizes[drawSizePreset].width;
                    drawHeight = HudElemSizes[drawSizePreset].height;
                } else {
                    imageWidth = hudElement->customImageSize.x;
                    imageHeight = hudElement->customImageSize.y;
                    drawWidth = hudElement->customDrawSize.x;
                    drawHeight = hudElement->customDrawSize.y;
                }

                if (!(hudElement->flags & HUD_ELEMENT_FLAG_RESIZE_DIR)) {
                    hudElement->flags |= HUD_ELEMENT_FLAG_RESIZE_DIR;
                    hudElement->dynamicSize[0] = imageWidth;
                    hudElement->dynamicSize[1] = imageHeight;
                    hudElement->deltaSizeX = ((f32)drawWidth - (f32)imageWidth) / (f32)hudElement->updateTimer;
                    hudElement->deltaSizeY = ((f32)drawHeight - (f32)imageHeight) / (f32)hudElement->updateTimer;
                } else {
                    hudElement->flags &= ~HUD_ELEMENT_FLAG_RESIZE_DIR;
                    hudElement->dynamicSize[0] = drawWidth;
                    hudElement->dynamicSize[1] = drawHeight;
                    hudElement->deltaSizeX = ((f32)imageWidth - (f32)drawWidth) / (f32)hudElement->updateTimer;
                    hudElement->deltaSizeY = ((f32)imageHeight - (f32)drawHeight) / (f32)hudElement->updateTimer;
                }
            }
            break;
        case HUD_ELEMENT_OP_SetCI:
            hudElement->updateTimer = *nextPos++;
            hudElement->imageAddr = (u8*)*nextPos++;
            hudElement->paletteAddr = (u8*)*nextPos++;
            hudElement->readPos = (HudScript*)nextPos;

            if (hudElement->flags & HUD_ELEMENT_FLAG_MEMOFFSET) {
                hudElement->imageAddr += hudElement->memOffset;
                hudElement->paletteAddr += hudElement->memOffset;
            }

            if (hudElement->flags & HUD_ELEMENT_FLAG_RESIZING) {
                if (!(hudElement->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                    tileSizePreset = hudElement->tileSizePreset;
                    drawSizePreset = hudElement->drawSizePreset;
                    imageWidth = HudElemSizes[tileSizePreset].width;
                    imageHeight = HudElemSizes[tileSizePreset].height;
                    drawWidth = HudElemSizes[drawSizePreset].width;
                    drawHeight = HudElemSizes[drawSizePreset].height;
                } else {
                    imageWidth = hudElement->customImageSize.x;
                    imageHeight = hudElement->customImageSize.y;
                    drawWidth = hudElement->customDrawSize.x;
                    drawHeight = hudElement->customDrawSize.y;
                }

                if (!(hudElement->flags & HUD_ELEMENT_FLAG_RESIZE_DIR)) {
                    hudElement->flags |= HUD_ELEMENT_FLAG_RESIZE_DIR;
                    hudElement->dynamicSize[0] = imageWidth;
                    hudElement->dynamicSize[1] = imageHeight;
                    hudElement->deltaSizeX = ((f32)drawWidth - (f32)imageWidth) / (f32)hudElement->updateTimer;
                    hudElement->deltaSizeY = ((f32)drawHeight - (f32)imageHeight) / (f32)hudElement->updateTimer;
                } else {
                    hudElement->flags &= ~HUD_ELEMENT_FLAG_RESIZE_DIR;
                    hudElement->dynamicSize[0] = drawWidth;
                    hudElement->dynamicSize[1] = drawHeight;
                    hudElement->deltaSizeX = ((f32)imageWidth - (f32)drawWidth) / (f32)hudElement->updateTimer;
                    hudElement->deltaSizeY = ((f32)imageHeight - (f32)drawHeight) / (f32)hudElement->updateTimer;
                }
            }
            break;
        case HUD_ELEMENT_OP_SetImage:
            hudElement->updateTimer = *nextPos++;

            if (!(hudElement->flags & HUD_ELEMENT_FLAG_BATTLE)) {
                entryRaster = gHudElementCacheTableRasterWorld;
                entryPalette = gHudElementCacheTablePaletteWorld;
            } else {
                entryRaster = gHudElementCacheTableRasterBattle;
                entryPalette = gHudElementCacheTablePaletteBattle;
            }

            i = 0;
            while (true) {
                if (entryRaster[i].id == *nextPos) {
                    break;
                }
                ASSERT(++i < MAX_HUD_CACHE_ENTRIES);
            }

            nextPos++;
            hudElement->imageAddr = entryRaster[i].data;

            i = 0;
            while (true) {
                if (entryPalette[i].id == *nextPos) {
                    break;
                }
                ASSERT(++i < MAX_HUD_CACHE_ENTRIES);
            }
            hudElement->paletteAddr = entryPalette[i].data;
            nextPos += 3;
            hudElement->readPos = (HudScript*)nextPos;

            if (hudElement->flags & HUD_ELEMENT_FLAG_RESIZING) {
                if (!(hudElement->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                    tileSizePreset = hudElement->tileSizePreset;
                    drawSizePreset = hudElement->drawSizePreset;
                    imageWidth = HudElemSizes[tileSizePreset].width;
                    imageHeight = HudElemSizes[tileSizePreset].height;
                    drawWidth = HudElemSizes[drawSizePreset].width;
                    drawHeight = HudElemSizes[drawSizePreset].height;
                } else {
                    imageWidth = hudElement->customImageSize.x;
                    imageHeight = hudElement->customImageSize.y;
                    drawWidth = hudElement->customDrawSize.x;
                    drawHeight = hudElement->customDrawSize.y;
                }

                if (!(hudElement->flags & HUD_ELEMENT_FLAG_RESIZE_DIR)) {
                    hudElement->flags |= HUD_ELEMENT_FLAG_RESIZE_DIR;
                    hudElement->dynamicSize[0] = imageWidth;
                    hudElement->dynamicSize[1] = imageHeight;
                    hudElement->deltaSizeX = ((f32)drawWidth - (f32)imageWidth) / (f32)hudElement->updateTimer;
                    hudElement->deltaSizeY = ((f32)drawHeight - (f32)imageHeight) / (f32)hudElement->updateTimer;
                } else {
                    hudElement->flags &= ~HUD_ELEMENT_FLAG_RESIZE_DIR;
                    hudElement->dynamicSize[0] = drawWidth;
                    hudElement->dynamicSize[1] = drawHeight;
                    hudElement->deltaSizeX = ((f32)imageWidth - (f32)drawWidth) / (f32)hudElement->updateTimer;
                    hudElement->deltaSizeY = ((f32)imageHeight - (f32)drawHeight) / (f32)hudElement->updateTimer;
                }
            }
            break;
        case HUD_ELEMENT_OP_Restart:
            hudElement->readPos = hudElement->loopStartPos;
            return true;
        case HUD_ELEMENT_OP_Loop:
            hudElement->loopStartPos = (HudScript*)nextPos;
            hudElement->readPos = (HudScript*)nextPos;
            return true;
        case HUD_ELEMENT_OP_RandomRestart:
            s1 = *nextPos++;
            s2 = *nextPos++;
            if (rand_int(s1) < s2) {
                hudElement->readPos = hudElement->loopStartPos;
            } else {
                hudElement->readPos = (HudScript*)nextPos;
            }
            return true;
        case HUD_ELEMENT_OP_SetTileSize:
            sizePreset = *nextPos++;
            hudElement->widthScale = X10(1);
            hudElement->heightScale = X10(1);
            hudElement->readPos = (HudScript*)nextPos;
            hudElement->drawSizePreset = sizePreset;
            hudElement->tileSizePreset = sizePreset;
            hudElement->flags &= ~HUD_ELEMENT_FLAG_RESIZING;
            hudElement->flags &= ~HUD_ELEMENT_FLAG_REPEATED;
            return true;
        case HUD_ELEMENT_OP_SetSizesAutoScale:
            tileSizePreset = *nextPos++;
            arg2 = *nextPos++;

            hudElement->readPos = (HudScript*)nextPos;
            hudElement->tileSizePreset = tileSizePreset;
            hudElement->drawSizePreset = arg2;

            imageWidth = HudElemSizes[tileSizePreset].width;
            imageHeight = HudElemSizes[tileSizePreset].height;
            drawWidth = HudElemSizes[arg2].width;
            drawHeight = HudElemSizes[arg2].height;

            xScaled = (f32) drawWidth / (f32) imageWidth;
            yScaled = (f32) drawHeight / (f32) imageHeight;

            xScaled = 1.0f / xScaled;
            yScaled = 1.0f / yScaled;

            hudElement->widthScale = X10(xScaled);
            hudElement->heightScale = X10(yScaled);

            hudElement->flags &= ~HUD_ELEMENT_FLAG_RESIZING;
            hudElement->flags |= HUD_ELEMENT_FLAG_REPEATED;
            return true;
        case HUD_ELEMENT_OP_SetSizesFixedScale:
            tileSizePreset = *nextPos++;
            drawSizePreset = *nextPos++;

            hudElement->widthScale = X10(1);
            hudElement->heightScale = X10(1);
            hudElement->readPos = (HudScript*)nextPos;
            hudElement->tileSizePreset = tileSizePreset;
            hudElement->drawSizePreset = drawSizePreset;
            hudElement->flags |= HUD_ELEMENT_FLAG_RESIZING;
            hudElement->flags &= ~HUD_ELEMENT_FLAG_REPEATED;
            hudElement->flags &= ~HUD_ELEMENT_FLAG_RESIZE_DIR;
            return true;
        case HUD_ELEMENT_OP_AddTexelOffsetX:
            s1 = *nextPos++;
            hudElement->readPos = (HudScript*)nextPos;
            hudElement->screenPosOffset.x += s1;
            return true;
        case HUD_ELEMENT_OP_AddTexelOffsetY:
            s2 = *nextPos++;
            if (!(hudElement->flags & HUD_ELEMENT_FLAG_FLIPY)) {
                hudElement->screenPosOffset.y += s2;
            } else {
                hudElement->screenPosOffset.y -= s2;
            }
            hudElement->readPos = (HudScript*)nextPos;
            return true;
        case HUD_ELEMENT_OP_SetTexelOffset:
            s1 = *nextPos++;
            s2 = *nextPos++;
            hudElement->screenPosOffset.x = s1;
            if (!(hudElement->flags & HUD_ELEMENT_FLAG_FLIPY)) {
                hudElement->screenPosOffset.y = s2;
            } else {
                hudElement->screenPosOffset.y = -s2;
            }
            hudElement->readPos = (HudScript*)nextPos;
            return true;
        case HUD_ELEMENT_OP_SetScale:
            uniformScale = (f32)*nextPos++;
            uniformScale /= 65536;
            hudElement->uniformScale = uniformScale;
            if (!(hudElement->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                imageWidth = HudElemSizes[hudElement->tileSizePreset].width;
                imageHeight = HudElemSizes[hudElement->tileSizePreset].height;
                drawWidth = HudElemSizes[hudElement->drawSizePreset].width;
                drawHeight = HudElemSizes[hudElement->drawSizePreset].height;
            } else {
                imageWidth = hudElement->customImageSize.x;
                imageHeight = hudElement->customImageSize.y;
                drawWidth = hudElement->customDrawSize.x;
                drawHeight = hudElement->customDrawSize.y;
            }

            hudElement->sizeX = drawWidth * uniformScale;
            hudElement->sizeY = drawHeight * uniformScale;

            xScaled = (f32) drawWidth / (f32) imageWidth * uniformScale;
            yScaled = (f32) drawHeight / (f32) imageHeight * uniformScale;

            xScaled = 1.0f / xScaled;
            yScaled = 1.0f / yScaled;

            hudElement->widthScale = X10(xScaled);
            hudElement->heightScale = X10(yScaled);

            hudElement->readPos = (HudScript*)nextPos;
            hudElement->flags &= ~HUD_ELEMENT_FLAG_RESIZING;
            hudElement->flags |= HUD_ELEMENT_FLAG_REPEATED | HUD_ELEMENT_FLAG_SCALED;
            return true;
        case HUD_ELEMENT_OP_SetAlpha:
            s1 = *nextPos++;
            hudElement->opacity = s1;
            hudElement->flags |= HUD_ELEMENT_FLAG_TRANSPARENT;
            if (hudElement->opacity == 255) {
                hudElement->flags &= ~HUD_ELEMENT_FLAG_TRANSPARENT;
            }
            hudElement->readPos = (HudScript*)nextPos;
            return true;
        case HUD_ELEMENT_OP_RandomDelay:
            s1 = *nextPos++;
            s2 = *nextPos++;
            hudElement->updateTimer = rand_int(s2 - s1) + s1;
            hudElement->readPos = (HudScript*)nextPos;
            break;
        case HUD_ELEMENT_OP_SetCustomSize:
            hudElement->customDrawSize.x = hudElement->customImageSize.x = *nextPos++;
            hudElement->customDrawSize.y = hudElement->customImageSize.y = *nextPos++;
            hudElement->readPos = (HudScript*)nextPos;
            hudElement->widthScale = X10(1);
            hudElement->heightScale = X10(1);
            hudElement->drawSizePreset = 0;
            hudElement->tileSizePreset = 0;
            hudElement->flags &= ~HUD_ELEMENT_FLAG_RESIZING;
            hudElement->flags &= ~HUD_ELEMENT_FLAG_REPEATED;
            hudElement->flags |= HUD_ELEMENT_FLAG_CUSTOM_SIZE;
            return true;
        case HUD_ELEMENT_OP_SetVariable:
            s1 = *nextPos++;
            hudElement->readPos = (HudScript*)nextPos;
            hudElement->flags &= ~HUD_ELEMENT_VARIABLE_MASK;
            hudElement->flags |= s1 << 24;
            return true;
        case HUD_ELEMENT_OP_RandomBranch:
            s1 = *nextPos++;
            newReadPos = (HudScript*)nextPos[rand_int(s1 - 1)];
            hudElement->readPos = newReadPos;
            hud_element_load_script(hudElement, newReadPos);
            return true;
        case HUD_ELEMENT_OP_PlaySound:
            arg2 = *nextPos++;
            sfx_play_sound(arg2);
            hudElement->readPos = (HudScript*)nextPos;
            return true;
        case HUD_ELEMENT_OP_SetPivot:
            arg1 = *nextPos++;
            arg2 = *nextPos++;
            hudElement->readPos = (HudScript*)nextPos;
            if (hudElement->flags & HUD_ELEMENT_FLAG_TRANSFORM) {
                hudTransform->pivot.x = arg1;
                // TODO find better match
                do {
                    hudTransform->pivot.y = arg2;
                } while (0);
            }
            return true;
        case HUD_ELEMENT_OP_op_16:
            break;
    }

    return false;
}

void render_hud_elements_backUI(void) {
    s32 i, count, j;
    s32 sortedElements[ARRAY_COUNT(*gHudElements)];
    s32 el1, el2;
    s32 texSizeX, texSizeY;
    s32 drawSizeX, drawSizeY, offsetX, offsetY;
    HudElement* hudElement;

    FrameQuadIndex++;
    if (FrameQuadIndex > 2) {
        FrameQuadIndex = 0;
    }
    gSPDisplayList(gMainGfxPos++, VIRTUAL_TO_PHYSICAL(HudElemBackInitGfx));

    count = 0;
    for (i = 0; i < ARRAY_COUNT(*gHudElements); i++) {
        hudElement = (*gHudElements)[i];
        if (hudElement != nullptr) {
            s32 flags = hudElement->flags;
            if (flags && !(flags & HUD_ELEMENT_FLAG_DISABLED)) {
                if (!(flags & (HUD_ELEMENT_FLAG_MANUAL_RENDER | HUD_ELEMENT_FLAG_TRANSFORM | HUD_ELEMENT_FLAG_INVISIBLE | HUD_ELEMENT_FLAG_HIDDEN | HUD_ELEMENT_FLAG_BATTLE_CAM))) {
                    if (!(flags & HUD_ELEMENT_FLAG_FRONTUI) && hudElement->drawSizePreset >= 0) {
                        sortedElements[count++] = i;
                    }
                }
            }
        }
    }

    for (i = 0; i < count - 1; i++) {
        for (j = i + 1; j < count; j++) {
            el1 = sortedElements[i];
            el2 = sortedElements[j];
            if ((*gHudElements)[el1]->worldPosOffset.z < (*gHudElements)[el2]->worldPosOffset.z) {
                sortedElements[i] = el2;
                sortedElements[j] = el1;
            }
        }
    }

    for (i = 0; i < count; i++) {
        hudElement = (*gHudElements)[sortedElements[i]];

        if (hudElement->readPos == nullptr) {
            break;
        }

        if (!(hudElement->flags & HUD_ELEMENT_FLAG_RESIZING)) {
            if (!(hudElement->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                texSizeX = HudElemSizes[hudElement->tileSizePreset].width;
                texSizeY = HudElemSizes[hudElement->tileSizePreset].height;
            } else {
                texSizeX = hudElement->customImageSize.x;
                texSizeY = hudElement->customImageSize.y;
            }

            if (!(hudElement->flags & HUD_ELEMENT_FLAG_SCALED)) {
                if (!(hudElement->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                    drawSizeX = HudElemSizes[hudElement->drawSizePreset].width;
                    drawSizeY = HudElemSizes[hudElement->drawSizePreset].height;
                    offsetX = -drawSizeX / 2;
                    offsetY = -drawSizeY / 2;
                } else {
                    // TODO find better match
                    do {
                    drawSizeX = hudElement->customDrawSize.x;
                    drawSizeY = hudElement->customDrawSize.y;
                    offsetX = -drawSizeX / 2;
                    } while (0);
                    offsetY = -drawSizeY / 2;

                }
            } else {
                drawSizeX = hudElement->sizeX;
                drawSizeY = hudElement->sizeY;
                offsetX = -drawSizeX / 2;
                offsetY = -drawSizeY / 2;
            }

            if (!(hudElement->flags & HUD_ELEMENT_FLAG_REPEATED)) {
                if (hudElement->flags & HUD_ELEMENT_FLAG_DROP_SHADOW) {
                    hud_element_draw_rect(hudElement, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, true, true);
                }
                hud_element_draw_rect(hudElement, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, true, false);
            } else {
                if (hudElement->flags & HUD_ELEMENT_FLAG_DROP_SHADOW) {
                    hud_element_draw_rect(hudElement, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, false, true);
                }
                hud_element_draw_rect(hudElement, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, false, false);
            }
        } else {
            f32 xScaled, yScaled;

            if (!(hudElement->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                texSizeX = HudElemSizes[hudElement->tileSizePreset].width;
                texSizeY = HudElemSizes[hudElement->tileSizePreset].height;
            } else {
                texSizeX = hudElement->customImageSize.x;
                texSizeY = hudElement->customImageSize.y;
            }

            drawSizeX = hudElement->dynamicSize[0];
            drawSizeY = hudElement->dynamicSize[1];

            offsetX = -hudElement->dynamicSize[0] / 2;
            offsetY = -hudElement->dynamicSize[1] / 2;

            xScaled = (f32) drawSizeX / (f32) texSizeX;
            yScaled = (f32) drawSizeY / (f32) texSizeY;

            xScaled = 1.0f / xScaled;
            yScaled = 1.0f / yScaled;

            hudElement->widthScale = X10(xScaled);
            hudElement->heightScale = X10(yScaled);

            if (hudElement->flags & HUD_ELEMENT_FLAG_DROP_SHADOW) {
                hud_element_draw_rect(hudElement, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, false, true);
            }
            hud_element_draw_rect(hudElement, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, false, false);
        }
    }
}

void render_hud_elements_frontUI(void) {
    s32 i, count, j;
    s32 sortedElements[ARRAY_COUNT(*gHudElements)];
    s32 el1, el2;
    s32 texSizeX, texSizeY;
    s32 drawSizeX, drawSizeY, offsetX, offsetY;
    HudElement* hudElement;

    gSPDisplayList(gMainGfxPos++, VIRTUAL_TO_PHYSICAL(HudElemFrontInitGfx));

    count = 0;
    for (i = 0; i < ARRAY_COUNT(*gHudElements); i++) {
        hudElement = (*gHudElements)[i];
        if (hudElement != nullptr) {
            s32 flags = hudElement->flags;
            if (flags && !(flags & HUD_ELEMENT_FLAG_DISABLED)) {
                if (!(flags & (HUD_ELEMENT_FLAG_MANUAL_RENDER | HUD_ELEMENT_FLAG_TRANSFORM | HUD_ELEMENT_FLAG_INVISIBLE | HUD_ELEMENT_FLAG_HIDDEN))) {
                    if ((flags & HUD_ELEMENT_FLAG_FRONTUI) && hudElement->drawSizePreset >= 0) {
                        sortedElements[count++] = i;
                    }
                }
            }
        }
    }

    for (i = 0; i < count - 1; i++) {
        for (j = i + 1; j < count; j++) {
            el1 = sortedElements[i];
            el2 = sortedElements[j];
            if ((*gHudElements)[el1]->worldPosOffset.z < (*gHudElements)[el2]->worldPosOffset.z) {
                sortedElements[i] = el2;
                sortedElements[j] = el1;
            }
        }
    }

    for (i = 0; i < count; i++) {
        hudElement = (*gHudElements)[sortedElements[i]];
        if (!(hudElement->flags & HUD_ELEMENT_FLAG_RESIZING)) {
            if (!(hudElement->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                texSizeX = HudElemSizes[hudElement->tileSizePreset].width;
                texSizeY = HudElemSizes[hudElement->tileSizePreset].height;
            } else {
                texSizeX = hudElement->customImageSize.x;
                texSizeY = hudElement->customImageSize.y;
            }

            if (!(hudElement->flags & HUD_ELEMENT_FLAG_SCALED)) {
                if (!(hudElement->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                    drawSizeX = HudElemSizes[hudElement->drawSizePreset].width;
                    drawSizeY = HudElemSizes[hudElement->drawSizePreset].height;
                    offsetX = -drawSizeX / 2;
                    offsetY = -drawSizeY / 2;
                } else {
                    // TODO find better match
                    do {
                    drawSizeX = hudElement->customDrawSize.x;
                    drawSizeY = hudElement->customDrawSize.y;
                    offsetX = -drawSizeX / 2;
                    } while (0);
                    offsetY = -drawSizeY / 2;

                }
            } else {
                drawSizeX = hudElement->sizeX;
                drawSizeY = hudElement->sizeY;
                offsetX = -drawSizeX / 2;
                offsetY = -drawSizeY / 2;
            }

            if (!(hudElement->flags & HUD_ELEMENT_FLAG_REPEATED)) {
                if (hudElement->flags & HUD_ELEMENT_FLAG_DROP_SHADOW) {
                    hud_element_draw_rect(hudElement, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, true, true);
                }
                hud_element_draw_rect(hudElement, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, true, false);
            } else {
                if (hudElement->flags & HUD_ELEMENT_FLAG_DROP_SHADOW) {
                    hud_element_draw_rect(hudElement, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, false, true);
                }
                hud_element_draw_rect(hudElement, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, false, false);
            }
        } else {
            f32 xScaled, yScaled;

            if (!(hudElement->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                texSizeX = HudElemSizes[hudElement->drawSizePreset].width;
                texSizeY = HudElemSizes[hudElement->drawSizePreset].height;
            } else {
                texSizeX = hudElement->customImageSize.x;
                texSizeY = hudElement->customImageSize.y;
            }

            drawSizeX = hudElement->dynamicSize[0];
            drawSizeY = hudElement->dynamicSize[1];

            offsetX = -hudElement->dynamicSize[0] / 2;
            offsetY = -hudElement->dynamicSize[1] / 2;

            xScaled = (f32) drawSizeX / (f32) texSizeX;
            yScaled = (f32) drawSizeY / (f32) texSizeY;

            xScaled = 1.0f / xScaled;
            yScaled = 1.0f / yScaled;

            hudElement->widthScale = X10(xScaled);
            hudElement->heightScale = X10(yScaled);

            if (hudElement->flags & HUD_ELEMENT_FLAG_DROP_SHADOW) {
                hud_element_draw_rect(hudElement, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, false, true);
            }
            hud_element_draw_rect(hudElement, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, false, false);
        }
    }
}

void render_complex_hud_element(HudElement* hudElement) {
    Matrix4f sp20;
    Matrix4f sp60;
    Matrix4f spA0;
    Matrix4f spE0;
    Matrix4f sp120;
    Matrix4f sp160;
    Matrix4f sp1A0;
    Matrix4f sp1E0;
    Matrix4f sp220;
    Matrix4f sp260;
    ImgFXTexture ifxImg;
    f32 xScaleFactor, yScaleFactor;
    s32 height, width;
    HudTransform* transform;
    s32 mode;
    u32 flags;
    IMG_PTR raster;
    PAL_PTR palette;
    u32 a0, v1;
    s32 ulx, uly, lrx, lry;
    Vtx* vtx;
    Vtx* vtx2;

    if (hudElement->flags & HUD_ELEMENT_FLAG_FILTER_TEX) {
        gDPSetTextureFilter(gMainGfxPos++, G_TF_AVERAGE);
    } else {
        gDPSetTextureFilter(gMainGfxPos++, G_TF_POINT);
    }

    if (!(hudElement->flags & HUD_ELEMENT_FLAG_RESIZING)) {
        xScaleFactor = 1.0f;
        yScaleFactor = 1.0f;
    } else {
        if (!(hudElement->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
            xScaleFactor = HudElemSizes[hudElement->tileSizePreset].width;
            yScaleFactor = HudElemSizes[hudElement->tileSizePreset].height;
        } else {
            xScaleFactor = hudElement->customImageSize.x;
            yScaleFactor = hudElement->customImageSize.y;
        }
        xScaleFactor /= hudElement->dynamicSize[0];
        xScaleFactor = 1.0f / xScaleFactor;
        yScaleFactor /= hudElement->dynamicSize[1];
        yScaleFactor = 1.0f / yScaleFactor;
    }

    if (!(hudElement->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
        width = HudElemSizes[hudElement->tileSizePreset].width;
        height = HudElemSizes[hudElement->tileSizePreset].height;
    } else {
        width = hudElement->customImageSize.x;
        height = hudElement->customImageSize.y;
    }

    transform = hudElement->hudTransform;

    guTranslateF(sp1E0, transform->pivot.x, -transform->pivot.y, 0.0f);
    guTranslateF(sp220, -transform->pivot.x, transform->pivot.y, 0.0f);
    guTranslateF(
        sp1A0,
        hudElement->renderPosX + hudElement->screenPosOffset.x + hudElement->worldPosOffset.x + transform->pos.x,
        -hudElement->renderPosY - hudElement->screenPosOffset.y + hudElement->worldPosOffset.y + transform->pos.y,
        - (hudElement->worldPosOffset.z / 10.0) + transform->pos.z
    );
    guScaleF(sp260, hudElement->uniformScale * xScaleFactor * transform->scale.x,
                    hudElement->uniformScale * yScaleFactor * transform->scale.y,
                    transform->scale.z);
    guRotateF(sp120, transform->rot.y, 0.0f, 1.0f, 0.0f);
    guRotateF(sp160, transform->rot.z, 0.0f, 0.0f, 1.0f);
    guRotateF(spE0, transform->rot.x, 1.0f, 0.0f, 0.0f);
    guMtxCatF(sp160, spE0, sp20);
    guMtxCatF(sp20, sp120, spA0);
    guMtxCatF(sp260, sp1E0, sp20);
    guMtxCatF(sp20, spA0, sp60);
    guMtxCatF(sp60, sp220, sp20);
    guMtxCatF(sp20, sp1A0, sp60);
    guMtxF2L(sp60, &gDisplayContext->matrixStack[gMatrixListPos]);
    gSPMatrix(gMainGfxPos++, VIRTUAL_TO_PHYSICAL(&gDisplayContext->matrixStack[gMatrixListPos++]),
              G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    mode = 0;
    a0 = mode;
    a0 = (hudElement->flags & HUD_ELEMENT_FLAG_FMT_CI4) > a0;
    v1 = mode < (hudElement->flags & HUD_ELEMENT_FLAG_FMT_IA8);

    if (a0 == true) {
        mode = (v1 < 1);
    }
    if (a0 == false) {
        if (v1 == true) {
            mode = 2;
        }
    }

    if (a0 == true) {
        if (v1 == a0) {
            mode = 2;
        }
    }

    { /* BTLUI2 debug switch cplxhud: mode, flags, opacity, position and scale of each complex HUD draw, 120 calls after frame 150 */
        static int cplx_sw = -1;
        static unsigned cplx_n;
        extern void pm_log_d(const char* fmt, ...);
        extern unsigned pm_frame_count(void);
        extern int pm_debug_has(const char* word);
        if (cplx_sw < 0) cplx_sw = pm_debug_has("cplxhud") ? 1 : 0;
        if (cplx_sw && pm_frame_count() > 150 && cplx_n < 120) {
            cplx_n++;
            pm_log_d("cplx f%u mode %d flags %08x op %u pos %d,%d scale %d tsc %d,%d img %08x q %d", pm_frame_count(), mode,
                     hudElement->flags, (unsigned)hudElement->opacity, (int)hudElement->renderPosX, (int)hudElement->renderPosY,
                     (int)(hudElement->uniformScale * 100), (int)(transform->scale.x * 100), (int)(transform->scale.y * 100),
                     (unsigned)(uintptr_t)hudElement->imageAddr, (int)FrameQuadIndex);
        }
    }

    switch (mode) {
        case 1:
            if (hudElement->flags & HUD_ELEMENT_FLAG_NO_FOLD) {
                if (hudElement->flags & HUD_ELEMENT_FLAG_TRANSPARENT) {
                    imgfx_update(0, IMGFX_SET_ALPHA, 255, 255, 255, hudElement->opacity, 0);
                } else {
                    imgfx_update(0, IMGFX_CLEAR, 0, 0, 0, 0, 0);
                }
            } else {
                if (hudElement->flags & HUD_ELEMENT_FLAG_TRANSPARENT) {
                    imgfx_update(transform->imgfxIdx, IMGFX_SET_ALPHA, 255, 255, 255, hudElement->opacity, 0);
                } else {
                    imgfx_update(transform->imgfxIdx, IMGFX_CLEAR, 0, 0, 0, 0, 0);
                }
            }

            ifxImg.raster = hudElement->imageAddr;
            palette = (u16*) hudElement->paletteAddr;
            ifxImg.width = width;
            ifxImg.height = height;
            ifxImg.xOffset = -width / 2;
            ifxImg.yOffset = height / 2;
            ifxImg.alpha = 255;
            ifxImg.palette = palette;

            if (hudElement->flags & HUD_ELEMENT_FLAG_NO_FOLD) {
                if (hudElement->flags & HUD_ELEMENT_FLAG_ANTIALIASING) {
                    imgfx_appendGfx_component(0, &ifxImg, IMGFX_FLAG_SKIP_ZBUF, sp60);
                } else {
                    imgfx_appendGfx_component(0, &ifxImg, IMGFX_FLAG_SKIP_ZBUF, sp60);
                }
            } else {
                imgfx_appendGfx_component(transform->imgfxIdx, &ifxImg, IMGFX_FLAG_SKIP_ZBUF, sp60);
            }
            break;
        case 2:
            transform->quadBuffers[FrameQuadIndex].vtx[0] = HudElemTemplateQuad[0];
            transform->quadBuffers[FrameQuadIndex].vtx[1] = HudElemTemplateQuad[1];
            transform->quadBuffers[FrameQuadIndex].vtx[2] = HudElemTemplateQuad[2];
            transform->quadBuffers[FrameQuadIndex].vtx[3] = HudElemTemplateQuad[3];

            vtx2 = transform->quadBuffers[FrameQuadIndex].vtx;
            vtx = vtx2;

            vtx2->v.ob[0] = -width / 2;
            vtx2->v.ob[1] = -height / 2;
            vtx2->v.ob[2] = 0;
            vtx2->v.tc[0] = 0;
            vtx2->v.tc[1] = height * 32;
            vtx2 = vtx2 + 1;
            vtx2[0].v.ob[0] = (width / 2) - 1;
            vtx2[0].v.ob[1] = -height / 2;
            vtx2[0].v.ob[2] = 0;
            vtx2[0].v.tc[0] = width * 32;
            vtx2[0].v.tc[1] = height * 32;
            vtx[2].v.ob[0] = (width / 2) - 1;
            vtx[2].v.ob[1] = (height / 2) - 1;
            vtx[2].v.ob[2] = 0;
            vtx[2].v.tc[0] = width * 32;
            vtx[2].v.tc[1] = 0;
            vtx[3].v.ob[0] = -width / 2;
            vtx[3].v.ob[1] = (height / 2) - 1;
            vtx[3].v.ob[2] = 0;
            vtx[3].v.tc[0] = 0;
            vtx[3].v.tc[1] = 0;

            gDPPipeSync(gMainGfxPos++);
            gDPSetCombineMode(gMainGfxPos++, PM_CC_47, PM_CC_47);
            gDPSetPrimColor(gMainGfxPos++, 0, 0,
                            hudElement->tint.r, hudElement->tint.g, hudElement->tint.b, hudElement->opacity);

            if (!(hudElement->flags & HUD_ELEMENT_FLAG_TRANSPARENT)) {
                if (!(hudElement->flags & HUD_ELEMENT_FLAG_ANTIALIASING)) {
                    gDPSetRenderMode(gMainGfxPos++, G_RM_TEX_EDGE, G_RM_TEX_EDGE2);
                } else {
                    gDPSetRenderMode(gMainGfxPos++, G_RM_AA_TEX_EDGE, G_RM_AA_TEX_EDGE2);
                }
            } else {
                if (!(hudElement->flags & HUD_ELEMENT_FLAG_ANTIALIASING)) {
                    gDPSetRenderMode(gMainGfxPos++, G_RM_CLD_SURF, G_RM_CLD_SURF2);
                } else {
                    gDPSetRenderMode(gMainGfxPos++, AA_EN | G_RM_CLD_SURF, AA_EN | G_RM_CLD_SURF2);
                }
            }

            gDPLoadTextureBlock(gMainGfxPos++, hudElement->imageAddr, G_IM_FMT_IA, G_IM_SIZ_8b, width, height, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                                G_TX_NOLOD, G_TX_NOLOD);

            gDPSetTextureLUT(gMainGfxPos++, G_TT_NONE);
            gSPVertex(gMainGfxPos++, &transform->quadBuffers[FrameQuadIndex], 4, 0);
            { static int ctr_sw = -1; extern void ctr_store(const void* q); extern int pm_debug_has(const char* word); /* BTLUI3: cplxtrace stores the quad for nd_geom_vtx */
              if (ctr_sw < 0) ctr_sw = pm_debug_has("cplxtrace") ? 1 : 0;
              if (ctr_sw) ctr_store(&transform->quadBuffers[FrameQuadIndex]); }
            gSP1Triangle(gMainGfxPos++, 0, 1, 2, 0);
            gSP1Triangle(gMainGfxPos++, 0, 2, 3, 0);
            break;
    }

    gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);
    gDPPipeSync(gMainGfxPos++);
}

void render_transformed_hud_elements(void) {
    s32 i, count, j;
    s32 sortedElements[ARRAY_COUNT(*gHudElements)];
    s32 el1, el2;
    HudElement* hudElement;
    s16 tempOffsetZ;
    s32 flags;
    s32 z1, z2;

    // TODO fix this terrible match
    do {
        do {
            count = 0;
            if (gCurrentCamID == CAM_HUD) {
                for (i = 0; i < ARRAY_COUNT(*gHudElements); i++) {
                    hudElement = (*gHudElements)[i];
                    if (hudElement == nullptr) {
                        continue;
                    }

                    flags = hudElement->flags;
                    if (flags == 0 || flags & HUD_ELEMENT_FLAG_DISABLED) {
                        continue;
                    }

                    if (flags & (HUD_ELEMENT_FLAG_INVISIBLE | HUD_ELEMENT_FLAG_HIDDEN)) {
                        continue;
                    }

                    if (!(flags & HUD_ELEMENT_FLAG_TRANSFORM)) {
                        continue;
                    }

                    if (flags & HUD_ELEMENT_FLAG_BATTLE_CAM) {
                        continue;
                    }

                    if (flags & HUD_ELEMENT_FLAG_FRONTUI || hudElement->drawSizePreset < 0 || flags & HUD_ELEMENT_FLAG_MANUAL_RENDER) {
                        continue;
                    }

                    // add element to list
                    sortedElements[count++] = i;
                }

                if (count != 0) {
                    for (i = 0; i < count - 1; i++) {
                        for (j = i + 1; j < count; j++) {
                            el1 = sortedElements[i];
                            el2 = sortedElements[j];
                            tempOffsetZ = (*gHudElements)[el1]->worldPosOffset.z;
                            do {} while (0);
                            if (tempOffsetZ < (*gHudElements)[el2]->worldPosOffset.z) {
                                sortedElements[i] = el2;
                                sortedElements[j] = el1;
                            }
                        }
                    }

                    gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, pm_wide ? 0 : 12, pm_wide ? 0 : 20, pm_wide ? SCREEN_WIDTH : 308, pm_wide ? SCREEN_HEIGHT : 220); /* WIDE4: a transformed HUD element (battle wheel spots, reel, beam) placed in the wide margin is drawn to the screen edge */
                    gDPPipeSync(gMainGfxPos++);
                    gSPClearGeometryMode(gMainGfxPos++, G_ZBUFFER | G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH);
                    gSPSetGeometryMode(gMainGfxPos++, G_ZBUFFER | G_SHADE | G_LIGHTING | G_SHADING_SMOOTH);
                    gSPSetLights1(gMainGfxPos++, HudElemLights);
                    gSPTexture(gMainGfxPos++, -1, -1, 0, G_TX_RENDERTILE, G_ON);
                    gDPSetAlphaCompare(gMainGfxPos++, G_AC_NONE);
                    gSPSetOtherMode(gMainGfxPos++, G_SETOTHERMODE_H, G_MDSFT_ALPHADITHER, 18, G_AD_DISABLE | G_CD_DISABLE | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_PERSP | G_CYC_1CYCLE);

                    for (i = 0; i < count; i++) {
                        FI_OPEN("HUD Elements", (uintptr_t)(*gHudElements)[sortedElements[i]]);
                        render_complex_hud_element((*gHudElements)[sortedElements[i]]);
                        FI_CLOSE();
                    }
                }
            }

            count = 0;
            if (gCurrentCamID == CAM_BATTLE) {
                for (i = 0; i < ARRAY_COUNT(*gHudElements); i++) {
                    hudElement = (*gHudElements)[i];
                    if (hudElement == nullptr) {
                        continue;
                    }

                    flags = hudElement->flags;
                    if (flags == 0 || flags & HUD_ELEMENT_FLAG_DISABLED) {
                        continue;
                    }

                    if (flags & (HUD_ELEMENT_FLAG_INVISIBLE | HUD_ELEMENT_FLAG_HIDDEN)) {
                        continue;
                    }

                    if (!(flags & HUD_ELEMENT_FLAG_TRANSFORM)) {
                        continue;
                    }

                    // different from CAM_HUD pass
                    if (!(flags & HUD_ELEMENT_FLAG_BATTLE_CAM)) {
                        continue;
                    }

                    if (flags & HUD_ELEMENT_FLAG_FRONTUI || hudElement->drawSizePreset < 0 || flags & HUD_ELEMENT_FLAG_MANUAL_RENDER) {
                        continue;
                    }

                    // add element to list
                    sortedElements[count++] = i;
                }

                if (count != 0) {
                    for (i = 0; i < count - 1; i++) {
                        for (j = i + 1; j < count; j++) {
                            el1 = sortedElements[i];
                            el2 = sortedElements[j];
                            tempOffsetZ = (*gHudElements)[el1]->worldPosOffset.z;
                            do {} while (0);
                            if ((*gHudElements)[el1]->worldPosOffset.z < (*gHudElements)[el2]->worldPosOffset.z) {
                                sortedElements[i] = el2;
                                sortedElements[j] = el1;
                            }
                        }
                    }

                    gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, pm_wide ? 0 : 12, pm_wide ? 0 : 20, pm_wide ? SCREEN_WIDTH : 308, pm_wide ? SCREEN_HEIGHT : 220); /* WIDE4: a transformed HUD element (battle wheel spots, reel, beam) placed in the wide margin is drawn to the screen edge */
                    gDPPipeSync(gMainGfxPos++);
                    gSPClearGeometryMode(gMainGfxPos++, G_ZBUFFER | G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH);
                    gSPSetGeometryMode(gMainGfxPos++, G_ZBUFFER | G_SHADE | G_LIGHTING | G_SHADING_SMOOTH);
                    gSPSetLights1(gMainGfxPos++, HudElemLights);
                    gSPTexture(gMainGfxPos++, -1, -1, 0, G_TX_RENDERTILE, G_ON);
                    gDPSetAlphaCompare(gMainGfxPos++, G_AC_NONE);
                    gSPSetOtherMode(gMainGfxPos++, G_SETOTHERMODE_H, G_MDSFT_ALPHADITHER, 18, G_AD_DISABLE | G_CD_DISABLE | G_CK_NONE | G_TC_FILT | G_TF_BILERP | G_TT_NONE | G_TL_TILE | G_TD_CLAMP | G_TP_PERSP | G_CYC_1CYCLE);

                    for (i = 0; i < count; i++) {
                        FI_OPEN("HUD Elements", (uintptr_t)(*gHudElements)[sortedElements[i]]);
                        render_complex_hud_element((*gHudElements)[sortedElements[i]]);
                        FI_CLOSE();
                    }
                }
            }
        } while (0);
    } while (0);
}

void immediately_render_complex_hud_element(s32 elemID, s32 arg1, s32 camID) {
    Camera* camera = &gCameras[camID];

    if (arg1 > 0) {
        if (camera->flags == 0 || (camera->flags & CAMERA_FLAG_DISABLED)) {
            return;
        }

        gCurrentCamID = camID;

        gSPViewport(gMainGfxPos++, PM_VP(&camera->vp));
        gSPClearGeometryMode(gMainGfxPos++, G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN |
                                                G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH);
        gSPTexture(gMainGfxPos++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        gDPPipelineMode(gMainGfxPos++, G_PM_NPRIMITIVE);
        gDPSetTextureLUT(gMainGfxPos++, G_TT_NONE);
        gDPSetTextureFilter(gMainGfxPos++, G_TF_BILERP);
        gDPSetCombineMode(gMainGfxPos++, G_CC_SHADE, G_CC_SHADE);
        gDPSetRenderMode(gMainGfxPos++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
        gSPClipRatio(gMainGfxPos++, FRUSTRATIO_2);
        gDPPipeSync(gMainGfxPos++);
        //clear Z buffer inside camera viewport
        gDPSetCycleType(gMainGfxPos++, G_CYC_FILL);
        gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, osVirtualToPhysical(nuGfxZBuffer));
        gDPSetFillColor(gMainGfxPos++, GPACK_ZDZ(G_MAXFBZ, 0)<<16 | GPACK_ZDZ(G_MAXFBZ, 0));
        gDPFillRectangle(gMainGfxPos++, camera->viewportStartX, camera->viewportStartY,
                         camera->viewportStartX + camera->viewportW - 1,
                         camera->viewportStartY + camera->viewportH - 1);
        gDPPipeSync(gMainGfxPos++);

        gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, osVirtualToPhysical(nuGfxCfb_ptr));
        gDPPipeSync(gMainGfxPos++);

        guOrthoF(camera->mtxPerspective, 0.0f, 320.0f, -240.0f, 0.0f, -1000.0f, 1000.0f, 1.0f);
        guMtxF2L(camera->mtxPerspective, &gDisplayContext->camPerspMatrix[gCurrentCamID]);

        gSPMatrix(gMainGfxPos++, &gDisplayContext->camPerspMatrix[gCurrentCamID], G_MTX_NOPUSH | G_MTX_LOAD |
                                                                                    G_MTX_PROJECTION);
        gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, pm_wide ? 0 : 12, pm_wide ? 0 : 20, pm_wide ? SCREEN_WIDTH : 308, pm_wide ? SCREEN_HEIGHT : 220); /* WIDE4: a transformed HUD element (battle wheel spots, reel, beam) placed in the wide margin is drawn to the screen edge */
        gDPPipeSync(gMainGfxPos++);
        gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
        gSPClearGeometryMode(gMainGfxPos++, G_ZBUFFER | G_SHADE | G_CULL_BOTH | G_FOG | G_LIGHTING |
                                                G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR | G_LOD | G_SHADING_SMOOTH);
        gSPSetGeometryMode(gMainGfxPos++, G_ZBUFFER | G_SHADE | G_LIGHTING | G_SHADING_SMOOTH);
        gSPSetLights1(gMainGfxPos++, HudElemLights);
        gSPTexture(gMainGfxPos++, -1, -1, 0, G_TX_RENDERTILE, G_ON);
        gDPSetTextureLOD(gMainGfxPos++, G_TL_TILE);
        gDPSetTexturePersp(gMainGfxPos++, G_TP_PERSP);
        gDPSetColorDither(gMainGfxPos++, G_CD_DISABLE);
        gDPSetTextureDetail(gMainGfxPos++, G_TD_CLAMP);
        gDPSetTextureConvert(gMainGfxPos++, G_TC_FILT);
        gDPSetCombineKey(gMainGfxPos++, G_CK_NONE);
        gDPSetAlphaCompare(gMainGfxPos++, G_AC_NONE);
    }

    if (elemID >= 0) {
        HudElement* elem;

        elemID &= ~HUD_ELEMENT_BATTLE_ID_MASK;
        elem = (*gHudElements)[elemID];

        if (elem == nullptr) {
            return;
        }

        if (elem->flags == 0 || elem->flags & HUD_ELEMENT_FLAG_DISABLED) {
            return;
        }

        if (elem->flags & (HUD_ELEMENT_FLAG_INVISIBLE | HUD_ELEMENT_FLAG_HIDDEN)) {
            return;
        }

        if (!(elem->flags & HUD_ELEMENT_FLAG_TRANSFORM)) {
            return;
        }

        if (elem->flags & HUD_ELEMENT_FLAG_FRONTUI || elem->drawSizePreset < 0 || !(elem->flags & HUD_ELEMENT_FLAG_MANUAL_RENDER)) {
            return;
        }

        render_complex_hud_element(elem);
    }
}

void hud_element_draw_complex_hud_first(s32 id) {
    immediately_render_complex_hud_element(id, 1, CAM_HUD);
}

void hud_element_draw_complex_hud_next(s32 id) {
    immediately_render_complex_hud_element(id, 0, CAM_HUD);
}

void hud_element_draw_complex_battle_first(s32 id) {
    immediately_render_complex_hud_element(id, 1, CAM_BATTLE);
}

void hud_element_draw_complex_battle_next(s32 id) {
    immediately_render_complex_hud_element(id, 0, CAM_BATTLE);
}

void draw_hud_element_internal(s32 id, s32 clipMode) {
    HudElement* elem = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];
    s32 texSizeX, texSizeY;
    s32 drawSizeX, drawSizeY;
    s32 offsetX, offsetY;
    s32 preset;

    if (elem->flags && !(elem->flags & HUD_ELEMENT_FLAG_DISABLED)) {
        if (!(elem->flags & (HUD_ELEMENT_FLAG_INVISIBLE | HUD_ELEMENT_FLAG_HIDDEN)) && (elem->drawSizePreset >= 0)) {
            if (clipMode != HUD_ELEMENT_DRAW_NEXT) {
                if (clipMode == HUD_ELEMENT_DRAW_FIRST_WITH_CLIPPING) {
                    gDPSetScissor(gMainGfxPos++, G_SC_NON_INTERLACE, pm_wide ? 0 : 12, pm_wide ? 0 : 20, pm_wide ? SCREEN_WIDTH : SCREEN_WIDTH - 12, pm_wide ? SCREEN_HEIGHT : SCREEN_HEIGHT - 20) /* PT58: the wide view is the whole 240 rows */;
                }
                gDPPipeSync(gMainGfxPos++);
                gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
                gDPSetTexturePersp(gMainGfxPos++, G_TP_NONE);
                gDPSetTextureLOD(gMainGfxPos++, G_TL_TILE);
                gDPSetTextureDetail(gMainGfxPos++, G_TD_CLAMP);
                gDPSetTextureConvert(gMainGfxPos++, G_TC_FILT);
                gDPSetCombineKey(gMainGfxPos++, G_CK_NONE);
                gDPSetAlphaCompare(gMainGfxPos++, G_AC_NONE);
                gDPNoOp(gMainGfxPos++);
                gDPSetColorDither(gMainGfxPos++, G_CD_DISABLE);
                gDPSetAlphaDither(gMainGfxPos++, G_AD_DISABLE);
                gSPTexture(gMainGfxPos++, -1, -1, 0, G_TX_RENDERTILE, G_ON);
            }

            if (!(elem->flags & HUD_ELEMENT_FLAG_RESIZING)) {
                if (!(elem->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                    preset = elem->tileSizePreset;
                    texSizeX = HudElemSizes[preset].width;
                    texSizeY = HudElemSizes[preset].height;
                } else {
                    texSizeX = elem->customImageSize.x;
                    texSizeY = elem->customImageSize.y;
                }

                if (!(elem->flags & HUD_ELEMENT_FLAG_SCALED)) {
                    if (!(elem->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                        drawSizeX = HudElemSizes[elem->drawSizePreset].width;
                        drawSizeY = HudElemSizes[elem->drawSizePreset].height;
                    } else {
                        drawSizeX = elem->customDrawSize.x;
                        drawSizeY = elem->customDrawSize.y;
                    }
                } else {
                    drawSizeX = elem->sizeX;
                    drawSizeY = elem->sizeY;
                }

                do {
                    offsetX = -drawSizeX / 2;
                    offsetY = -drawSizeY / 2;
                } while (0); // required to match

                if (!(elem->flags & HUD_ELEMENT_FLAG_REPEATED)) {
                    if (elem->flags & HUD_ELEMENT_FLAG_DROP_SHADOW) {
                        hud_element_draw_rect(elem, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, true, true);
                    }
                    hud_element_draw_rect(elem, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, true, false);
                } else {
                    if (elem->flags & HUD_ELEMENT_FLAG_DROP_SHADOW) {
                        hud_element_draw_rect(elem, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, false, true);
                    }
                    hud_element_draw_rect(elem, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, false, false);
                }
            } else {
                f32 xScaled, yScaled;

                if (!(elem->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
                    preset = elem->tileSizePreset;
                    texSizeX = HudElemSizes[preset].width;
                    texSizeY = HudElemSizes[preset].height;
                } else {
                    texSizeX = elem->customImageSize.x;
                    texSizeY = elem->customImageSize.y;
                }

                drawSizeX = elem->dynamicSize[0];
                drawSizeY = elem->dynamicSize[1];

                offsetX = -elem->dynamicSize[0] / 2;
                offsetY = -elem->dynamicSize[1] / 2;

                xScaled = (f32) drawSizeX / (f32) texSizeX;
                yScaled = (f32) drawSizeY / (f32) texSizeY;

                xScaled = 1.0f / xScaled;
                yScaled = 1.0f / yScaled;

                elem->widthScale = X10(xScaled);
                elem->heightScale = X10(yScaled);

                if (elem->flags & HUD_ELEMENT_FLAG_DROP_SHADOW) {
                    hud_element_draw_rect(elem, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, false, true);
                }
                hud_element_draw_rect(elem, texSizeX, texSizeY, drawSizeX, drawSizeY, offsetX, offsetY, false, false);
            }
        }
    }
}

void hud_element_draw_clipped(s32 id) {
    draw_hud_element_internal(id, HUD_ELEMENT_DRAW_FIRST_WITH_CLIPPING);
}

void hud_element_draw_next(s32 id) {
    draw_hud_element_internal(id, HUD_ELEMENT_DRAW_NEXT);
}

void hud_element_draw_without_clipping(s32 id) {
    draw_hud_element_internal(id, HUD_ELEMENT_DRAW_FIRST_WITHOUT_CLIPPING);
}

void hud_element_set_script(s32 id, HudScript* anim) {
    HudElement* hudElement = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];

    if (anim == nullptr) {
        anim = &HES_Empty;
    }

    hudElement->updateTimer = 1;
    hudElement->widthScale = X10(1.0f);
    hudElement->heightScale = X10(1.0f);
    hudElement->readPos = anim;
    hudElement->anim = anim;
    hudElement->loopStartPos = anim;
    hudElement->screenPosOffset.x = 0;
    hudElement->screenPosOffset.y = 0;
    hudElement->worldPosOffset.x = 0;
    hudElement->worldPosOffset.y = 0;
    hudElement->flags &= ~HUD_ELEMENT_FLAG_ANIMATION_FINISHED;
    hudElement->uniformScale = 1.0f;
    hudElement->flags &= ~(HUD_ELEMENT_FLAG_SCALED | HUD_ELEMENT_FLAG_TRANSPARENT | HUD_ELEMENT_FLAG_RESIZING | HUD_ELEMENT_FLAG_REPEATED);
    hud_element_load_script(hudElement, anim);

    while (hud_element_update(hudElement) != 0) {}
}

HudScript* hud_element_get_script(s32 id) {
    return (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK]->anim;
}

HudElement* get_hud_element(s32 id) {
    return (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];
}

void hud_element_free(s32 id) {
    if ((*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK]->flags & HUD_ELEMENT_FLAG_TRANSFORM) {
        hud_element_free_transform(id & ~HUD_ELEMENT_BATTLE_ID_MASK);
    }

    heap_free((*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK]);
    (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK] = nullptr;
    HudElemCount--;
}

void hud_element_set_render_pos(s32 id, s32 x, s32 y) {
    HudElement* hudElement = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];

    hudElement->renderPosX = x;
    hudElement->renderPosY = y;
}

void hud_element_get_render_pos(s32 id, s32* x, s32* y) {
    HudElement* hudElement = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];

    *x = hudElement->renderPosX;
    *y = hudElement->renderPosY;
}

void hud_element_set_render_depth(s32 id, s32 z) {
    (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK]->worldPosOffset.z = z;
}

void hud_element_set_flags(s32 id, s32 flags) {
    (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK]->flags |= flags;
}

void hud_element_clear_flags(s32 id, s32 flags) {
    (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK]->flags &= ~flags;
}

void ALT_clear_hud_element_cache(void) {
    s32 i;

    if (gGameStatusPtr->context == CONTEXT_WORLD) {
        heap_free(gHudElementCacheBuffer);
        gHudElementCacheBuffer = heap_malloc(HudElemCacheCapacity);
        ASSERT(gHudElementCacheBuffer);
        gHudElementCacheBufferWorld = gHudElementCacheBuffer;
        *gHudElementCacheSize = 0;
        gHudElementCacheSize = &gHudElementCacheSizeWorld;
        gHudElementCacheTableRaster = gHudElementCacheTableRasterWorld;
        gHudElementCacheTablePalette = gHudElementCacheTablePaletteWorld;
        gHudElementCacheBuffer = gHudElementCacheBufferWorld;

        for (i = 0; i < MAX_HUD_CACHE_ENTRIES; i++) {
            gHudElementCacheTableRasterWorld[i].id = -1;
            gHudElementCacheTablePaletteWorld[i].id = -1;
        }
    } else {
        if (HudElemAuxCache == nullptr) {
            heap_free(gHudElementCacheBuffer);
            gHudElementCacheBuffer = heap_malloc(HudElemCacheCapacity / 2);
            ASSERT(gHudElementCacheBuffer);
        } else {
            gHudElementCacheBuffer = HudElemAuxCache;
        }
        gHudElementCacheBufferBattle = gHudElementCacheBuffer;
        *gHudElementCacheSize = 0;
        gHudElementCacheSize = &gHudElementCacheSizeBattle;
        gHudElementCacheTableRaster = gHudElementCacheTableRasterBattle;
        gHudElementCacheTablePalette = gHudElementCacheTablePaletteBattle;
        gHudElementCacheBuffer = gHudElementCacheBufferBattle;

        for (i = 0; i < MAX_HUD_CACHE_ENTRIES; i++) {
            gHudElementCacheTableRasterBattle[i].id = -1;
            gHudElementCacheTablePaletteBattle[i].id = -1;
        }
    }
}

void hud_element_set_scale(s32 index, f32 scale) {
    HudElement* elem = (*gHudElements)[index & ~HUD_ELEMENT_BATTLE_ID_MASK];
    s32 drawSizeX;
    s32 drawSizeY;
    s32 imgSizeX;
    s32 imgSizeY;
    f32 xScaled, yScaled;

    elem->uniformScale = scale;
    if (!(elem->flags & HUD_ELEMENT_FLAG_CUSTOM_SIZE)) {
        imgSizeX = HudElemSizes[elem->tileSizePreset].width;
        imgSizeY = HudElemSizes[elem->tileSizePreset].height;
        drawSizeX = HudElemSizes[elem->drawSizePreset].width;
        drawSizeY = HudElemSizes[elem->drawSizePreset].height;
    } else {
        imgSizeX = elem->customImageSize.x;
        imgSizeY = elem->customImageSize.y;
        drawSizeX = elem->customDrawSize.x;
        drawSizeY = elem->customDrawSize.y;
    }
    elem->sizeX = drawSizeX * scale;
    elem->sizeY = drawSizeY * scale;
    elem->flags &= ~HUD_ELEMENT_FLAG_RESIZING;
    elem->flags |= HUD_ELEMENT_FLAG_REPEATED | HUD_ELEMENT_FLAG_SCALED;

    xScaled = ((f32) drawSizeX / (f32) imgSizeX) * scale;
    yScaled = ((f32) drawSizeY / (f32) imgSizeY) * scale;

    xScaled = 1.0f / xScaled;
    yScaled = 1.0f / yScaled;

    elem->widthScale = X10(xScaled);
    elem->heightScale = X10(yScaled);
}

void hud_element_use_preset_size(s32 id, s8 sizePreset) {
    HudElement* hudElement = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];

    hudElement->widthScale = X10(1.0f);
    hudElement->heightScale = X10(1.0f);
    hudElement->tileSizePreset = sizePreset;
    hudElement->drawSizePreset = sizePreset;
    hudElement->uniformScale = 1.0f;
    hudElement->flags &= ~HUD_ELEMENT_FLAG_RESIZING;
    hudElement->flags &= ~(HUD_ELEMENT_FLAG_SCALED | HUD_ELEMENT_FLAG_REPEATED);
}

s32 hud_element_get_variable(s32 id) {
    return ((*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK]->flags >> 24) & 0xF;
}

void hud_element_set_variable(s32 id, s32 arg1) {
    HudElement* hudElement = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];

    hudElement->flags &= ~HUD_ELEMENT_VARIABLE_MASK;
    hudElement->flags |= arg1 << 24;
}

void hud_element_set_alpha(s32 id, s32 opacity) {
    HudElement* hudElement = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];

    hudElement->flags |= HUD_ELEMENT_FLAG_TRANSPARENT;
    hudElement->opacity = opacity;

    if (opacity == 255) {
        hudElement->flags &= ~HUD_ELEMENT_FLAG_TRANSPARENT;
    }
}

void hud_element_set_tint(s32 id, s32 r, s32 g, s32 b) {
    HudElement* hudElement = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];

    hudElement->tint.r = r;
    hudElement->tint.g = g;
    hudElement->tint.b = b;
}

void hud_element_create_transform_A(s32 id) {
    HudElement* element = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];
    HudTransform* transform = general_heap_malloc(sizeof(*transform));

    element->hudTransform = transform;
    ASSERT(transform != nullptr);
    element->flags |= HUD_ELEMENT_FLAG_TRANSFORM;
    transform->imgfxIdx = imgfx_get_free_instances(1);
    transform->pos.x = 0.0f;
    transform->pos.y = 0.0f;
    transform->pos.z = 0.0f;
    transform->rot.x = 0.0f;
    transform->rot.y = 0.0f;
    transform->rot.z = 0.0f;
    transform->scale.x = 1.0f;
    transform->scale.y = 1.0f;
    transform->scale.z = 1.0f;
    transform->pivot.x = 0;
    transform->pivot.y = 0;
    hud_element_setup_cam();
}

void hud_element_create_transform_B(s32 id) {
    HudElement* element = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];
    HudTransform* transform = general_heap_malloc(sizeof(*transform));

    element->hudTransform = transform;
    ASSERT(transform != nullptr);
    element->flags |= HUD_ELEMENT_FLAG_TRANSFORM | HUD_ELEMENT_FLAG_NO_FOLD;
    transform->imgfxIdx = 0;
    transform->pos.x = 0.0f;
    transform->pos.y = 0.0f;
    transform->pos.z = 0.0f;
    transform->rot.x = 0.0f;
    transform->rot.y = 0.0f;
    transform->rot.z = 0.0f;
    transform->scale.x = 1.0f;
    transform->scale.y = 1.0f;
    transform->scale.z = 1.0f;
    hud_element_setup_cam();
}

void hud_element_create_transform_C(s32 id) {
    HudElement* element = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];
    HudTransform* transform = general_heap_malloc(sizeof(*transform));

    element->hudTransform = transform;
    ASSERT(transform != nullptr);
    element->flags |= HUD_ELEMENT_FLAG_BATTLE_CAM | HUD_ELEMENT_FLAG_NO_FOLD | HUD_ELEMENT_FLAG_TRANSFORM;
    transform->imgfxIdx = 0;
    transform->pos.x = 0.0f;
    transform->pos.y = 0.0f;
    transform->pos.z = 0.0f;
    transform->rot.x = 0.0f;
    transform->rot.y = 0.0f;
    transform->rot.z = 0.0f;
    transform->scale.x = 1.0f;
    transform->scale.y = 1.0f;
    transform->scale.z = 1.0f;
    hud_element_setup_cam();
}

void hud_element_free_transform(s32 id) {
    HudElement* hudElement = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];
    HudTransform* hudTransform = hudElement->hudTransform;

    if (!(hudElement->flags & HUD_ELEMENT_FLAG_NO_FOLD)) {
        imgfx_release_instance(hudTransform->imgfxIdx);
    }

    heap_free(hudElement->hudTransform);
    hudElement->hudTransform = nullptr;
    hudElement->flags &= ~(HUD_ELEMENT_FLAG_BATTLE_CAM | HUD_ELEMENT_FLAG_NO_FOLD | HUD_ELEMENT_FLAG_TRANSFORM);
}

void hud_element_set_transform_pos(s32 id, f32 x, f32 y, f32 z) {
    HudElement* element = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];
    HudTransform* transform = element->hudTransform;

    if (element->flags & HUD_ELEMENT_FLAG_TRANSFORM) {
        transform->pos.x = x;
        transform->pos.y = y;
        transform->pos.z = z;
    }
}

void hud_element_set_transform_scale(s32 id, f32 x, f32 y, f32 z) {
    HudElement* element = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];
    HudTransform* transform = element->hudTransform;

    if (element->flags & HUD_ELEMENT_FLAG_TRANSFORM) {
        transform->scale.x = x;
        transform->scale.y = y;
        transform->scale.z = z;
    }
}

void hud_element_set_transform_rotation(s32 id, f32 x, f32 y, f32 z) {
    HudElement* element = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];
    HudTransform* transform = element->hudTransform;

    if (element->flags & HUD_ELEMENT_FLAG_TRANSFORM) {
        transform->rot.x = x;
        transform->rot.y = y;
        transform->rot.z = z;
    }
}

void hud_element_set_transform_rotation_pivot(s32 id, s32 dx, s32 dy) {
    HudElement* element = (*gHudElements)[id & ~HUD_ELEMENT_BATTLE_ID_MASK];
    HudTransform* transform = element->hudTransform;

    if (element->flags & HUD_ELEMENT_FLAG_TRANSFORM) {
        transform->pivot.x = dx;
        transform->pivot.y = dy;
    }
}

void copy_world_hud_element_ref_to_battle(s32 worldID, s32 battleID) {
    gHudElementsBattle[battleID & ~HUD_ELEMENT_BATTLE_ID_MASK] = gHudElementsWorld[worldID & ~HUD_ELEMENT_BATTLE_ID_MASK];
}

void hud_element_set_aux_cache(void* base, s32 size) {
    HudElemAuxCache = (u8*)base;
    if (base == nullptr) {
        HudElemCacheCapacity = 0x11000;
    } else {
        HudElemCacheCapacity = size;
    }
}

/* F06-4: screen anchor (PaperBoat ScreenAnchorPatches). Inside an anchor a texture rectangle becomes a quad under a matrix
 * that frame interpolation can move. Only with fps60: without it begin does nothing and every rectangle stays a rectangle. */
extern int pm_interp_on;
static s32 sAnchorOpen;
static s32 sAnchorX;
static s32 sAnchorY;
static Vp sAnchorViewport = { .vp = { .vscale = { SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0 }, .vtrans = { SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, G_MAXZ / 2, 0 } } };

void pm_anchor_begin(const void* key, unsigned idx, int x, int y) {
    if (pm_interp_on <= 0) {
        return;
    }
    FI_OPEN(key, idx);
    guOrtho(&gDisplayContext->matrixStack[gMatrixListPos], 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, -1.0f, 1.0f, 1.0f);
    gSPMatrix(gMainGfxPos++, &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    guTranslate(&gDisplayContext->matrixStack[gMatrixListPos], x, y, 0.0f);
    gSPMatrix(gMainGfxPos++, &gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPViewport(gMainGfxPos++, &sAnchorViewport);
    gSPLoadGeometryMode(gMainGfxPos++, 0);
    gSPTexture(gMainGfxPos++, -1, -1, 0, G_TX_RENDERTILE, G_ON);
    sAnchorOpen = true;
    sAnchorX = x;
    sAnchorY = y;
}

void pm_anchor_end(void) {
    if (!sAnchorOpen) {
        return;
    }
    gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);
    FI_CLOSE();
    sAnchorOpen = false;
}

int pm_anchor_rect(int ulx, int uly, int lrx, int lry, int tile, int s, int t, int dsdx, int dtdy) {
    const s32 slots = (4 * sizeof(Vtx) + sizeof(Gfx) - 1) / sizeof(Gfx);
    Vtx_t* vtx;
    s16 corners[4][4];
    s32 i;

    if (!sAnchorOpen) {
        return false;
    }
    vtx = (Vtx_t*) nd_rec_data(slots * sizeof(Gfx)); /* DIRECT D9: the corner vertices are stream data */

    corners[0][0] = corners[3][0] = (ulx >> 2) - sAnchorX;
    corners[0][1] = corners[1][1] = (uly >> 2) - sAnchorY;
    corners[1][0] = corners[2][0] = (lrx >> 2) - sAnchorX;
    corners[2][1] = corners[3][1] = (lry >> 2) - sAnchorY;
    corners[0][2] = corners[3][2] = s;
    corners[0][3] = corners[1][3] = t;
    corners[1][2] = corners[2][2] = ((s << 7) + dsdx * (lrx - ulx)) >> 7;
    corners[2][3] = corners[3][3] = ((t << 7) + dtdy * (lry - uly)) >> 7;
    for (i = 0; i < 4; i++) {
        vtx[i].ob[0] = corners[i][0];
        vtx[i].ob[1] = corners[i][1];
        vtx[i].ob[2] = 0;
        vtx[i].flag = 0;
        vtx[i].tc[0] = corners[i][2];
        vtx[i].tc[1] = corners[i][3];
        vtx[i].cn[0] = vtx[i].cn[1] = vtx[i].cn[2] = vtx[i].cn[3] = 255;
    }
    gSPVertex(gMainGfxPos++, vtx, 4, 0);
    gSP2Triangles(gMainGfxPos++, 0, 3, 1, 0, 3, 2, 1, 0);
    return true;
}

int pm_anchor_is_open(void) { return sAnchorOpen; }
