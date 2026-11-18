/* nd_tx.h: the texture, rectangle, viewport and colour calls of a call site as setters (ws_TEX C1, C2). Header only (no nd_tx.c).
 * Each setter is the gbi macro with the packet argument removed. It records the native record(s) (nd_nr.h, nd_rec_texrect for a texture
 * rectangle) built from the setter's own fields: the same bits the nd_gbi.h redirect records for that macro (nd_rec_nr, the nd_gx_w path
 * of nd_rec.c). A load is the gbi body of the load macro with its syncs removed (a sync is the no-op unit, which nd_gx_w drops). Each setter
 * is a do { } while (0) statement or an expression. Include gbi.h (with the game's headers) before this file. */
#ifndef ND_TX_H
#define ND_TX_H
#include <stdint.h>
#include "nd_cmd.h"
#include "nd_gbi.h"
#include "nd_nr.h"
#include "nd_fs.h"

/* word setters: the image, tile, load and load-tlut commands (gbi.h gDPSetTextureImage, gDPSetTile, gDPLoadTile, gDPLoadBlock, gDPLoadTLUTCmd) */
#define NDTX_IMG(O, f, s, w, i) O(ND_FS_W0(ND_FS_TIMG, ND_NR_IMG(f, s, w)), (unsigned int)(i))
#define nd_tex_img(...) NDTX_IMG(nd_nr, __VA_ARGS__)
#define NDTX_TILE(O, fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts) \
    O(ND_FS_W0(ND_FS_TILE, ND_NR_TILE_A(fmt, siz, line, tmem)), ND_NR_TILE_B(tile, palette, cmt, maskt, shiftt, cms, masks, shifts))
#define nd_tex_tile(...) NDTX_TILE(nd_nr, __VA_ARGS__)
#define NDTX_LDTILE(O, t, uls, ult, lrs, lrt) O(ND_FS_W0(ND_FS_LOADTILE, ND_NR_XY12(uls, ult)), ND_NR_TXY(t, lrs, lrt))
#define nd_tex_ldtile(...) NDTX_LDTILE(nd_nr, __VA_ARGS__)
#define NDTX_TSZ(O, t, uls, ult, lrs, lrt) O(ND_FS_W0(ND_FS_TILESIZE, ND_NR_XY12(uls, ult)), ND_NR_TXY(t, lrs, lrt))
#define nd_tex_tsz(...) NDTX_TSZ(nd_nr, __VA_ARGS__)
#define nd_tex_hilite1(t, hl, wd, ht) nd_tex_tsz(t, ((hl)->h.x1 & 0xfff), ((hl)->h.y1 & 0xfff), (((((wd)-1)*4)+(hl)->h.x1) & 0xfff), (((((ht)-1)*4)+(hl)->h.y1) & 0xfff)) /* gDPSetHilite1Tile */
#define nd_tex_hilite2(t, hl, wd, ht) nd_tex_tsz(t, ((hl)->h.x2 & 0xfff), ((hl)->h.y2 & 0xfff), (((((wd)-1)*4)+(hl)->h.x2) & 0xfff), (((((ht)-1)*4)+(hl)->h.y2) & 0xfff)) /* gDPSetHilite2Tile */
#define NDTX_LDBLK(O, tile, uls, ult, lrs, dxt) O(ND_FS_W0(ND_FS_LOADBLOCK, ND_NR_XY12(uls, ult)), ND_NR_TXY(tile, MIN(lrs, G_TX_LDBLK_MAX_TXL), dxt))
#define nd_tex_ldblk(...) NDTX_LDBLK(nd_nr, __VA_ARGS__)
#define NDTX_LDTLUT(O, tile, count) O(ND_FS_W0(ND_FS_LOADTLUT, 0u), ND_NR_TLUT(tile, count))
#define nd_tex_ldtlut(...) NDTX_LDTLUT(nd_nr, __VA_ARGS__)

/* colour and value setters: gDPSetFillColor, gDPSetFogColor, gDPSetBlendColor (the DPRGBColor word), gSPViewport (gDma2p) */
#define nd_val_fill(d) nd_nr(ND_FS_W0(ND_FS_FILL, 0u), (unsigned int)(d))
#define nd_val_fog(r, g, b, a) nd_nr(ND_FS_W0(ND_FS_FOG, 0u), ND_NR_RGBA(r, g, b, a))
#define nd_val_blend(r, g, b, a) nd_nr(ND_FS_W0(ND_FS_BLEND, 0u), ND_NR_RGBA(r, g, b, a))
#define NDTX_VIEWPORT(O, v) O(ND_NR_MOVEMEM(sizeof(Vp), G_MV_VIEWPORT, 0), (unsigned int)(v))
#define nd_xf_viewport(v) NDTX_VIEWPORT(nd_nr, v)

/* pipeline, fog, geometry and key setters: gSPFogPosition (gMoveWd), gDPPipelineMode (gSPSetOtherMode), gSPLoadGeometryMode (c = -1), gDPSetKeyR, gDPSetKeyGB */
#define nd_pso_fogpos(min, max) nd_nr(ND_NR_MW(ND_FS_MW_FOG, 0u), \
    ND_NR_HL((128000/((max)-(min))), ((500-(min))*256/((max)-(min)))))
#define nd_pso_pipe(m) nd_nr(ND_NR_OM(1, G_MDSFT_PIPELINE, 1), (unsigned int)(m))
#define nd_pso_geom_load(w) nd_nr(ND_NR_GEOM(0xffffffffu), (u32)(w))
#define nd_pso_keyr(cR, sR, wR) nd_nr(ND_FS_W0(ND_FS_RARE, ND_FS_RARE_KEYR), ND_NR_KEYR(cR, sR, wR))
#define nd_pso_keygb(cG, sG, wG, cB, sB, wB) nd_nr(ND_FS_W0(ND_FS_RARE, ND_FS_RARE_KEYGB), ND_NR_RGBA(cG, sG, cB, sB)), \
    (void)(wG), (void)(wB)

/* scissor and rectangle setters: gDPSetScissorFrac, gDPFillRectangle, gDPScisFillRectangle, gSPTextureRectangle, gSPScisTextureRectangle */
#define nd_pso_scisf(mode, ulx, uly, lrx, lry) nd_nr(ND_FS_W0(ND_FS_SCISSOR, ND_NR_XY12((int)(ulx), (int)(uly))), \
    ND_NR_XY12((int)(lrx), (int)(lry))), (void)(mode)
#define NDTX_FILL(O, ulx, uly, lrx, lry) O(ND_FS_W0(ND_FS_RECT, ND_NR_XY10(lrx, lry)), ND_NR_XY10(ulx, uly))
#define nd_rect_fill(ulx, uly, lrx, lry) NDTX_FILL(nd_nr, ulx, uly, lrx, lry)
#define nd_rect_sfill(ulx, uly, lrx, lry) nd_nr(ND_FS_W0(ND_FS_RECT, ND_NR_XY10(MAX((lrx), 0), MAX((lry), 0))), \
    ND_NR_XY10(MAX((ulx), 0), MAX((uly), 0)))
#define nd_rect_tex(xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    nd_rec_texrect(ND_NR_TR_W0(xh, yh), ND_NR_TXY(tile, xl, yl), ND_TR_H1(s, t), ND_TR_H2(dsdx, dtdy))
#define nd_rect_scis(xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    nd_rec_texrect(ND_NR_TR_W0(PM_SCIS_XH(xh), MAX((s16)(yh), 0)), \
               ND_NR_TXY((tile), PM_SCIS_XL(xl), MAX((s16)(yl), 0)), \
               (_SHIFTL(((s) - (PM_SCIS_CLIP(xl) ? (((s16)(dsdx) < 0) ? (MAX((((s16)(xl) * (s16)(dsdx)) >> 7), 0)) \
                                                                    : (MIN((((s16)(xl) * (s16)(dsdx)) >> 7), 0))) : 0)), 16, 16) | \
                _SHIFTL(((t) - (((yl) < 0) ? (((s16)(dtdy) < 0) ? (MAX((((s16)(yl) * (s16)(dtdy)) >> 7), 0)) \
                                                              : (MIN((((s16)(yl) * (s16)(dtdy)) >> 7), 0))) : 0)), 0, 16)), \
               (_SHIFTL((dsdx), 16, 16) | _SHIFTL((dtdy), 0, 16)))

/* the texture rectangle as three units (the walker reads the rectangle and its two RDPHALF units at op_ctl, nd_gx_w merges them): gSPTextureRectangle,
 * gSPScisTextureRectangle (the scissor form: the 4 values of nd_rect_scis), and the window rectangle (screen_overlays.c, w0 and w1 as the gbi words). */
#define NDTX_RECT_TEX(O, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) do {     O(ND_FS_W0(ND_FS_RECT, ND_NR_XY12(xh, yh)), ND_FS_RECT_TEX | ND_NR_TXY(tile, xl, yl));     O(ND_NOP_A, ND_TR_H1(s, t));     O(ND_NOP_A, ND_TR_H2(dsdx, dtdy)); } while (0)
#define NDTX_RECT_SCIS(O, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) do {     O(ND_FS_W0(ND_FS_RECT, ND_NR_XY12(PM_SCIS_XH(xh), MAX((s16)(yh), 0))), ND_FS_RECT_TEX | ND_NR_TXY((tile), PM_SCIS_XL(xl), MAX((s16)(yl), 0)));     O(ND_NOP_A, (_SHIFTL(((s) - (PM_SCIS_CLIP(xl) ? (((s16)(dsdx) < 0) ? (MAX((((s16)(xl) * (s16)(dsdx)) >> 7), 0))                                                                     : (MIN((((s16)(xl) * (s16)(dsdx)) >> 7), 0))) : 0)), 16, 16) |                 _SHIFTL(((t) - (((yl) < 0) ? (((s16)(dtdy) < 0) ? (MAX((((s16)(yl) * (s16)(dtdy)) >> 7), 0))                                                               : (MIN((((s16)(yl) * (s16)(dtdy)) >> 7), 0))) : 0)), 0, 16)));     O(ND_NOP_A, (_SHIFTL((dsdx), 16, 16) | _SHIFTL((dtdy), 0, 16))); } while (0)
#define NDTX_RECT_W(O, w0, w1, hs, hd) do {     O(ND_FS_W0(ND_FS_RECT, (w0)), ND_FS_RECT_TEX | ((uint32_t)(w1) & 0x07ffffffu));     O(ND_NOP_A, (hs));     O(ND_NOP_A, (hd)); } while (0)

/* texture load macros (gbi.h gDPLoadTextureTile and its _4b, gDPLoadMultiTile and its _4b, gDPLoadTextureBlock and its _4b,
 * gDPLoadTLUT_pal16 and _pal256; gbi_custom.h gDPScroll*). The packet argument is removed; each sync is dropped. */
#define NDTX_LOAD_TILE(O, timg, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    NDTX_IMG(O, fmt, siz, width, timg); \
    NDTX_TILE(O, fmt, siz, (((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_LDTILE(O, G_TX_LOADTILE, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
    NDTX_TILE(O, fmt, siz, (((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_TSZ(O, G_TX_RENDERTILE, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
} while (0)
/* the body of nd_tex_load_tile is the gbi body with the O forms expanded to nd_nr: a variadic or named wrapper of NDTX_LOAD_TILE would
 * expand the size argument before its siz##_BYTES paste. */
#define nd_tex_load_tile(timg, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    nd_tex_img(fmt, siz, width, timg); \
    nd_tex_tile(fmt, siz, (((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_ldtile(G_TX_LOADTILE, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
    nd_tex_tile(fmt, siz, (((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_tsz(G_TX_RENDERTILE, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
} while (0)

#define NDTX_LOAD_TILE_4B(O, timg, fmt, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    NDTX_IMG(O, fmt, G_IM_SIZ_8b, ((width)>>1), timg); \
    NDTX_TILE(O, fmt, G_IM_SIZ_8b, (((((lrs)-(uls)+1)>>1)+7)>>3), 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_LDTILE(O, G_TX_LOADTILE, (uls)<<(G_TEXTURE_IMAGE_FRAC-1), (ult)<<(G_TEXTURE_IMAGE_FRAC), (lrs)<<(G_TEXTURE_IMAGE_FRAC-1), (lrt)<<(G_TEXTURE_IMAGE_FRAC)); \
    NDTX_TILE(O, fmt, G_IM_SIZ_4b, (((((lrs)-(uls)+1)>>1)+7)>>3), 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_TSZ(O, G_TX_RENDERTILE, (uls)<<(G_TEXTURE_IMAGE_FRAC), (ult)<<(G_TEXTURE_IMAGE_FRAC), (lrs)<<(G_TEXTURE_IMAGE_FRAC), (lrt)<<(G_TEXTURE_IMAGE_FRAC)); \
} while (0)
#define nd_tex_load_tile_4b(...) NDTX_LOAD_TILE_4B(nd_nr, __VA_ARGS__)

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

#define NDTX_LOAD_BLOCK(O, timg, fmt, siz, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    NDTX_IMG(O, fmt, siz##_LOAD_BLOCK, 1, timg); \
    NDTX_TILE(O, fmt, siz##_LOAD_BLOCK, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_LDBLK(O, G_TX_LOADTILE, 0, 0, (((width)*(height) + siz##_INCR) >> siz##_SHIFT) -1, CALC_DXT(width, siz##_BYTES)); \
    NDTX_TILE(O, fmt, siz, (((width) * siz##_LINE_BYTES)+7)>>3, 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_TSZ(O, G_TX_RENDERTILE, 0, 0, ((width)-1) << G_TEXTURE_IMAGE_FRAC, ((height)-1) << G_TEXTURE_IMAGE_FRAC); \
} while (0)
/* the body of nd_tex_load_block is the gbi body with the O forms expanded to nd_nr: a variadic or named wrapper of NDTX_LOAD_BLOCK would
 * expand the size argument before its siz##_BYTES paste. */
#define nd_tex_load_block(timg, fmt, siz, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    nd_tex_img(fmt, siz##_LOAD_BLOCK, 1, timg); \
    nd_tex_tile(fmt, siz##_LOAD_BLOCK, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_ldblk(G_TX_LOADTILE, 0, 0, (((width)*(height) + siz##_INCR) >> siz##_SHIFT) -1, CALC_DXT(width, siz##_BYTES)); \
    nd_tex_tile(fmt, siz, (((width) * siz##_LINE_BYTES)+7)>>3, 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    nd_tex_tsz(G_TX_RENDERTILE, 0, 0, ((width)-1) << G_TEXTURE_IMAGE_FRAC, ((height)-1) << G_TEXTURE_IMAGE_FRAC); \
} while (0)

#define NDTX_LOAD_BLOCK_4B(O, timg, fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    NDTX_IMG(O, fmt, G_IM_SIZ_16b, 1, timg); \
    NDTX_TILE(O, fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_LDBLK(O, G_TX_LOADTILE, 0, 0, (((width)*(height)+3)>>2)-1, CALC_DXT_4b(width)); \
    NDTX_TILE(O, fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_TSZ(O, G_TX_RENDERTILE, 0, 0, ((width)-1) << G_TEXTURE_IMAGE_FRAC, ((height)-1) << G_TEXTURE_IMAGE_FRAC); \
} while (0)
#define nd_tex_load_block_4b(...) NDTX_LOAD_BLOCK_4B(nd_nr, __VA_ARGS__)

#define NDTX_LOAD_TLUT16(O, pal, dram) do { \
    NDTX_IMG(O, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, dram); \
    NDTX_TILE(O, 0, 0, 0, (256+(((pal)&0xf)*16)), G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0); \
    NDTX_LDTLUT(O, G_TX_LOADTILE, 15); \
} while (0)
#define nd_tex_load_tlut16(...) NDTX_LOAD_TLUT16(nd_nr, __VA_ARGS__)

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

#define NDTX_SCROLL_BLOCK_4B(O, timg, fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt) do { \
    NDTX_IMG(O, fmt, G_IM_SIZ_16b, 1, timg); \
    NDTX_TILE(O, fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_LDBLK(O, G_TX_LOADTILE, 0, 0, (((width)*(height)+3)>>2)-1, CALC_DXT_4b(width)); \
    NDTX_TILE(O, fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), 0, G_TX_RENDERTILE, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_TSZ(O, G_TX_RENDERTILE, (scrolls) << G_TEXTURE_IMAGE_FRAC, (scrollt) << G_TEXTURE_IMAGE_FRAC, \
               ((width)-1 + scrolls) << G_TEXTURE_IMAGE_FRAC, ((height)-1 + scrollt) << G_TEXTURE_IMAGE_FRAC); \
} while (0)
#define nd_tex_scroll_block_4b(...) NDTX_SCROLL_BLOCK_4B(nd_nr, __VA_ARGS__)
/* NMDL0: the multi-tile loads, the scrolling loads and the 256 colour palette load of make_texture_gfx (gbi.h gDPLoadMultiTile and its _4b,
 * gDPLoadTLUT_pal256; gbi_custom.h gDPScrollMultiTile, gDPScrollMultiTile_4b, gDPScrollTextureBlockHalfHeight and its _4b). Each sync is dropped.
 * The size argument is a literal G_IM_SIZ_* token (it is pasted). */
#define NDTX_LOAD_MTILE(O, timg, tmem, rtile, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    NDTX_IMG(O, fmt, siz, width, timg); \
    NDTX_TILE(O, fmt, siz, (((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), tmem, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_LDTILE(O, G_TX_LOADTILE, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
    NDTX_TILE(O, fmt, siz, (((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), tmem, rtile, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_TSZ(O, rtile, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
} while (0)
#define NDTX_LOAD_MTILE_4B(O, timg, tmem, rtile, fmt, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt) do { \
    NDTX_IMG(O, fmt, G_IM_SIZ_8b, ((width)>>1), timg); \
    NDTX_TILE(O, fmt, G_IM_SIZ_8b, (((((lrs)-(uls)+1)>>1)+7)>>3), tmem, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_LDTILE(O, G_TX_LOADTILE, (uls)<<(G_TEXTURE_IMAGE_FRAC-1), (ult)<<(G_TEXTURE_IMAGE_FRAC), (lrs)<<(G_TEXTURE_IMAGE_FRAC-1), (lrt)<<(G_TEXTURE_IMAGE_FRAC)); \
    NDTX_TILE(O, fmt, G_IM_SIZ_4b, (((((lrs)-(uls)+1)>>1)+7)>>3), tmem, rtile, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_TSZ(O, rtile, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
} while (0)
#define NDTX_SCROLL_MTILE(O, timg, tmem, rtile, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt) do { \
    NDTX_IMG(O, fmt, siz, width, timg); \
    NDTX_TILE(O, fmt, siz, (((((lrs)-(uls)+1) * siz##_TILE_BYTES)+7)>>3), tmem, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_LDTILE(O, G_TX_LOADTILE, (uls)<<G_TEXTURE_IMAGE_FRAC, (ult)<<G_TEXTURE_IMAGE_FRAC, (lrs)<<G_TEXTURE_IMAGE_FRAC, (lrt)<<G_TEXTURE_IMAGE_FRAC); \
    NDTX_TILE(O, fmt, siz, (((((lrs)-(uls)+1) * siz##_LINE_BYTES)+7)>>3), tmem, rtile, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_TSZ(O, rtile, ((uls)<<G_TEXTURE_IMAGE_FRAC) + scrolls, ((ult)<<G_TEXTURE_IMAGE_FRAC) + scrollt, \
             ((lrs)<<G_TEXTURE_IMAGE_FRAC) + scrolls, ((lrt)<<G_TEXTURE_IMAGE_FRAC) + scrollt); \
} while (0)
#define NDTX_SCROLL_MTILE_4B(O, timg, tmem, rtile, fmt, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt) do { \
    NDTX_IMG(O, fmt, G_IM_SIZ_8b, ((width)>>1), timg); \
    NDTX_TILE(O, fmt, G_IM_SIZ_8b, (((((lrs)-(uls)+1)>>1)+7)>>3), tmem, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_LDTILE(O, G_TX_LOADTILE, (uls)<<(G_TEXTURE_IMAGE_FRAC-1), (ult)<<(G_TEXTURE_IMAGE_FRAC), (lrs)<<(G_TEXTURE_IMAGE_FRAC-1), (lrt)<<(G_TEXTURE_IMAGE_FRAC)); \
    NDTX_TILE(O, fmt, G_IM_SIZ_4b, (((((lrs)-(uls)+1)>>1)+7)>>3), tmem, rtile, pal, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_TSZ(O, rtile, ((uls)<<G_TEXTURE_IMAGE_FRAC) + scrolls, ((ult)<<G_TEXTURE_IMAGE_FRAC) + scrollt, \
             ((lrs)<<G_TEXTURE_IMAGE_FRAC) + scrolls, ((lrt)<<G_TEXTURE_IMAGE_FRAC) + scrollt); \
} while (0)
#define NDTX_SCROLL_HH(O, timg, fmt, siz, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt, shifts2, shiftt2) do { \
    NDTX_IMG(O, fmt, siz##_LOAD_BLOCK, 1, timg); \
    NDTX_TILE(O, fmt, siz##_LOAD_BLOCK, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_LDBLK(O, G_TX_LOADTILE, 0, 0, (((width)*(height) + siz##_INCR) >> siz##_SHIFT) -1, CALC_DXT(width, siz##_BYTES)); \
    NDTX_TILE(O, fmt, siz, (((width) * siz##_LINE_BYTES)+7)>>3, 0, G_TX_RENDERTILE, pal, cmt, maskt - 1, shiftt, cms, masks, shifts); \
    NDTX_TSZ(O, G_TX_RENDERTILE, 0, 0, ((width)-1) << G_TEXTURE_IMAGE_FRAC, (((height) >> 1)-1) << G_TEXTURE_IMAGE_FRAC); \
    NDTX_TILE(O, fmt, siz, (((width) * siz##_LINE_BYTES)+7)>>3, (((width) * ((height) >> 1) * siz##_LINE_BYTES)+7)>>3, G_TX_RENDERTILE + 1, pal, cmt, maskt - 1, shiftt2, cms, masks, shifts2); \
    NDTX_TSZ(O, G_TX_RENDERTILE + 1, (scrolls), (scrollt), (((width)-1) << G_TEXTURE_IMAGE_FRAC) + (scrolls), ((((height) >> 1)-1) << G_TEXTURE_IMAGE_FRAC) + (scrollt)); \
} while (0)
#define NDTX_SCROLL_HH_4B(O, timg, fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt, scrolls, scrollt, shifts2, shiftt2) do { \
    NDTX_IMG(O, fmt, G_IM_SIZ_16b, 1, timg); \
    NDTX_TILE(O, fmt, G_IM_SIZ_16b, 0, 0, G_TX_LOADTILE, 0, cmt, maskt, shiftt, cms, masks, shifts); \
    NDTX_LDBLK(O, G_TX_LOADTILE, 0, 0, (((width)*(height)+3)>>2)-1, CALC_DXT_4b(width)); \
    NDTX_TILE(O, fmt, G_IM_SIZ_4b, ((((width)>>1)+7)>>3), 0, G_TX_RENDERTILE, pal, cmt, maskt - 1, shiftt, cms, masks, shifts); \
    NDTX_TSZ(O, G_TX_RENDERTILE, 0, 0, ((width)-1) << G_TEXTURE_IMAGE_FRAC, (((height) >> 1)-1) << G_TEXTURE_IMAGE_FRAC); \
    NDTX_TILE(O, fmt, G_IM_SIZ_4b, (((width)>>1)+7)>>3, (((width) * ((height) >> 1) / 2)+7)>>3, G_TX_RENDERTILE + 1, pal, cmt, maskt - 1, shiftt2, cms, masks, shifts2); \
    NDTX_TSZ(O, G_TX_RENDERTILE + 1, scrolls, scrollt, (((width)-1) << G_TEXTURE_IMAGE_FRAC) + scrolls, ((((height) >> 1)-1) << G_TEXTURE_IMAGE_FRAC) + scrollt); \
} while (0)
#define NDTX_LOAD_TLUT256(O, dram) do { \
    NDTX_IMG(O, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, dram); \
    NDTX_TILE(O, 0, 0, 0, 256, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0); \
    NDTX_LDTLUT(O, G_TX_LOADTILE, 255); \
} while (0)
#endif
