/* old_mtg.c: the frozen gbi-macro form of make_texture_gfx and SolidCombineModes (src/model.c before NMDL0). The host test nmdl0.sh compares it
 * with the O-form function extracted from src/model.c, unit for unit (the sync units dropped). Never edit it. */
#include "ultra64.h"
#include "variables.h"
#include "pm_port.h"
#include "gbi_custom.h"
#include "model.h"
#include "macros.h"
#include "nd_gbi.h"
// supported values for auxCombineType
enum {
    AUX_COMBINE_0               = 0,
    AUX_COMBINE_1               = 1,
    AUX_COMBINE_2               = 2,
    AUX_COMBINE_3               = 3,
    AUX_COMBINE_4               = 4,
    AUX_COMBINE_5               = 5,
    AUX_COMBINE_6               = 6,
    AUX_COMBINE_7               = 7,
    AUX_COMBINE_8               = 8,
    AUX_COMBINE_9               = 9,
    AUX_COMBINE_A               = 10,
};

// supported values for auxCombineSubType
// different subtypes are only supported for textures with EXTRA_TILE_NONE
enum {
    AUX_COMBINE_SUB_0           = 0, // multiply TEX * SHADE for color, use TEX for alpha
    AUX_COMBINE_SUB_1           = 1, // lerp from TEX to SHADE based on TEX alpha
    AUX_COMBINE_SUB_2           = 2, // TEX only, shade is ignored
    AUX_COMBINE_SUB_COUNT       = 3,
};

// different methods for combining main/aux texture maps
// derives from auxCombineType and auxCombineSubType
enum {
    // no texture
    TEX_COMBINE_NOTEX           = 0,

    // extra tile mode = EXTRA_TILE_NONE
    TEX_COMBINE_MAIN_ONLY_0     = 1,
    TEX_COMBINE_MAIN_ONLY_1     = 2,
    TEX_COMBINE_MAIN_ONLY_2     = 3,

    // extra tile mode = EXTRA_TILE_MIPMAPS
    TEX_COMBINE_MIPMAPS_0       = 4,
    TEX_COMBINE_MIPMAPS_1       = 5,
    TEX_COMBINE_MIPMAPS_2       = 6,

    // extra tile mode = EXTRA_TILE_AUX_SAME_AS_MAIN
    TEX_COMBINE_AUX_SHARED_0    = 7,
    TEX_COMBINE_AUX_SHARED_1    = 8,
    TEX_COMBINE_AUX_SHARED_2    = 9,

    // extra tile mode = EXTRA_TILE_AUX_INDEPENDENT
    // NOTE: unused; copy of TEX_COMBINE_AUX_SHARED; may not work properly
    TEX_COMBINE_AUX_IND_0       = 10,
    TEX_COMBINE_AUX_IND_1       = 11,
    TEX_COMBINE_AUX_IND_2       = 12,

    // special types selected by auxCombineType (these ignore auxCombineSubType)
    TEX_COMBINE_3               = 13,
    TEX_COMBINE_4               = 14,
    TEX_COMBINE_5               = 15,
    TEX_COMBINE_6               = 16,
    TEX_COMBINE_7               = 17,
    TEX_COMBINE_8               = 18,
    TEX_COMBINE_9               = 19,
    TEX_COMBINE_A               = 20,
};

enum {
    TINT_COMBINE_NONE           = 0,
    TINT_COMBINE_FOG            = 1,
    TINT_COMBINE_SHROUD         = 2,
    TINT_COMBINE_DEPTH          = 3,
    TINT_COMBINE_REMAP          = 4,
};

Gfx old_SolidCombineModes[][5] = {
    [TEX_COMBINE_NOTEX] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_SHADE, G_CC_PASS2),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(G_CC_SHADE, G_CC_PASS2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_TINT_DEPTH_NOTEX, G_CC_PASS2),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_TINT_REMAP_NOTEX, G_CC_PASS2),
    },

    [TEX_COMBINE_MAIN_ONLY_0] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(G_CC_MODULATEIA, G_CC_MODULATEIA),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_MODULATEIDECALA, G_CC_PASS2),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(G_CC_MODULATEIA, G_CC_PASS2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(G_CC_MODULATEIDECALA, PM_CC_20),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(G_CC_MODULATEIA, PM_CC_TINT_REMAP_NO_SHADE),
    },
    [TEX_COMBINE_MAIN_ONLY_1] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(G_CC_BLENDRGBA, G_CC_BLENDRGBA),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_BLENDRGBA, G_CC_PASS2),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(G_CC_BLENDRGBA, G_CC_PASS2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(G_CC_BLENDRGBDECALA, PM_CC_TINT_DEPTH_NO_SHADE),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(G_CC_BLENDRGBA, PM_CC_TINT_REMAP_NO_SHADE),
    },
    [TEX_COMBINE_MAIN_ONLY_2] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_DECALRGBA),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_PASS2),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(G_CC_DECALRGBA, G_CC_PASS2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_1A, G_CC_PASS2),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(G_CC_DECALRGBA, PM_CC_TINT_REMAP_NO_SHADE),
    },

    // blend LODs in first cycle, tint in second cycle
    // all three sub-types are identical
    [TEX_COMBINE_MIPMAPS_0] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(G_CC_TRILERP, G_CC_MODULATEIA2),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_TRILERP, PM_CC2_MULTIPLY_SHADE),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(G_CC_TRILERP, G_CC_MODULATEIA2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(G_CC_TRILERP, PM_CC_TINT_DEPTH_MIPMAPS),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(G_CC_TRILERP, PM_CC_TINT_REMAP_NO_SHADE),
    },
    [TEX_COMBINE_MIPMAPS_1] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(G_CC_TRILERP, G_CC_MODULATEIA2),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_TRILERP, PM_CC2_MULTIPLY_SHADE),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(G_CC_TRILERP, G_CC_MODULATEIA2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(G_CC_TRILERP, PM_CC_TINT_DEPTH_MIPMAPS),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(G_CC_TRILERP, PM_CC_TINT_REMAP_NO_SHADE),
    },
    [TEX_COMBINE_MIPMAPS_2] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(G_CC_TRILERP, G_CC_MODULATEIA2),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_TRILERP, PM_CC2_MULTIPLY_SHADE),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(G_CC_TRILERP, G_CC_MODULATEIA2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(G_CC_TRILERP, PM_CC_TINT_DEPTH_MIPMAPS),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(G_CC_TRILERP, PM_CC_TINT_REMAP_NO_SHADE),
    },

    // blend main/aux textures in first cycle, tint in second cycle
    // all three sub-types are identical
    [TEX_COMBINE_AUX_SHARED_0] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_INTERFERENCE, PM_CC2_MULTIPLY_SHADE),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, PM_CC_TINT_REMAP_SHADE_ALPHA),
    },
    [TEX_COMBINE_AUX_SHARED_1] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_INTERFERENCE, PM_CC2_MULTIPLY_SHADE),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, PM_CC_TINT_REMAP_SHADE_ALPHA),
    },
    [TEX_COMBINE_AUX_SHARED_2] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_INTERFERENCE, PM_CC2_MULTIPLY_SHADE),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, PM_CC_TINT_REMAP_SHADE_ALPHA),
    },

    // blend main/aux textures in first cycle, tint in second cycle
    // all three sub-types are identical
    [TEX_COMBINE_AUX_IND_0] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_INTERFERENCE, PM_CC2_MULTIPLY_SHADE),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, PM_CC_TINT_REMAP_SHADE_ALPHA),
    },
    [TEX_COMBINE_AUX_IND_1] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_INTERFERENCE, PM_CC2_MULTIPLY_SHADE),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, PM_CC_TINT_REMAP_SHADE_ALPHA),
    },
    [TEX_COMBINE_AUX_IND_2] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(G_CC_INTERFERENCE, PM_CC2_MULTIPLY_SHADE),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, G_CC_MODULATEIA2),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_ALT_INTERFERENCE, PM_CC_TINT_REMAP_SHADE_ALPHA),
    },

    // shaded color multiplied main/aux textures for alpha
    [TEX_COMBINE_3] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC_26, PM_CC_27),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(PM_CC_26, PM_CC_27),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC_26, PM_CC_27),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_26, PM_CC_27),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_26, PM_CC_28),
    },
    // lerp between main/aux textures with shade alpha
    [TEX_COMBINE_4] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC_22, G_CC_PASS2),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(PM_CC_22, G_CC_PASS2),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC_22, G_CC_PASS2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_22, G_CC_PASS2),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_22, PM_CC_TINT_REMAP_NO_SHADE),
    },
    [TEX_COMBINE_5] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC1_24, PM_CC2_24),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(PM_CC1_24, PM_CC2_24),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC1_24, PM_CC2_24),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC1_24, PM_CC2_24),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC1_24, PM_CC2_24),
    },
    [TEX_COMBINE_6] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC_23, PM_CC_23),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(PM_CC_23, G_CC_PASS2),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC_23, G_CC_PASS2),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_23, G_CC_PASS2),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_23, PM_CC_TINT_REMAP_NO_SHADE),
    },
    [TEX_COMBINE_7] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC1_29, PM_CC2_29),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(PM_CC1_29, PM_CC2_29),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC1_29, PM_CC2_29),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC1_29, PM_CC2_29),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC1_29, PM_CC2_29),
    },
    [TEX_COMBINE_8] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
    },
    [TEX_COMBINE_9] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
    },
    [TEX_COMBINE_A] {
        [TINT_COMBINE_NONE]     gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
        [TINT_COMBINE_FOG]      gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
        [TINT_COMBINE_SHROUD]   gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
        [TINT_COMBINE_DEPTH]    gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
        [TINT_COMBINE_REMAP]    gsDPSetCombineMode(PM_CC_NOISE, PM_CC_NOISE),
    },
};

void old_make_texture_gfx(TextureHeader* header, Gfx** gfxPos, IMG_PTR raster, PAL_PTR palette, IMG_PTR auxRaster, PAL_PTR auxPalette, u8 auxShiftS, u8 auxShiftT, u16 auxOffsetS, u16 auxOffsetT) {
    s32 mainWidth, mainHeight;
    s32 auxWidth, auxHeight;
    s32 mainFmt;
    s32 auxFmt;
    s32 mainWrapW, mainWrapH;
    s32 auxWrapW, auxWrapH;
    s32 extraTileType;
    u32 texCombineType;
    s32 lod;
    s32 lodDivisor;
    IMG_PTR rasterPtr;
    s32 filteringMode;
    s32 auxPaletteIndex;
    s32 lutMode;
    s32 lodMode;
    s32 mainMasks, mainMaskt;
    s32 auxMasks, auxMaskt;
    s32 mainBitDepth;
    s32 auxBitDepth;
    s32 temp;

    mainWidth = header->mainW;
    mainHeight = header->mainH;

    lod = 0;
    auxPaletteIndex = 0;

    mainMasks = INTEGER_LOG2(mainWidth);
    mainMaskt = INTEGER_LOG2(mainHeight);

    mainWrapW = header->mainWrapW;
    mainWrapH = header->mainWrapH;

    mainFmt = header->mainFmt;
    mainBitDepth = header->mainBitDepth;

    extraTileType = header->extraTiles;
    filteringMode = header->filtering << G_MDSFT_TEXTFILT;

    auxWidth = header->auxW;
    auxHeight = header->auxH;

    auxMasks = INTEGER_LOG2(auxWidth);
    auxMaskt = INTEGER_LOG2(auxHeight);

    auxWrapW = header->auxWrapW;
    auxWrapH = header->auxWrapH;
    auxFmt = header->auxFmt;
    auxBitDepth = header->auxBitDepth;

    if (extraTileType == EXTRA_TILE_AUX_INDEPENDENT) {
        if (palette != nullptr) {
            auxPaletteIndex = 1;
        } else {
            auxPaletteIndex = 0;
        }
    }

    if (palette != nullptr || auxPalette != nullptr) {
        lutMode = G_TT_RGBA16;
        if (palette != nullptr) {
            if (mainBitDepth == G_IM_SIZ_4b) {
                gDPLoadTLUT_pal16((*gfxPos)++, 0, palette);
            } else if (mainBitDepth == G_IM_SIZ_8b) {
                gDPLoadTLUT_pal256((*gfxPos)++, palette);
            }
        }
        if (auxPalette != nullptr) {
            if (auxBitDepth == G_IM_SIZ_4b) {
                gDPLoadTLUT_pal16((*gfxPos)++, auxPaletteIndex, auxPalette);
            } else if (auxBitDepth == G_IM_SIZ_8b) {
                gDPLoadTLUT_pal256((*gfxPos)++, auxPalette);
            }
        }
    } else {
        lutMode = G_TT_NONE;
    }

    // only the following aux combine modes are ever used:
    // (A) 0x00 -> 0, 0
    // (B) 0x08 -> 2, 0
    // (C) 0x0D -> 3, 1
    // (D) 0x10 -> 4, 0
    texCombineType = header->auxCombineType;
    if (texCombineType >= 3) {
        // combine modes 3, 4, ... are directly appended to the end of the table and subtype is ignored
        texCombineType = TEX_COMBINE_3 + (texCombineType - 3);
    } else {
        // select based on aux combine subtypes
        // in practice, auxCombineSubType is ALWAYS zero here since the only (A) and (B) may reach this block
        texCombineType = 1 + header->extraTiles * AUX_COMBINE_SUB_COUNT + header->auxCombineSubType;
    }

    *(*gfxPos) = old_SolidCombineModes[texCombineType][TINT_COMBINE_NONE];
    (*gfxPos)++;

    switch (extraTileType) {
        case EXTRA_TILE_NONE:
            lodMode = G_TL_TILE;
            gSPTexture((*gfxPos)++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
            switch (mainBitDepth) {
                case G_IM_SIZ_4b:
                    gDPLoadTextureBlock_4b((*gfxPos)++, raster, mainFmt,
                                           mainWidth, mainHeight, 0,
                                           mainWrapW, mainWrapH, mainMasks, mainMaskt, G_TX_NOLOD, G_TX_NOLOD);
                    break;
                case G_IM_SIZ_8b:
                    gDPLoadTextureBlock((*gfxPos)++, raster, mainFmt, G_IM_SIZ_8b,
                                        mainWidth, mainHeight, 0,
                                        mainWrapW, mainWrapH, mainMasks, mainMaskt, G_TX_NOLOD, G_TX_NOLOD);
                    break;
                case G_IM_SIZ_16b:
                    gDPLoadTextureBlock((*gfxPos)++, raster, mainFmt, G_IM_SIZ_16b,
                                        mainWidth, mainHeight, 0,
                                        mainWrapW, mainWrapH, mainMasks, mainMaskt, G_TX_NOLOD, G_TX_NOLOD);
                    break;
                case 3:
                    gDPLoadTextureBlock((*gfxPos)++, raster, mainFmt, G_IM_SIZ_32b,
                                        mainWidth, mainHeight, 0,
                                        mainWrapW, mainWrapH, mainMasks, mainMaskt, G_TX_NOLOD, G_TX_NOLOD);
                    break;
            }
            break;
        case EXTRA_TILE_MIPMAPS:
            lodMode = G_TL_LOD;
            switch (mainBitDepth) {
                case G_IM_SIZ_4b:
                    for (rasterPtr = raster, lod = 0, lodDivisor = 1;
                         mainWidth / lodDivisor * 4 >= 64 && mainHeight / lodDivisor != 0;
                         rasterPtr += mainWidth / lodDivisor * mainHeight / lodDivisor / 2, lodDivisor *= 2, lod++)
                    {
                        gDPLoadMultiTile_4b((*gfxPos)++, rasterPtr, (u32)(rasterPtr - raster) >> 3, lod, mainFmt,
                                            mainWidth / lodDivisor, mainHeight / lodDivisor,
                                            0, 0, mainWidth / lodDivisor - 1, mainHeight / lodDivisor - 1, 0,
                                            mainWrapW, mainWrapH, mainMasks - lod, mainMaskt - lod, lod, lod);
                    }
                    break;
                case G_IM_SIZ_8b:
                    for (rasterPtr = raster, lod = 0, lodDivisor = 1;
                         mainWidth / lodDivisor * 8 >= 64 && mainHeight / lodDivisor != 0;
                         rasterPtr += mainWidth / lodDivisor * mainHeight / lodDivisor, lodDivisor *= 2, lod++)
                    {
                        gDPLoadMultiTile((*gfxPos)++, rasterPtr, ((u32)(rasterPtr - raster)) >> 3, lod, mainFmt, G_IM_SIZ_8b,
                                         mainWidth / lodDivisor, mainHeight / lodDivisor,
                                         0, 0, mainWidth / lodDivisor - 1, mainHeight / lodDivisor - 1, 0,
                                         mainWrapW, mainWrapH, mainMasks - lod, mainMaskt - lod, lod, lod);
                    }
                    break;
                case G_IM_SIZ_16b:
                    for (rasterPtr = raster, lod = 0, lodDivisor = 1;
                         mainWidth / lodDivisor * 16 >= 64 && mainHeight / lodDivisor != 0;
                         rasterPtr += mainWidth / lodDivisor * mainHeight / lodDivisor * 2, lodDivisor *= 2, lod++)
                    {
                        gDPLoadMultiTile((*gfxPos)++, rasterPtr, ((u32)(rasterPtr - raster)) >> 3, lod, mainFmt, G_IM_SIZ_16b,
                                         mainWidth / lodDivisor, mainHeight / lodDivisor,
                                         0, 0, mainWidth / lodDivisor - 1, mainHeight / lodDivisor - 1, 0,
                                         mainWrapW, mainWrapH, mainMasks - lod, mainMaskt - lod, lod, lod);
                    }
                    break;
                case G_IM_SIZ_32b:
                    for (rasterPtr = raster, lod = 0, lodDivisor = 1;
                         mainWidth / lodDivisor * 32 >= 64 && mainHeight / lodDivisor != 0;
                         rasterPtr += mainWidth / lodDivisor * mainHeight / lodDivisor * 4, lodDivisor *= 2, lod++)
                    {
                        gDPLoadMultiTile((*gfxPos)++, rasterPtr, ((u32)(rasterPtr - raster)) >> 4, lod, mainFmt, G_IM_SIZ_32b,
                                         mainWidth / lodDivisor, mainHeight / lodDivisor,
                                         0, 0, mainWidth / lodDivisor - 1, mainHeight / lodDivisor - 1, 0,
                                         mainWrapW, mainWrapH, mainMasks - lod, mainMaskt - lod, lod, lod);
                    }
                    break;
            }
            gSPTexture((*gfxPos)++, 0xFFFF, 0xFFFF, lod - 1, G_TX_RENDERTILE, G_ON);
            break;
        case EXTRA_TILE_AUX_SAME_AS_MAIN:
            gSPTexture((*gfxPos)++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
            gDPPipeSync((*gfxPos)++);
            lodMode = G_TL_TILE;
            switch (mainBitDepth) {
                case G_IM_SIZ_4b:
                    gDPScrollTextureBlockHalfHeight_4b((*gfxPos)++, raster, mainFmt, mainWidth, mainHeight, 0,
                                                       mainWrapW, mainWrapH, mainMasks, mainMaskt, G_TX_NOLOD, G_TX_NOLOD,
                                                       auxOffsetS, auxOffsetT, auxShiftS, auxShiftT);
                    break;
                case G_IM_SIZ_8b:
                    gDPScrollTextureBlockHalfHeight((*gfxPos)++, raster, mainFmt, G_IM_SIZ_8b, mainWidth, mainHeight, 0,
                                                    mainWrapW, mainWrapH, mainMasks, mainMaskt, G_TX_NOLOD, G_TX_NOLOD,
                                                    auxOffsetS, auxOffsetT, auxShiftS, auxShiftT);
                    break;
                case G_IM_SIZ_16b:
                    gDPScrollTextureBlockHalfHeight((*gfxPos)++, raster, mainFmt, G_IM_SIZ_16b, mainWidth, mainHeight, 0,
                                                    mainWrapW, mainWrapH, mainMasks, mainMaskt, G_TX_NOLOD, G_TX_NOLOD,
                                                    auxOffsetS, auxOffsetT, auxShiftS, auxShiftT);
                    break;
                case G_IM_SIZ_32b:
                    gDPScrollTextureBlockHalfHeight((*gfxPos)++, raster, mainFmt, G_IM_SIZ_32b, mainWidth, mainHeight, 0,
                                                    mainWrapW, mainWrapH, mainMasks, mainMaskt, G_TX_NOLOD, G_TX_NOLOD,
                                                    auxOffsetS, auxOffsetT, auxShiftS, auxShiftT);
                    break;
            }
            break;
        case EXTRA_TILE_AUX_INDEPENDENT:
            gSPTexture((*gfxPos)++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
            lodMode = G_TL_TILE;
            switch (mainBitDepth) {
                case G_IM_SIZ_4b:
                    gDPLoadTextureTile_4b((*gfxPos)++, raster, mainFmt, mainWidth, mainHeight,
                                          0, 0, mainWidth - 1, mainHeight - 1, 0,
                                          mainWrapW, mainWrapH, mainMasks, mainMaskt, G_TX_NOLOD, G_TX_NOLOD);
                    lodDivisor = (((mainWidth * mainHeight) >> 1) + 7)>>3; // required to use lodDivisor here
                    break;
                case G_IM_SIZ_8b:
                    gDPLoadTextureTile((*gfxPos)++, raster, mainFmt, G_IM_SIZ_8b, mainWidth, mainHeight,
                                       0, 0, mainWidth - 1, mainHeight - 1, 0,
                                       mainWrapW, mainWrapH, mainMasks, mainMaskt, G_TX_NOLOD, G_TX_NOLOD);
                    lodDivisor = ((mainWidth * mainHeight) + 7)>>3;
                    break;
                case G_IM_SIZ_16b:
                    gDPLoadTextureTile((*gfxPos)++, raster, mainFmt, G_IM_SIZ_16b, mainWidth, mainHeight,
                                       0, 0, mainWidth - 1, mainHeight - 1, 0,
                                       mainWrapW, mainWrapH, mainMasks, mainMaskt, G_TX_NOLOD, G_TX_NOLOD);
                    lodDivisor = ((mainWidth * mainHeight) * 2 + 7)>>3;
                    break;
                case G_IM_SIZ_32b:
                    gDPLoadTextureTile((*gfxPos)++, raster, mainFmt, G_IM_SIZ_32b, mainWidth, mainHeight,
                                       0, 0, mainWidth - 1, mainHeight - 1, 0,
                                       mainWrapW, mainWrapH, mainMasks, mainMaskt, G_TX_NOLOD, G_TX_NOLOD);
                    lodDivisor = ((mainWidth * mainHeight / 2) * 2 + 7)>>3;
                    break;
            }

            switch (auxBitDepth) {
                case G_IM_SIZ_4b:
                    gDPScrollMultiTile_4b((*gfxPos)++, auxRaster, lodDivisor, 1, auxFmt, auxWidth, auxHeight,
                                          0, 0, auxWidth - 1, auxHeight - 1, auxPaletteIndex,
                                          auxWrapW, auxWrapH, auxMasks, auxMaskt,
                                          auxShiftS, auxShiftT, auxOffsetS, auxOffsetT);
                    break;
                case G_IM_SIZ_8b:
                    gDPScrollMultiTile((*gfxPos)++, auxRaster, lodDivisor, 1, auxFmt, G_IM_SIZ_8b, auxWidth, auxHeight,
                                       0, 0, auxWidth - 1, auxHeight - 1, auxPaletteIndex,
                                       auxWrapW, auxWrapH, auxMasks, auxMaskt,
                                       auxShiftS, auxShiftT, auxOffsetS, auxOffsetT);
                    break;
                case G_IM_SIZ_16b:
                    gDPScrollMultiTile((*gfxPos)++, auxRaster, lodDivisor, 1, auxFmt, G_IM_SIZ_16b, auxWidth, auxHeight,
                                       0, 0, auxWidth - 1, auxHeight - 1, auxPaletteIndex,
                                       auxWrapW, auxWrapH, auxMasks, auxMaskt,
                                       auxShiftS, auxShiftT, auxOffsetS, auxOffsetT);
                    break;
                case G_IM_SIZ_32b:
                    gDPScrollMultiTile((*gfxPos)++, auxRaster, lodDivisor, 1, auxFmt, G_IM_SIZ_32b, auxWidth, auxHeight,
                                       0, 0, auxWidth - 1, auxHeight - 1, auxPaletteIndex,
                                       auxWrapW, auxWrapH, auxMasks, auxMaskt,
                                       auxShiftS, auxShiftT, auxOffsetS, auxOffsetT);
                    break;
            }
    }
    gSPSetOtherMode((*gfxPos)++, G_SETOTHERMODE_H, 4, 16, filteringMode | G_TC_FILT | lutMode | lodMode | G_TP_PERSP );
}
