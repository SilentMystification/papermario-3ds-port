/* nd_gbi.h: DIRECT record-side redirect of the dynamic gbi macros (plan_direct.md section 5). include/common.h includes it after gbi.h for
 * every game file (a file that defines ND_GBI_WORDS before its includes keeps list words: the named fallbacks of plan section 5).
 * The base macros are redefined, so every derived and compound macro follows (gDPSetRenderMode, gDPLoadTextureBlock, gSPDisplayList, ...).
 * Each base macro evaluates its packet as gbi.h does (once per word) and calls nd_gx_w (nd_rec.c): a packet that is gMainGfxPos++ is taken
 * back and the op recorded into the open NDC_STATE run of the frame stream; any other packet (a local list) gets the word. The branch form
 * of G_DL stays a word (a branch skips data in the list); a top-level G_ENDDL is dropped (NDC_END ends the task or range). The texture rectangle
 * (G_TEXRECT or G_TEXRECTFLIP and its two RDPHALF words) is recorded as one NDC_TEXRECT. */
#ifndef ND_GBI_H
#define ND_GBI_H
#include "nd_cmd.h"

void nd_gx_w(Gfx* g, uint32_t w0, uint32_t w1);
/* D9: the top-level packet of the redirected macros: gMainGfxPos at the start of a task (nd_gx_w takes each packet back). */
Gfx* nd_rec_task(void);

#undef gSPSetOtherMode
#define gSPSetOtherMode(pkt, cmd, sft, len, data) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(cmd, 24, 8) | _SHIFTL(32 - (sft) - (len), 8, 8) | _SHIFTL((len) - 1, 0, 8), (unsigned int)(data)); }
#undef gSPGeometryMode
#define gSPGeometryMode(pkt, c, s) { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_GEOMETRYMODE, 24, 8) | _SHIFTL(~(u32)(c), 0, 24), (u32)(s)); }
#undef gSPTexture
#define gSPTexture(pkt, s, t, level, tile, on) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_TEXTURE, 24, 8) | _SHIFTL(BOWTIE_VAL, 16, 8) | _SHIFTL((level), 11, 3) | _SHIFTL((tile), 8, 3) | \
              _SHIFTL((on), 1, 7), _SHIFTL((s), 16, 16) | _SHIFTL((t), 0, 16)); }
#undef gSetImage
#define gSetImage(pkt, cmd, fmt, siz, width, i) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(cmd, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) | _SHIFTL((width) - 1, 0, 12), (unsigned int)(i)); }
#undef gDPSetCombineLERP
#define gDPSetCombineLERP(pkt, a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_SETCOMBINE, 24, 8) | _SHIFTL(GCCc0w0(G_CCMUX_##a0, G_CCMUX_##c0, G_ACMUX_##Aa0, G_ACMUX_##Ac0) | \
                                                                GCCc1w0(G_CCMUX_##a1, G_CCMUX_##c1), 0, 24), \
              (unsigned int)(GCCc0w1(G_CCMUX_##b0, G_CCMUX_##d0, G_ACMUX_##Ab0, G_ACMUX_##Ad0) | \
                             GCCc1w1(G_CCMUX_##b1, G_ACMUX_##Aa1, G_ACMUX_##Ac1, G_CCMUX_##d1, G_ACMUX_##Ab1, G_ACMUX_##Ad1))); }
#undef gDPSetColor
#define gDPSetColor(pkt, c, d) { nd_gx_w((Gfx*)(pkt), _SHIFTL(c, 24, 8), (unsigned int)(d)); }
#undef gDPSetPrimColor
#define gDPSetPrimColor(pkt, m, l, r, g, b, a) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_SETPRIMCOLOR, 24, 8) | _SHIFTL(m, 8, 8) | _SHIFTL(l, 0, 8), \
              _SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8)); }
#undef gDPNoParam
#define gDPNoParam(pkt, cmd) { nd_gx_w((Gfx*)(pkt), _SHIFTL(cmd, 24, 8), 0); }
#undef gImmp1
#define gImmp1(pkt, c, p0) { nd_gx_w((Gfx*)(pkt), _SHIFTL((c), 24, 8), (unsigned int)(p0)); }
#undef gDPSetScissor
#define gDPSetScissor(pkt, mode, ulx, uly, lrx, lry) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_SETSCISSOR, 24, 8) | _SHIFTL((int)((float)(ulx) * 4.0F), 12, 12) | _SHIFTL((int)((float)(uly) * 4.0F), 0, 12), \
              _SHIFTL(mode, 24, 2) | _SHIFTL((int)((float)(lrx) * 4.0F), 12, 12) | _SHIFTL((int)((float)(lry) * 4.0F), 0, 12)); }
#undef gDPSetScissorFrac
#define gDPSetScissorFrac(pkt, mode, ulx, uly, lrx, lry) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_SETSCISSOR, 24, 8) | _SHIFTL((int)((ulx)), 12, 12) | _SHIFTL((int)((uly)), 0, 12), \
              _SHIFTL(mode, 24, 2) | _SHIFTL((int)((lrx)), 12, 12) | _SHIFTL((int)((lry)), 0, 12)); }
#undef gDPFillRectangle
#define gDPFillRectangle(pkt, ulx, uly, lrx, lry) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_FILLRECT, 24, 8) | _SHIFTL((lrx), 14, 10) | _SHIFTL((lry), 2, 10), _SHIFTL((ulx), 14, 10) | _SHIFTL((uly), 2, 10)); }
#undef gDPScisFillRectangle
#define gDPScisFillRectangle(pkt, ulx, uly, lrx, lry) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_FILLRECT, 24, 8) | _SHIFTL(MAX((lrx), 0), 14, 10) | _SHIFTL(MAX((lry), 0), 2, 10), \
              _SHIFTL(MAX((ulx), 0), 14, 10) | _SHIFTL(MAX((uly), 0), 2, 10)); }
#undef gDPLoadTileGeneric
#define gDPLoadTileGeneric(pkt, c, tile, uls, ult, lrs, lrt) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(c, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12), _SHIFTL(tile, 24, 3) | _SHIFTL(lrs, 12, 12) | _SHIFTL(lrt, 0, 12)); }
#undef gDPSetTile
#define gDPSetTile(pkt, fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_SETTILE, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) | _SHIFTL(line, 9, 9) | _SHIFTL(tmem, 0, 9), \
              _SHIFTL(tile, 24, 3) | _SHIFTL(palette, 20, 4) | _SHIFTL(cmt, 18, 2) | _SHIFTL(maskt, 14, 4) | _SHIFTL(shiftt, 10, 4) | \
              _SHIFTL(cms, 8, 2) | _SHIFTL(masks, 4, 4) | _SHIFTL(shifts, 0, 4)); }
#undef gDPLoadBlock
#define gDPLoadBlock(pkt, tile, uls, ult, lrs, dxt) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_LOADBLOCK, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12), \
              _SHIFTL(tile, 24, 3) | _SHIFTL((MIN(lrs, G_TX_LDBLK_MAX_TXL)), 12, 12) | _SHIFTL(dxt, 0, 12)); }
#undef gDPLoadTLUTCmd
#define gDPLoadTLUTCmd(pkt, tile, count) { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_LOADTLUT, 24, 8), _SHIFTL((tile), 24, 3) | _SHIFTL((count), 14, 10)); }
#undef gDma0p
#define gDma0p(pkt, c, s, l) { nd_gx_w((Gfx*)(pkt), _SHIFTL((c), 24, 8) | _SHIFTL((l), 0, 24), (unsigned int)(s)); }
#undef gDma1p
#define gDma1p(pkt, c, s, l, p) { nd_gx_w((Gfx*)(pkt), _SHIFTL((c), 24, 8) | _SHIFTL((p), 16, 8) | _SHIFTL((l), 0, 16), (unsigned int)(s)); }
#undef gDma2p
#define gDma2p(pkt, c, adrs, len, idx, ofs) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL((c), 24, 8) | _SHIFTL(((len) - 1) / 8, 19, 5) | _SHIFTL((ofs) / 8, 8, 8) | _SHIFTL((idx), 0, 8), (unsigned int)(adrs)); }
#undef gSPVertex
#define gSPVertex(pkt, v, n, v0) { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_VTX, 24, 8) | _SHIFTL((n), 12, 8) | _SHIFTL((v0) + (n), 1, 7), (unsigned int)(v)); }
#undef gSP1Triangle
#define gSP1Triangle(pkt, v0, v1, v2, flag) { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_TRI1, 24, 8) | __gsSP1Triangle_w1f(v0, v1, v2, flag), 0); }
#undef gSP2Triangles
#define gSP2Triangles(pkt, v00, v01, v02, flag0, v10, v11, v12, flag1) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_TRI2, 24, 8) | __gsSP1Triangle_w1f(v00, v01, v02, flag0), __gsSP1Triangle_w1f(v10, v11, v12, flag1)); }
/* D7: the ops that gbi.h writes through a local packet pointer: the list end, the vertex modify, the chroma key colours. */
#undef gSPEndDisplayList
#define gSPEndDisplayList(pkt) { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_ENDDL, 24, 8), 0); }
#undef gSPModifyVertex
#define gSPModifyVertex(pkt, vtx, where, val) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_MODIFYVTX, 24, 8) | _SHIFTL((where), 16, 8) | _SHIFTL((vtx) * 2, 0, 16), (unsigned int)(val)); }
#undef gDPSetKeyR
#define gDPSetKeyR(pkt, cR, sR, wR) { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_SETKEYR, 24, 8), _SHIFTL(wR, 16, 12) | _SHIFTL(cR, 8, 8) | _SHIFTL(sR, 0, 8)); }
#undef gDPSetKeyGB
#define gDPSetKeyGB(pkt, cG, sG, wG, cB, sB, wB) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_SETKEYGB, 24, 8) | _SHIFTL(wG, 12, 12) | _SHIFTL(wB, 0, 12), \
              _SHIFTL(cG, 24, 8) | _SHIFTL(sG, 16, 8) | _SHIFTL(cB, 8, 8) | _SHIFTL(sB, 0, 8)); }
#undef gPmNdSplash
#define gPmNdSplash(pkt, on) { nd_gx_w((Gfx*)(pkt), ((u32)G_PM_ND << 24) | ((u32)G_PM_ND_SPLASH << 16) | ((on) ? 1u : 0u), 0); }
#undef gPmNdGrabHold
#define gPmNdGrabHold(pkt, on) { nd_gx_w((Gfx*)(pkt), ((u32)G_PM_ND << 24) | ((u32)G_PM_ND_GRABHOLD << 16) | ((on) ? 1u : 0u), 0); }
#undef gPmNdGutter
#define gPmNdGutter(pkt) { nd_gx_w((Gfx*)(pkt), ((u32)G_PM_ND << 24) | ((u32)G_PM_ND_GUTTER << 16), 0); }
/* The rectangle words: the first word through nd_gx_w, then the two RDPHALF words (gImmp1 above), as gbi.h writes them. */
#undef gSPTextureRectangle
#define gSPTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(xh, 12, 12) | _SHIFTL(yh, 0, 12), _SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) | _SHIFTL(yl, 0, 12)); \
      gImmp1(pkt, G_RDPHALF_1, (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16))); \
      gImmp1(pkt, G_RDPHALF_2, (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16))); }
#undef gSPTextureRectangleFlip
#define gSPTextureRectangleFlip(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_TEXRECTFLIP, 24, 8) | _SHIFTL(xh, 12, 12) | _SHIFTL(yh, 0, 12), _SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) | _SHIFTL(yl, 0, 12)); \
      gImmp1(pkt, G_RDPHALF_1, (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16))); \
      gImmp1(pkt, G_RDPHALF_2, (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16))); }
#undef gSPScisTextureRectangle
#define gSPScisTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    { nd_gx_w((Gfx*)(pkt), _SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(PM_SCIS_XH(xh), 12, 12) | _SHIFTL(MAX((s16)(yh), 0), 0, 12), \
              _SHIFTL((tile), 24, 3) | _SHIFTL(PM_SCIS_XL(xl), 12, 12) | _SHIFTL(MAX((s16)(yl), 0), 0, 12)); \
      gImmp1(pkt, G_RDPHALF_1, \
             (_SHIFTL(((s) - (PM_SCIS_CLIP(xl) ? (((s16)(dsdx) < 0) ? (MAX((((s16)(xl) * (s16)(dsdx)) >> 7), 0)) \
                                                                    : (MIN((((s16)(xl) * (s16)(dsdx)) >> 7), 0))) : 0)), 16, 16) | \
              _SHIFTL(((t) - (((yl) < 0) ? (((s16)(dtdy) < 0) ? (MAX((((s16)(yl) * (s16)(dtdy)) >> 7), 0)) \
                                                              : (MIN((((s16)(yl) * (s16)(dtdy)) >> 7), 0))) : 0)), 0, 16))); \
      gImmp1(pkt, G_RDPHALF_2, (_SHIFTL((dsdx), 16, 16) | _SHIFTL((dtdy), 0, 16))); }
#endif
