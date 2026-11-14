#include "common.h"
#include <string.h>
#include "nd_api.h"
#include "nd_st.h"
#include "nd_val.h"
#define DIMG_CW_(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1)     { ND_W0_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1), ND_W1_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) }
#define DIMG_CW(a, b) DIMG_CW_(a, b)
static const NdTRect* dimg_tpl; /* S5TIMG: the state of draw_ci_image_with_clipping for the strips of its draw (NULL: old ops) */

s32 draw_image_with_clipping(IMG_PTR raster, u32 width, u32 height, s32 fmt, s32 bitDepth, s16 posX, s16 posY,
                             u16 clipX, u16 clipY, u16 clipWidth, u16 clipHeight);

s32 draw_ci_image_with_clipping(IMG_PTR raster, s32 width, s32 height, s32 fmt, s32 bitDepth, PAL_PTR palette, s16 posX,
                                s16 posY, u16 clipULx, u16 clipULy, u16 clipLRx, u16 clipRLy, u8 opacity) {
    s32 ret = 1;
    NdTRect t;

    nd_pso_cyc(G_CYC_1CYCLE);
    nd_pso_filt(G_TF_POINT);
    nd_pso_persp(G_TP_NONE);

    if (opacity == 255) {
        nd_pso_rm(G_RM_TEX_EDGE, G_RM_TEX_EDGE2);
        nd_pso_cc(G_CC_DECALRGBA, G_CC_DECALRGBA);
    } else if (opacity == 0) {
        return ret;
    } else {
        nd_pso_rm(G_RM_XLU_SURF, G_RM_XLU_SURF2);
        nd_pso_cc(PM_CC_02, PM_CC_02);
        nd_val_prim(0, 0, 0, 0, 0, opacity);
    }

    if (fmt == G_IM_FMT_CI) {
        nd_pso_lut(G_TT_RGBA16);
        if (bitDepth == G_IM_SIZ_4b) {
            gDPLoadTLUT_pal16(gMainGfxPos++, 0, palette);
        } else {
            gDPLoadTLUT_pal256(gMainGfxPos++, palette);
        }
    } else {
        nd_pso_lut(G_TT_NONE);
    }
    {
        static const u32 cw_decal[2] = DIMG_CW(G_CC_DECALRGBA, G_CC_DECALRGBA), cw_02[2] = DIMG_CW(PM_CC_02, PM_CC_02);
        memset(&t, 0, sizeof t);
        if (opacity == 255) {
            t.cw0 = cw_decal[0]; t.cw1 = cw_decal[1]; t.oml = G_RM_TEX_EDGE | G_RM_TEX_EDGE2; t.pad = 1;
        } else {
            t.cw0 = cw_02[0]; t.cw1 = cw_02[1]; t.oml = G_RM_XLU_SURF | G_RM_XLU_SURF2; t.prim = opacity;
        }
        t.omh = (fmt == G_IM_FMT_CI ? G_TT_RGBA16 : G_TT_NONE) | G_TF_POINT;
        if (fmt == G_IM_FMT_CI) {
            t.pal = palette;
            t.pal_n = bitDepth == G_IM_SIZ_4b ? 16u : 256u;
        }
        dimg_tpl = &t;
    }
    ret = draw_image_with_clipping(raster, width, height, fmt, bitDepth, posX, posY, clipULx, clipULy, clipLRx, clipRLy);
    dimg_tpl = NULL;
    return ret;
}

typedef struct Rect2b {
    /* 0x00 */ s16 ulx;
    /* 0x02 */ s16 uly;
    /* 0x04 */ s16 lrx;
    /* 0x06 */ s16 lry;
} Rect2b; // size = 0x08

s32 draw_image_with_clipping(IMG_PTR raster, u32 width, u32 height, s32 fmt, s32 bitDepth,
                     s16 posX, s16 posY,
                     u16 clipX, u16 clipY,
                     u16 clipWidth, u16 clipHeight) {
    Rect2b texRect;
    Rect2b drawRect;
    u8 stopDrawing;
    u8 stopDrawingLine;
    s8 zero = 0; // required to match

    u16 texOffsetX, texOffsetY;

    if (posX >= clipX + clipWidth || posY >= clipY + clipHeight) {
        return false;
    }
    if (clipX >= (s16)(posX + width)) {
        return false;
    }
    if (clipY >= (s16)(posY + height)) {
        return false;
    }

    texRect.uly = 0;
    drawRect.uly = posY;
    stopDrawing = false;

    while (true) {
        texRect.lry = texRect.uly + 31;
        drawRect.lry = drawRect.uly + 32;
        texOffsetY = 0;
        if (drawRect.lry <= clipY) {
            do {
                texRect.uly += 32;
                drawRect.uly = drawRect.lry;
                drawRect.lry += 32;
            } while (drawRect.lry < clipY);
            texRect.lry = texRect.uly + 31;
        }

        if (drawRect.uly < clipY) {
            texOffsetY = abs(posY - clipY);
            drawRect.uly = clipY;
        }

        if (drawRect.lry >= clipY + clipHeight) {
            texRect.lry = clipY + clipHeight - posY - 1;
            stopDrawing = true;
            drawRect.lry = clipY + clipHeight;
        }

        if (texRect.lry + 1 >= height) {
            texRect.lry = height - 1;
            drawRect.lry = texRect.lry + posY + 1;
            stopDrawing = true;
        }

        texRect.ulx = zero;
        drawRect.ulx = posX;
        stopDrawingLine = false;

        while (true) {
            texRect.lrx = texRect.ulx + 63;
            drawRect.lrx = drawRect.ulx + 64;
            texOffsetX = 0;

            if (drawRect.lrx <= clipX) {
                do {
                    texRect.ulx += 64;
                    drawRect.ulx = drawRect.lrx;
                    drawRect.lrx += 64;
                } while (drawRect.lrx < clipX);
                texRect.lrx = texRect.ulx + 63;
            }

            if (drawRect.ulx < clipX) {
                texOffsetX = abs(posX - clipX);
                drawRect.ulx = clipX;
            }

            if (drawRect.lrx >= clipX + clipWidth) {
                texRect.lrx = clipX + clipWidth - posX - 1;
                stopDrawingLine = true;
                drawRect.lrx = clipX + clipWidth;
            }

            if (texRect.lrx + 1 >= width) {
                texRect.lrx = width - 1;
                drawRect.lrx = texRect.lrx + posX + 1;
                stopDrawingLine = true;
            }

            if (dimg_tpl && (bitDepth == G_IM_SIZ_4b || bitDepth == G_IM_SIZ_8b || bitDepth == G_IM_SIZ_16b)) {
                NdTRect r = *dimg_tpl;
                r.img = raster; r.fmt = fmt; r.siz = bitDepth; r.width = width; r.height = height;
                r.uls = texRect.ulx; r.ult = texRect.uly; r.lrs = texRect.lrx; r.lrt = texRect.lry;
                r.cms = G_TX_WRAP; r.cmt = G_TX_WRAP; r.masks = 6; r.maskt = 5;
                r.w0 = ND_W0_TR(G_TEXRECT, drawRect.lrx * 4, drawRect.lry * 4); r.w1 = ND_W1_TR(0, drawRect.ulx * 4, drawRect.uly * 4);
                r.hs = ND_TR_H1(texOffsetX * 32, texOffsetY * 32); r.hd = ND_TR_H2(1024, 1024);
                nd_rec_napi(nd_trect_run, &r, sizeof r / 8u);
            } else
            {
                if (bitDepth == G_IM_SIZ_4b) {
                    gDPLoadTextureTile_4b(gMainGfxPos++, raster, fmt, width, height,
                                    texRect.ulx, texRect.uly, texRect.lrx, texRect.lry, 0,
                                    G_TX_WRAP, G_TX_WRAP, 6, 5, G_TX_NOLOD, G_TX_NOLOD);
                } else if (bitDepth == G_IM_SIZ_16b) {
                    gDPLoadTextureTile(gMainGfxPos++, raster, fmt, G_IM_SIZ_16b, width, height,
                                    texRect.ulx, texRect.uly, texRect.lrx, texRect.lry, 0,
                                    G_TX_WRAP, G_TX_WRAP, 6, 5, G_TX_NOLOD, G_TX_NOLOD);
                } else if (bitDepth == G_IM_SIZ_8b) {
                    gDPLoadTextureTile(gMainGfxPos++, raster, fmt, G_IM_SIZ_8b, width, height,
                                    texRect.ulx, texRect.uly, texRect.lrx, texRect.lry, 0,
                                    G_TX_WRAP, G_TX_WRAP, 6, 5, G_TX_NOLOD, G_TX_NOLOD);
                }

                gSPTextureRectangle(gMainGfxPos++, drawRect.ulx * 4, drawRect.uly * 4, drawRect.lrx * 4, drawRect.lry * 4,
                                    0, texOffsetX * 32, texOffsetY * 32, 1024, 1024);
            }

            if (stopDrawingLine) {
                break;
            }
            texRect.ulx += 64;
            drawRect.ulx = drawRect.lrx;
        }

        if (stopDrawing) {
            break;
        }
        texRect.uly += 32;
        drawRect.uly = drawRect.lry;
    }

    return true;
}

s32 integer_log(s32 number, u32 base) {
    f32 fNumber = number;
    s32 ret = 1;

    while (true) {
        fNumber /= base;
        if (fNumber <= 1.0) {
            return ret;
        }

        ret++;
    }
}
