/* nd_gbi_tex.h: DIRECT D4b redirect of the compound texture loads and the texture rectangles (plan_direct.md section 5). A file that includes
 * it after gbi.h records gDPLoadTLUT_pal16, gDPLoadTextureBlock_4b, gDPLoadTextureTile_4b, gDPLoadTextureTile, gSPTextureRectangle and
 * gSPScisTextureRectangle as frame stream commands (NDC_TEX4, NDC_TEXRECT) instead of list words. The packet is evaluated once; it must be
 * gMainGfxPos++ (nd_rec.c checks it). Inside a list for later (nd_rec_sub > 0) the words are written as before.
 * D4b-3: the single-word state macros (other mode, combine, colours, geometry mode, texture, scissor, fill rectangle, image, tile, load and sync
 * ops; every compound macro built from them) record NDC_STATE ops (nd_gx_w): the base macros below are redefined, so each derived macro
 * follows. A packet that is not gMainGfxPos++ keeps its word.
 * Converted files: src/msg_draw.c, src/hud_element.c, src/draw_box.c, src/draw_img_util.c. */
#ifndef ND_GBI_TEX_H
#define ND_GBI_TEX_H
#include "nd_cmd.h"

void nd_gx_tlut16(Gfx* g, unsigned slot, const void* dram);
void nd_gx_tex4(Gfx* g, const void* timg, unsigned fmt, unsigned w, unsigned h, int uls, int ult, int lrs, int lrt, unsigned pal,
                unsigned cms, unsigned cmt, unsigned masks, unsigned maskt, unsigned shifts, unsigned shiftt, unsigned tile_form);
void nd_gx_texrect(Gfx* g, int xl, int yl, int xh, int yh, unsigned tile, int s, int t, int dsdx, int dtdy);
void nd_gx_textile(Gfx* g, const void* timg, unsigned fmt, unsigned siz, unsigned w, unsigned h, int uls, int ult, int lrs, int lrt, unsigned pal,
                   unsigned cms, unsigned cmt, unsigned masks, unsigned maskt, unsigned shifts, unsigned shiftt);
void nd_gx_scisrect(Gfx* g, int xl, int yl, int xh, int yh, unsigned tile, int s, int t, int dsdx, int dtdy);

void nd_gx_w(Gfx* g, uint32_t w0, uint32_t w1);

#undef gSPSetOtherMode
#define gSPSetOtherMode(pkt, cmd, sft, len, data)     { nd_gx_w((Gfx*)(pkt), _SHIFTL(cmd, 24, 8) | _SHIFTL(32 - (sft) - (len), 8, 8) | _SHIFTL((len) - 1, 0, 8), (unsigned int)(data)); }
#undef gSPGeometryMode
#define gSPGeometryMode(pkt, c, s) { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_GEOMETRYMODE, 24, 8) | _SHIFTL(~(u32)(c), 0, 24), (u32)(s)); }
#undef gSPTexture
#define gSPTexture(pkt, s, t, level, tile, on)     { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_TEXTURE, 24, 8) | _SHIFTL(BOWTIE_VAL, 16, 8) | _SHIFTL((level), 11, 3) | _SHIFTL((tile), 8, 3) | _SHIFTL((on), 1, 7),               _SHIFTL((s), 16, 16) | _SHIFTL((t), 0, 16)); }
#undef gSetImage
#define gSetImage(pkt, cmd, fmt, siz, width, i)     { nd_gx_w((Gfx*)(pkt), _SHIFTL(cmd, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) | _SHIFTL((width) - 1, 0, 12), (unsigned int)(i)); }
#undef gDPSetCombineLERP
#define gDPSetCombineLERP(pkt, a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1)     { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_SETCOMBINE, 24, 8) | _SHIFTL(GCCc0w0(G_CCMUX_##a0, G_CCMUX_##c0, G_ACMUX_##Aa0, G_ACMUX_##Ac0) |                                                             GCCc1w0(G_CCMUX_##a1, G_CCMUX_##c1), 0, 24),               (unsigned int)(GCCc0w1(G_CCMUX_##b0, G_CCMUX_##d0, G_ACMUX_##Ab0, G_ACMUX_##Ad0) |                              GCCc1w1(G_CCMUX_##b1, G_ACMUX_##Aa1, G_ACMUX_##Ac1, G_CCMUX_##d1, G_ACMUX_##Ab1, G_ACMUX_##Ad1))); }
#undef gDPSetColor
#define gDPSetColor(pkt, c, d) { nd_gx_w((Gfx*)(pkt), _SHIFTL(c, 24, 8), (unsigned int)(d)); }
#undef gDPSetPrimColor
#define gDPSetPrimColor(pkt, m, l, r, g, b, a)     { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_SETPRIMCOLOR, 24, 8) | _SHIFTL(m, 8, 8) | _SHIFTL(l, 0, 8),               _SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8)); }
#undef gDPNoParam
#define gDPNoParam(pkt, cmd) { nd_gx_w((Gfx*)(pkt), _SHIFTL(cmd, 24, 8), 0); }
#undef gDPSetScissor
#define gDPSetScissor(pkt, mode, ulx, uly, lrx, lry)     { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_SETSCISSOR, 24, 8) | _SHIFTL((int)((float)(ulx) * 4.0F), 12, 12) | _SHIFTL((int)((float)(uly) * 4.0F), 0, 12),               _SHIFTL(mode, 24, 2) | _SHIFTL((int)((float)(lrx) * 4.0F), 12, 12) | _SHIFTL((int)((float)(lry) * 4.0F), 0, 12)); }
#undef gDPSetScissorFrac
#define gDPSetScissorFrac(pkt, mode, ulx, uly, lrx, lry)     { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_SETSCISSOR, 24, 8) | _SHIFTL((int)((ulx)), 12, 12) | _SHIFTL((int)((uly)), 0, 12),               _SHIFTL(mode, 24, 2) | _SHIFTL((int)((lrx)), 12, 12) | _SHIFTL((int)((lry)), 0, 12)); }
#undef gDPFillRectangle
#define gDPFillRectangle(pkt, ulx, uly, lrx, lry)     { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_FILLRECT, 24, 8) | _SHIFTL((lrx), 14, 10) | _SHIFTL((lry), 2, 10), _SHIFTL((ulx), 14, 10) | _SHIFTL((uly), 2, 10)); }
#undef gDPScisFillRectangle
#define gDPScisFillRectangle(pkt, ulx, uly, lrx, lry)     { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_FILLRECT, 24, 8) | _SHIFTL(MAX((lrx), 0), 14, 10) | _SHIFTL(MAX((lry), 0), 2, 10),               _SHIFTL(MAX((ulx), 0), 14, 10) | _SHIFTL(MAX((uly), 0), 2, 10)); }
#undef gDPLoadTileGeneric
#define gDPLoadTileGeneric(pkt, c, tile, uls, ult, lrs, lrt)     { nd_gx_w((Gfx*)(pkt), _SHIFTL(c, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12), _SHIFTL(tile, 24, 3) | _SHIFTL(lrs, 12, 12) | _SHIFTL(lrt, 0, 12)); }
#undef gDPSetTile
#define gDPSetTile(pkt, fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts)     { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_SETTILE, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) | _SHIFTL(line, 9, 9) | _SHIFTL(tmem, 0, 9),               _SHIFTL(tile, 24, 3) | _SHIFTL(palette, 20, 4) | _SHIFTL(cmt, 18, 2) | _SHIFTL(maskt, 14, 4) | _SHIFTL(shiftt, 10, 4) |               _SHIFTL(cms, 8, 2) | _SHIFTL(masks, 4, 4) | _SHIFTL(shifts, 0, 4)); }
#undef gDPLoadBlock
#define gDPLoadBlock(pkt, tile, uls, ult, lrs, dxt)     { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_LOADBLOCK, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12),               _SHIFTL(tile, 24, 3) | _SHIFTL((MIN(lrs, G_TX_LDBLK_MAX_TXL)), 12, 12) | _SHIFTL(dxt, 0, 12)); }
#undef gDPLoadTLUTCmd
#define gDPLoadTLUTCmd(pkt, tile, count) { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_LOADTLUT, 24, 8), _SHIFTL((tile), 24, 3) | _SHIFTL((count), 14, 10)); }

#undef gDPLoadTLUT_pal16
#define gDPLoadTLUT_pal16(pkt, pal, dram) nd_gx_tlut16((Gfx*)(pkt), (pal), (dram))
#undef gDPLoadTextureBlock_4b
#define gDPLoadTextureBlock_4b(pkt, timg, fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt) \
    nd_gx_tex4((Gfx*)(pkt), (timg), (fmt), (width), (height), 0, 0, 0, 0, (pal), (cms), (cmt), (masks), (maskt), (shifts), (shiftt), 0)
#undef gDPLoadTextureTile_4b
#define gDPLoadTextureTile_4b(pkt, timg, fmt, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt) \
    nd_gx_tex4((Gfx*)(pkt), (timg), (fmt), (width), (height), (uls), (ult), (lrs), (lrt), (pal), (cms), (cmt), (masks), (maskt), (shifts), (shiftt), 1)
#undef gDPLoadTextureTile
#define gDPLoadTextureTile(pkt, timg, fmt, siz, width, height, uls, ult, lrs, lrt, pal, cms, cmt, masks, maskt, shifts, shiftt)     nd_gx_textile((Gfx*)(pkt), (const void*)(timg), (fmt), (siz), (width), (height), (uls), (ult), (lrs), (lrt), (pal), (cms), (cmt), (masks), (maskt), (shifts), (shiftt))
#undef gSPScisTextureRectangle
#define gSPScisTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy)     nd_gx_scisrect((Gfx*)(pkt), (xl), (yl), (xh), (yh), (tile), (s), (t), (dsdx), (dtdy))
#undef gSPTextureRectangle
#define gSPTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    nd_gx_texrect((Gfx*)(pkt), (xl), (yl), (xh), (yh), (tile), (s), (t), (dsdx), (dtdy))
#endif
