/* nd_tx.h: the texture, rectangle, viewport and colour calls of a call site as setters (ws_TEX C1, C2). Header only (no nd_tx.c).
 * Each setter is the gbi macro with the packet argument removed. It records the same typed unit(s) that the nd_gbi.h redirect records
 * for that macro: nd_tx_rec is one state op (nd_rec_state), nd_tx_rect is one texture rectangle (nd_rec_texrect, the nd_gx_w path of
 * nd_rec.c). The words are the macro's (the ND_W0_* / ND_W1_* helpers of nd_gbi.h). A load is the gbi body of the load macro with its
 * syncs removed (a sync is the no-op unit, which nd_gx_w drops). Each setter is a do { } while (0) statement or an expression.
 * Include gbi.h (with the game's headers) before this file. */
#ifndef ND_TX_H
#define ND_TX_H
#include <stdint.h>
#include "nd_cmd.h"
#include "nd_gbi.h"

/* one state op: the unit words (ND_X0, ND_X1 of the gbi words), recorded as nd_gx_w records them */
static inline void nd_tx_rec(uint32_t w0, uint32_t w1) { nd_rec_state(ND_X0(w0, w1), ND_X1(w0, w1)); }

/* one texture rectangle: the rectangle unit (w0, w1 of gSPTextureRectangle) and the two RDPHALF words (h1: s, t; h2: dsdx, dtdy) */
static inline void nd_tx_rect(uint32_t w0, uint32_t w1, uint32_t h1, uint32_t h2) {
    const uint32_t a0 = ND_X0(w0, w1), a1 = ND_X1(w0, w1);
    nd_rec_texrect(((uint32_t)G_TEXRECT << 24) | (a0 & 0xffffffu), a1 & ~NDO_RECT_TEX, h1, h2);
}

/* word setters: the image, tile, load and load-tlut commands (gbi.h gDPSetTextureImage, gDPSetTile, gDPLoadTile, gDPLoadBlock, gDPLoadTLUTCmd) */
#define nd_tex_img(f, s, w, i) nd_tx_rec(ND_W0_IMG(G_SETTIMG, f, s, w), (unsigned int)(i))
#define nd_tex_tile(fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts) \
    nd_tx_rec(ND_W0_TILE(fmt, siz, line, tmem), ND_W1_TILE(tile, palette, cmt, maskt, shiftt, cms, masks, shifts))
#define nd_tex_ldtile(t, uls, ult, lrs, lrt) nd_tx_rec(ND_W0_LDT(G_LOADTILE, uls, ult), ND_W1_LDT(t, lrs, lrt))
#define nd_tex_tsz(t, uls, ult, lrs, lrt) nd_tx_rec(ND_W0_LDT(G_SETTILESIZE, uls, ult), ND_W1_LDT(t, lrs, lrt))
#define nd_tex_hilite1(t, hl, wd, ht) nd_tex_tsz(t, ((hl)->h.x1 & 0xfff), ((hl)->h.y1 & 0xfff), (((((wd)-1)*4)+(hl)->h.x1) & 0xfff), (((((ht)-1)*4)+(hl)->h.y1) & 0xfff)) /* gDPSetHilite1Tile */
#define nd_tex_hilite2(t, hl, wd, ht) nd_tex_tsz(t, ((hl)->h.x2 & 0xfff), ((hl)->h.y2 & 0xfff), (((((wd)-1)*4)+(hl)->h.x2) & 0xfff), (((((ht)-1)*4)+(hl)->h.y2) & 0xfff)) /* gDPSetHilite2Tile */
#define nd_tex_ldblk(tile, uls, ult, lrs, dxt) nd_tx_rec(ND_W0_LDB(uls, ult), ND_W1_LDB(tile, lrs, dxt))
#define nd_tex_ldtlut(tile, count) nd_tx_rec(_SHIFTL(G_LOADTLUT, 24, 8), _SHIFTL((tile), 24, 3) | _SHIFTL((count), 14, 10))

/* colour and value setters: gDPSetFillColor, gDPSetFogColor, gDPSetBlendColor (the DPRGBColor word), gSPViewport (gDma2p) */
#define nd_val_fill(d) nd_tx_rec(_SHIFTL(G_SETFILLCOLOR, 24, 8), (unsigned int)(d))
#define nd_val_fog(r, g, b, a) nd_tx_rec(_SHIFTL(G_SETFOGCOLOR, 24, 8), \
    (unsigned int)(_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8)))
#define nd_val_blend(r, g, b, a) nd_tx_rec(_SHIFTL(G_SETBLENDCOLOR, 24, 8), \
    (unsigned int)(_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8)))
#define nd_xf_viewport(v) nd_tx_rec(ND_W0_D2(G_MOVEMEM, sizeof(Vp), G_MV_VIEWPORT, 0), (unsigned int)(v))

/* pipeline, fog, geometry and key setters: gSPFogPosition (gMoveWd), gDPPipelineMode (gSPSetOtherMode), gSPLoadGeometryMode (c = -1), gDPSetKeyR, gDPSetKeyGB */
#define nd_pso_fogpos(min, max) nd_tx_rec(ND_W0_MW(G_MW_FOG, G_MWO_FOG), \
    (unsigned int)(_SHIFTL((128000/((max)-(min))), 16, 16) | _SHIFTL(((500-(min))*256/((max)-(min))), 0, 16)))
#define nd_pso_pipe(m) nd_tx_rec(ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_PIPELINE, 1), (unsigned int)(m))
#define nd_pso_geom_load(w) nd_tx_rec(ND_W0_GEOM(-1), (u32)(w))
#define nd_pso_keyr(cR, sR, wR) nd_tx_rec(_SHIFTL(G_SETKEYR, 24, 8), ND_W1_KEYR(cR, sR, wR))
#define nd_pso_keygb(cG, sG, wG, cB, sB, wB) nd_tx_rec(ND_W0_KEYGB(wG, wB), ND_W1_KEYGB(cG, sG, cB, sB))

/* scissor and rectangle setters: gDPSetScissorFrac, gDPFillRectangle, gDPScisFillRectangle, gSPTextureRectangle, gSPScisTextureRectangle */
#define nd_pso_scisf(mode, ulx, uly, lrx, lry) nd_tx_rec(ND_W0_SCISF(ulx, uly), ND_W1_SCISF(mode, lrx, lry))
#define nd_rect_fill(ulx, uly, lrx, lry) nd_tx_rec(ND_W0_FILL(lrx, lry), ND_W1_FILL(ulx, uly))
#define nd_rect_sfill(ulx, uly, lrx, lry) nd_tx_rec( \
    _SHIFTL(G_FILLRECT, 24, 8) | _SHIFTL(MAX((lrx), 0), 14, 10) | _SHIFTL(MAX((lry), 0), 2, 10), \
    _SHIFTL(MAX((ulx), 0), 14, 10) | _SHIFTL(MAX((uly), 0), 2, 10))
#define nd_rect_tex(xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    nd_tx_rect(ND_W0_TR(G_TEXRECT, xh, yh), ND_W1_TR(tile, xl, yl), ND_TR_H1(s, t), ND_TR_H2(dsdx, dtdy))
#define nd_rect_scis(xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    nd_tx_rect(_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(PM_SCIS_XH(xh), 12, 12) | _SHIFTL(MAX((s16)(yh), 0), 0, 12), \
               _SHIFTL((tile), 24, 3) | _SHIFTL(PM_SCIS_XL(xl), 12, 12) | _SHIFTL(MAX((s16)(yl), 0), 0, 12), \
               (_SHIFTL(((s) - (PM_SCIS_CLIP(xl) ? (((s16)(dsdx) < 0) ? (MAX((((s16)(xl) * (s16)(dsdx)) >> 7), 0)) \
                                                                    : (MIN((((s16)(xl) * (s16)(dsdx)) >> 7), 0))) : 0)), 16, 16) | \
                _SHIFTL(((t) - (((yl) < 0) ? (((s16)(dtdy) < 0) ? (MAX((((s16)(yl) * (s16)(dtdy)) >> 7), 0)) \
                                                              : (MIN((((s16)(yl) * (s16)(dtdy)) >> 7), 0))) : 0)), 0, 16)), \
               (_SHIFTL((dsdx), 16, 16) | _SHIFTL((dtdy), 0, 16)))

/* texture load macros (gbi.h gDPLoadTextureTile and its _4b, gDPLoadMultiTile and its _4b, gDPLoadTextureBlock and its _4b,
 * gDPLoadTLUT_pal16 and _pal256; gbi_custom.h gDPScroll*). The packet argument is removed; each sync is dropped. */
#define nd_tex_load_tile(timg, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    nd_tex_img(fmt, siz, width, timg); \
    nd_tex_tile(fmt, siz, (((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_ldtile(G_TX_LOADTILE, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
    nd_tex_tile(fmt, siz, (((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_tsz(G_TX_RENDERTILE, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
} while (0)

#define nd_tex_load_tile_4b(timg, fmt, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    nd_tex_img(fmt, G_IM_SIZ_8b, ((width)>>1), timg); \
    nd_tex_tile(fmt, G_IM_SIZ_8b, (((((lrs)-(uls)+1)>>1)+7)>>3), 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_ldtile(G_TX_LOADTILE, (uls)<<(G_TEXTURE_IMAGE_FRAC-1), (ult)<<(G_TEXTURE_IMAGE_FRAC), (lrs)<<(G_TEXTURE_IMAGE_FRAC-1), (lrt)<<(G_TEXTURE_IMAGE_FRAC)); \
    nd_tex_tile(fmt, G_IM_SIZ_4b, (((((lrs)-(uls)+1)>>1)+7)>>3), 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_tsz(G_TX_RENDERTILE, (uls)<<(G_TEXTURE_IMAGE_FRAC), (ult)<<(G_TEXTURE_IMAGE_FRAC), (lrs)<<(G_TEXTURE_IMAGE_FRAC), (lrt)<<(G_TEXTURE_IMAGE_FRAC)); \
} while (0)

#define nd_tex_load_mtile(timg, tmem, rtile, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    nd_tex_img(fmt, siz, width, timg); \
    nd_tex_tile(fmt, siz, (((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), tmem, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_ldtile(G_TX_LOADTILE, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
    nd_tex_tile(fmt, siz, (((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), tmem, rtile, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_tsz(rtile, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
} while (0)

#define nd_tex_load_mtile_4b(timg, tmem, rtile, fmt, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    nd_tex_img(fmt, G_IM_SIZ_8b, ((width)>>1), timg); \
    nd_tex_tile(fmt, G_IM_SIZ_8b, (((((lrs)-(uls)+1)>>1)+7)>>3), tmem, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_ldtile(G_TX_LOADTILE, (uls)<<(G_TEXTURE_IMAGE_FRAC-1), (ult)<<(G_TEXTURE_IMAGE_FRAC), (lrs)<<(G_TEXTURE_IMAGE_FRAC-1), (lrt)<<(G_TEXTURE_IMAGE_FRAC)); \
    nd_tex_tile(fmt, G_IM_SIZ_4b, (((((lrs)-(uls)+1)>>1)+7)>>3), tmem, rtile, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_tsz(rtile, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
} while (0)

#define nd_tex_load_block(timg, fmt, siz, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    nd_tex_img(fmt, siz##_LOAD_BLOCK, 1, timg); \
    nd_tex_tile(fmt, siz##_LOAD_BLOCK, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_ldblk(G_TX_LOADTILE, 0, 0, (((width)*(height) + siz##_INCR) >> siz##_SHIFT) -1, CALC_DXT(width, siz##_BYTES)); \
    nd_tex_tile(fmt, siz, (((width) * siz##_LINE_BYTES)+7)>>3, 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_tsz(G_TX_RENDERTILE, 0, 0, ((width)-1) << G_TEXTURE_IMAGE_FRAC, ((height)-1) << G_TEXTURE_IMAGE_FRAC); \
} while (0)

#define nd_tex_load_block_4b(timg, fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    nd_tex_img(fmt, G_IM_SIZ_16b, 1, timg); \
    nd_tex_tile(fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_ldblk(G_TX_LOADTILE, 0, 0, (((width)*(height)+3)>>2)-1, CALC_DXT_4b(width)); \
    nd_tex_tile(fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_tsz(G_TX_RENDERTILE, 0, 0, ((width)-1) << G_TEXTURE_IMAGE_FRAC, ((height)-1) << G_TEXTURE_IMAGE_FRAC); \
} while (0)

#define nd_tex_load_tlut16(pal, dram) do { \
    nd_tex_img(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, dram); \
    nd_tex_tile(0, 0, 0, (256+(((pal)&0xf)*16)), G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0); \
    nd_tex_ldtlut(G_TX_LOADTILE, 15); \
} while (0)

#define nd_tex_load_tlut256(dram) do { \
    nd_tex_img(G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, dram); \
    nd_tex_tile(0, 0, 0, 256, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0); \
    nd_tex_ldtlut(G_TX_LOADTILE, 255); \
} while (0)

#define nd_tex_scroll_mtile2_4b(timg, fmt, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt) do { \
    nd_tex_img(fmt, G_IM_SIZ_8b, ((width)>>1), timg); \
    nd_tex_tile(fmt, G_IM_SIZ_8b, (((((lrs)-(uls)+1)>>1)+7)>>3), 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_ldtile(G_TX_LOADTILE, (uls)<<(G_TEXTURE_IMAGE_FRAC-1), (ult)<<(G_TEXTURE_IMAGE_FRAC), (lrs)<<(G_TEXTURE_IMAGE_FRAC-1), (lrt)<<(G_TEXTURE_IMAGE_FRAC)); \
    nd_tex_tile(fmt, G_IM_SIZ_4b, (((((lrs)-(uls)+1)>>1)+7)>>3), 0, G_TX_RENDERTILE + 1, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_tsz(G_TX_RENDERTILE + 1, (uls + scrolls)<<G_TEXTURE_IMAGE_FRAC, (ult + scrollt)<<G_TEXTURE_IMAGE_FRAC, \
               (lrs + scrolls)<<G_TEXTURE_IMAGE_FRAC, (lrt + scrollt)<<G_TEXTURE_IMAGE_FRAC); \
    nd_tex_tile(fmt, G_IM_SIZ_4b, (((((lrs)-(uls)+1)>>1)+7)>>3), 0, G_TX_RENDERTILE + 0, 1, cmt, maskt, shiftt, cms, masks, shifts); \
} while (0)

#define nd_tex_scroll_tile_4b(timg, fmt, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt) do { \
    nd_tex_img(fmt, G_IM_SIZ_8b, ((width)>>1), timg); \
    nd_tex_tile(fmt, G_IM_SIZ_8b, (((((lrs)-(uls)+1)>>1)+7)>>3), 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_ldtile(G_TX_LOADTILE, (uls)<<(G_TEXTURE_IMAGE_FRAC-1), (ult)<<(G_TEXTURE_IMAGE_FRAC), (lrs)<<(G_TEXTURE_IMAGE_FRAC-1), (lrt)<<(G_TEXTURE_IMAGE_FRAC)); \
    nd_tex_tile(fmt, G_IM_SIZ_4b, (((((lrs)-(uls)+1)>>1)+7)>>3), 0, G_TX_RENDERTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_tsz(G_TX_RENDERTILE, (uls+scrolls)<<G_TEXTURE_IMAGE_FRAC, (ult+scrollt)<<G_TEXTURE_IMAGE_FRAC, \
               (lrs+scrolls)<<G_TEXTURE_IMAGE_FRAC, (lrt+scrollt)<<G_TEXTURE_IMAGE_FRAC); \
} while (0)

#define nd_tex_scroll_block_4b(timg, fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt) do { \
    nd_tex_img(fmt, G_IM_SIZ_16b, 1, timg); \
    nd_tex_tile(fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_ldblk(G_TX_LOADTILE, 0, 0, (((width)*(height)+3)>>2)-1, CALC_DXT_4b(width)); \
    nd_tex_tile(fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_tsz(G_TX_RENDERTILE, (scrolls) << G_TEXTURE_IMAGE_FRAC, (scrollt) << G_TEXTURE_IMAGE_FRAC, \
               ((width)-1 + scrolls) << G_TEXTURE_IMAGE_FRAC, ((height)-1 + scrollt) << G_TEXTURE_IMAGE_FRAC); \
} while (0)
#endif
