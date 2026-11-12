/* nd_gbi.h: DIRECT record-side redirect of the dynamic gbi macros (plan_direct.md section 5). include/common.h includes it after gbi.h for
 * every game file (a file that defines ND_GBI_WORDS before its includes keeps list words: the named fallbacks of plan section 5).
 * The base macros are redefined, so every derived and compound macro follows (gDPSetRenderMode, gDPLoadTextureBlock, gSPDisplayList, ...).
 * Each base macro (g form, gs form) builds the typed unit of nd_op.h (ND_X0, ND_X1: the id and fields of the op, no N64 word) and the g form calls
 * nd_gx_w (nd_rec.c) with it: a packet that is gMainGfxPos++ is taken back and the unit recorded into the open NDC_STATE run of the frame stream;
 * any other packet (a local list) gets the unit. The branch form of G_DL (NDO_CALL, a = 1) stays a unit of the list (a branch skips data in the
 * list); a top-level NDO_END is dropped (NDC_END ends the task or range). The texture rectangle (NDO_RECT with NDO_RECT_TEX and its two
 * no-op units of the RDPHALF words) is recorded as one NDC_TEXRECT. The gs form is the same unit in a static initializer. */
#ifndef ND_GBI_H
#define ND_GBI_H
#include "nd_cmd.h"
#include "nd_op.h"
void nd_gx_w(Gfx* g, uint32_t w0, uint32_t w1);
/* D9: the top-level packet of the redirected macros: gMainGfxPos at the start of a task (nd_gx_w takes each packet back). */
Gfx* nd_rec_task(void);
/* HC4e3 group 1 (scalar ops). ND_GX: the g form, one evaluation of each word, the typed unit of nd_op.h to nd_gx_w. ND_GS: the gs form (a static
 * initializer): the typed unit at compile time. The w0 and w1 expressions are those of include/PR/gbi.h (F3DEX_GBI_2). */
#define ND_GX(pkt, w0, w1) { const u32 _nd0 = (w0), _nd1 = (w1); nd_gx_w((Gfx*)(pkt), ND_X0(_nd0, _nd1), ND_X1(_nd0, _nd1)); }
#define ND_GS(w0, w1) {{ ND_X0(w0, w1), ND_X1(w0, w1) }}
#define ND_W0_SOM(cmd, sft, len) (_SHIFTL(cmd, 24, 8) | _SHIFTL(32 - (sft) - (len), 8, 8) | _SHIFTL((len) - 1, 0, 8))
#undef gSPSetOtherMode
#define gSPSetOtherMode(pkt, cmd, sft, len, data) ND_GX(pkt, ND_W0_SOM(cmd, sft, len), (unsigned int)(data))
#undef gsSPSetOtherMode
#define gsSPSetOtherMode(cmd, sft, len, data) ND_GS(ND_W0_SOM(cmd, sft, len), (unsigned int)(data))
#define ND_W0_GEOM(c) (_SHIFTL(G_GEOMETRYMODE, 24, 8) | _SHIFTL(~(u32)(c), 0, 24))
#undef gSPGeometryMode
#define gSPGeometryMode(pkt, c, s) ND_GX(pkt, ND_W0_GEOM(c), (u32)(s))
#undef gsSPGeometryMode
#define gsSPGeometryMode(c, s) ND_GS(ND_W0_GEOM(c), (u32)(s))
#define ND_W0_TEX(level, tile, on) (_SHIFTL(G_TEXTURE, 24, 8) | _SHIFTL(BOWTIE_VAL, 16, 8) | _SHIFTL((level), 11, 3) | _SHIFTL((tile), 8, 3) | _SHIFTL((on), 1, 7))
#define ND_W1_TEX(s, t) (_SHIFTL((s), 16, 16) | _SHIFTL((t), 0, 16))
#undef gSPTexture
#define gSPTexture(pkt, s, t, level, tile, on) ND_GX(pkt, ND_W0_TEX(level, tile, on), ND_W1_TEX(s, t))
#undef gsSPTexture
#define gsSPTexture(s, t, level, tile, on) ND_GS(ND_W0_TEX(level, tile, on), ND_W1_TEX(s, t))
#define ND_W0_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
    (_SHIFTL(G_SETCOMBINE, 24, 8) | _SHIFTL(GCCc0w0(G_CCMUX_##a0, G_CCMUX_##c0, G_ACMUX_##Aa0, G_ACMUX_##Ac0) | GCCc1w0(G_CCMUX_##a1, G_CCMUX_##c1), 0, 24))
#define ND_W1_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
    ((unsigned int)(GCCc0w1(G_CCMUX_##b0, G_CCMUX_##d0, G_ACMUX_##Ab0, G_ACMUX_##Ad0) | \
                    GCCc1w1(G_CCMUX_##b1, G_ACMUX_##Aa1, G_ACMUX_##Ac1, G_CCMUX_##d1, G_ACMUX_##Ab1, G_ACMUX_##Ad1)))
#undef gDPSetCombineLERP
#define gDPSetCombineLERP(pkt, a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
    ND_GX(pkt, ND_W0_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1), \
          ND_W1_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1))
#undef gsDPSetCombineLERP
#define gsDPSetCombineLERP(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
    ND_GS(ND_W0_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1), \
          ND_W1_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1))
#undef gDPSetColor
#define gDPSetColor(pkt, c, d) ND_GX(pkt, _SHIFTL(c, 24, 8), (unsigned int)(d))
#undef gsDPSetColor
#define gsDPSetColor(c, d) ND_GS(_SHIFTL(c, 24, 8), (unsigned int)(d))
#define ND_W0_PRIMC(m, l) (_SHIFTL(G_SETPRIMCOLOR, 24, 8) | _SHIFTL(m, 8, 8) | _SHIFTL(l, 0, 8))
#define ND_W1_PRIMC(r, g, b, a) (_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8))
#undef gDPSetPrimColor
#define gDPSetPrimColor(pkt, m, l, r, g, b, a) ND_GX(pkt, ND_W0_PRIMC(m, l), ND_W1_PRIMC(r, g, b, a))
#undef gsDPSetPrimColor
#define gsDPSetPrimColor(m, l, r, g, b, a) ND_GS(ND_W0_PRIMC(m, l), ND_W1_PRIMC(r, g, b, a))
#define ND_W0_SCIS(ulx, uly) (_SHIFTL(G_SETSCISSOR, 24, 8) | _SHIFTL((int)((float)(ulx) * 4.0F), 12, 12) | _SHIFTL((int)((float)(uly) * 4.0F), 0, 12))
#define ND_W1_SCIS(mode, lrx, lry) (_SHIFTL(mode, 24, 2) | _SHIFTL((int)((float)(lrx) * 4.0F), 12, 12) | _SHIFTL((int)((float)(lry) * 4.0F), 0, 12))
#undef gDPSetScissor
#define gDPSetScissor(pkt, mode, ulx, uly, lrx, lry) ND_GX(pkt, ND_W0_SCIS(ulx, uly), ND_W1_SCIS(mode, lrx, lry))
#undef gsDPSetScissor
#define gsDPSetScissor(mode, ulx, uly, lrx, lry) ND_GS(ND_W0_SCIS(ulx, uly), ND_W1_SCIS(mode, lrx, lry))
#define ND_W0_SCISF(ulx, uly) (_SHIFTL(G_SETSCISSOR, 24, 8) | _SHIFTL((int)((ulx)), 12, 12) | _SHIFTL((int)((uly)), 0, 12))
#define ND_W1_SCISF(mode, lrx, lry) (_SHIFTL(mode, 24, 2) | _SHIFTL((int)((lrx)), 12, 12) | _SHIFTL((int)((lry)), 0, 12))
#undef gDPSetScissorFrac
#define gDPSetScissorFrac(pkt, mode, ulx, uly, lrx, lry) ND_GX(pkt, ND_W0_SCISF(ulx, uly), ND_W1_SCISF(mode, lrx, lry))
#undef gsDPSetScissorFrac
#define gsDPSetScissorFrac(mode, ulx, uly, lrx, lry) ND_GS(ND_W0_SCISF(ulx, uly), ND_W1_SCISF(mode, lrx, lry))
/* G_MOVEWORD: gbi.h builds it with gDma1p (data = w1, offset = low 16 bits, index = bits 16 to 23); gDma1p itself stays a word until group 3. */
#define ND_W0_MW(index, offset) (_SHIFTL(G_MOVEWORD, 24, 8) | _SHIFTL((index), 16, 8) | _SHIFTL((offset), 0, 16))
#undef gMoveWd
#define gMoveWd(pkt, index, offset, data) ND_GX(pkt, ND_W0_MW(index, offset), (unsigned int)(data))
#undef gsMoveWd
#define gsMoveWd(index, offset, data) ND_GS(ND_W0_MW(index, offset), (unsigned int)(data))
#define ND_W0_CONV(k0, k1, k2) (_SHIFTL(G_SETCONVERT, 24, 8) | _SHIFTL(k0, 13, 9) | _SHIFTL(k1, 4, 9) | _SHIFTR(k2, 5, 4))
#define ND_W1_CONV(k2, k3, k4, k5) (_SHIFTL(k2, 27, 5) | _SHIFTL(k3, 18, 9) | _SHIFTL(k4, 9, 9) | _SHIFTL(k5, 0, 9))
#undef gDPSetConvert
#define gDPSetConvert(pkt, k0, k1, k2, k3, k4, k5) ND_GX(pkt, ND_W0_CONV(k0, k1, k2), ND_W1_CONV(k2, k3, k4, k5))
#undef gsDPSetConvert
#define gsDPSetConvert(k0, k1, k2, k3, k4, k5) ND_GS(ND_W0_CONV(k0, k1, k2), ND_W1_CONV(k2, k3, k4, k5))
#define ND_W1_KEYR(cR, sR, wR) (_SHIFTL(wR, 16, 12) | _SHIFTL(cR, 8, 8) | _SHIFTL(sR, 0, 8))
#undef gDPSetKeyR
#define gDPSetKeyR(pkt, cR, sR, wR) ND_GX(pkt, _SHIFTL(G_SETKEYR, 24, 8), ND_W1_KEYR(cR, sR, wR))
#undef gsDPSetKeyR
#define gsDPSetKeyR(cR, sR, wR) ND_GS(_SHIFTL(G_SETKEYR, 24, 8), ND_W1_KEYR(cR, sR, wR))
#define ND_W0_KEYGB(wG, wB) (_SHIFTL(G_SETKEYGB, 24, 8) | _SHIFTL(wG, 12, 12) | _SHIFTL(wB, 0, 12))
#define ND_W1_KEYGB(cG, sG, cB, sB) (_SHIFTL(cG, 24, 8) | _SHIFTL(sG, 16, 8) | _SHIFTL(cB, 8, 8) | _SHIFTL(sB, 0, 8))
#undef gDPSetKeyGB
#define gDPSetKeyGB(pkt, cG, sG, wG, cB, sB, wB) ND_GX(pkt, ND_W0_KEYGB(wG, wB), ND_W1_KEYGB(cG, sG, cB, sB))
#undef gsDPSetKeyGB
#define gsDPSetKeyGB(cG, sG, wG, cB, sB, wB) ND_GS(ND_W0_KEYGB(wG, wB), ND_W1_KEYGB(cG, sG, cB, sB))
/* HC4e3 group 2 (texture ops). */
#define ND_W0_IMG(cmd, fmt, siz, width) (_SHIFTL(cmd, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) | _SHIFTL((width) - 1, 0, 12))
#undef gSetImage
#define gSetImage(pkt, cmd, fmt, siz, width, i) ND_GX(pkt, ND_W0_IMG(cmd, fmt, siz, width), (unsigned int)(i))
#undef gsSetImage
#define gsSetImage(cmd, fmt, siz, width, i) ND_GS(ND_W0_IMG(cmd, fmt, siz, width), (unsigned int)(i))
#define ND_W0_LDT(c, uls, ult) (_SHIFTL(c, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12))
#define ND_W1_LDT(tile, lrs, lrt) (_SHIFTL(tile, 24, 3) | _SHIFTL(lrs, 12, 12) | _SHIFTL(lrt, 0, 12))
#undef gDPLoadTileGeneric
#define gDPLoadTileGeneric(pkt, c, tile, uls, ult, lrs, lrt) ND_GX(pkt, ND_W0_LDT(c, uls, ult), ND_W1_LDT(tile, lrs, lrt))
#undef gsDPLoadTileGeneric
#define gsDPLoadTileGeneric(c, tile, uls, ult, lrs, lrt) ND_GS(ND_W0_LDT(c, uls, ult), ND_W1_LDT(tile, lrs, lrt))
#define ND_W0_TILE(fmt, siz, line, tmem) (_SHIFTL(G_SETTILE, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) | _SHIFTL(line, 9, 9) | _SHIFTL(tmem, 0, 9))
#define ND_W1_TILE(tile, palette, cmt, maskt, shiftt, cms, masks, shifts) \
    (_SHIFTL(tile, 24, 3) | _SHIFTL(palette, 20, 4) | _SHIFTL(cmt, 18, 2) | _SHIFTL(maskt, 14, 4) | _SHIFTL(shiftt, 10, 4) | \
     _SHIFTL(cms, 8, 2) | _SHIFTL(masks, 4, 4) | _SHIFTL(shifts, 0, 4))
#undef gDPSetTile
#define gDPSetTile(pkt, fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts) \
    ND_GX(pkt, ND_W0_TILE(fmt, siz, line, tmem), ND_W1_TILE(tile, palette, cmt, maskt, shiftt, cms, masks, shifts))
#undef gsDPSetTile
#define gsDPSetTile(fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts) \
    ND_GS(ND_W0_TILE(fmt, siz, line, tmem), ND_W1_TILE(tile, palette, cmt, maskt, shiftt, cms, masks, shifts))
#define ND_W0_LDB(uls, ult) (_SHIFTL(G_LOADBLOCK, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12))
#define ND_W1_LDB(tile, lrs, dxt) (_SHIFTL(tile, 24, 3) | _SHIFTL((MIN(lrs, G_TX_LDBLK_MAX_TXL)), 12, 12) | _SHIFTL(dxt, 0, 12))
#undef gDPLoadBlock
#define gDPLoadBlock(pkt, tile, uls, ult, lrs, dxt) ND_GX(pkt, ND_W0_LDB(uls, ult), ND_W1_LDB(tile, lrs, dxt))
#undef gsDPLoadBlock
#define gsDPLoadBlock(tile, uls, ult, lrs, dxt) ND_GS(ND_W0_LDB(uls, ult), ND_W1_LDB(tile, lrs, dxt))
#undef gDPLoadTLUTCmd
#define gDPLoadTLUTCmd(pkt, tile, count) ND_GX(pkt, _SHIFTL(G_LOADTLUT, 24, 8), _SHIFTL((tile), 24, 3) | _SHIFTL((count), 14, 10))
#undef gsDPLoadTLUTCmd
#define gsDPLoadTLUTCmd(tile, count) ND_GS(_SHIFTL(G_LOADTLUT, 24, 8), _SHIFTL((tile), 24, 3) | _SHIFTL((count), 14, 10))
/* HC4e3 group 3 (geometry, control, sync, rectangle and G_PM ops). A sync, G_SPNOOP, G_NOOP and a RDPHALF word are the typed no-op unit (NDO_MISC). */
#define ND_W0_NP(cmd) _SHIFTL(cmd, 24, 8)
#undef gDPNoParam
#define gDPNoParam(pkt, cmd) ND_GX(pkt, ND_W0_NP(cmd), 0)
#undef gsDPNoParam
#define gsDPNoParam(cmd) ND_GS(ND_W0_NP(cmd), 0)
#undef gDPParam
#define gDPParam(pkt, cmd, param) ND_GX(pkt, ND_W0_NP(cmd), (param))
#undef gsDPParam
#define gsDPParam(cmd, param) ND_GS(ND_W0_NP(cmd), (param))
#undef gImmp1
#define gImmp1(pkt, c, p0) ND_GX(pkt, _SHIFTL((c), 24, 8), (unsigned int)(p0))
#undef gsImmp1
#define gsImmp1(c, p0) ND_GS(_SHIFTL((c), 24, 8), (unsigned int)(p0))
#define ND_W0_FILL(lrx, lry) (_SHIFTL(G_FILLRECT, 24, 8) | _SHIFTL((lrx), 14, 10) | _SHIFTL((lry), 2, 10))
#define ND_W1_FILL(ulx, uly) (_SHIFTL((ulx), 14, 10) | _SHIFTL((uly), 2, 10))
#undef gDPFillRectangle
#define gDPFillRectangle(pkt, ulx, uly, lrx, lry) ND_GX(pkt, ND_W0_FILL(lrx, lry), ND_W1_FILL(ulx, uly))
#undef gsDPFillRectangle
#define gsDPFillRectangle(ulx, uly, lrx, lry) ND_GS(ND_W0_FILL(lrx, lry), ND_W1_FILL(ulx, uly))
#undef gDPScisFillRectangle
#define gDPScisFillRectangle(pkt, ulx, uly, lrx, lry) \
    ND_GX(pkt, _SHIFTL(G_FILLRECT, 24, 8) | _SHIFTL(MAX((lrx), 0), 14, 10) | _SHIFTL(MAX((lry), 0), 2, 10), \
          _SHIFTL(MAX((ulx), 0), 14, 10) | _SHIFTL(MAX((uly), 0), 2, 10))
#define ND_W0_D0(c, l) (_SHIFTL((c), 24, 8) | _SHIFTL((l), 0, 24))
#undef gDma0p
#define gDma0p(pkt, c, s, l) ND_GX(pkt, ND_W0_D0(c, l), (unsigned int)(s))
#undef gsDma0p
#define gsDma0p(c, s, l) ND_GS(ND_W0_D0(c, l), (unsigned int)(s))
#define ND_W0_D1(c, l, p) (_SHIFTL((c), 24, 8) | _SHIFTL((p), 16, 8) | _SHIFTL((l), 0, 16))
#undef gDma1p
#define gDma1p(pkt, c, s, l, p) ND_GX(pkt, ND_W0_D1(c, l, p), (unsigned int)(s))
#undef gsDma1p
#define gsDma1p(c, s, l, p) ND_GS(ND_W0_D1(c, l, p), (unsigned int)(s))
#define ND_W0_D2(c, len, idx, ofs) (_SHIFTL((c), 24, 8) | _SHIFTL(((len) - 1) / 8, 19, 5) | _SHIFTL((ofs) / 8, 8, 8) | _SHIFTL((idx), 0, 8))
#undef gDma2p
#define gDma2p(pkt, c, adrs, len, idx, ofs) ND_GX(pkt, ND_W0_D2(c, len, idx, ofs), (unsigned int)(adrs))
#undef gsDma2p
#define gsDma2p(c, adrs, len, idx, ofs) ND_GS(ND_W0_D2(c, len, idx, ofs), (unsigned int)(adrs))
#define ND_W0_VTX(n, v0) (_SHIFTL(G_VTX, 24, 8) | _SHIFTL((n), 12, 8) | _SHIFTL((v0) + (n), 1, 7))
#undef gSPVertex
#define gSPVertex(pkt, v, n, v0) ND_GX(pkt, ND_W0_VTX(n, v0), (unsigned int)(v))
#undef gsSPVertex
#define gsSPVertex(v, n, v0) ND_GS(ND_W0_VTX(n, v0), (unsigned int)(v))
#undef gSP1Triangle
#define gSP1Triangle(pkt, v0, v1, v2, flag) ND_GX(pkt, _SHIFTL(G_TRI1, 24, 8) | __gsSP1Triangle_w1f(v0, v1, v2, flag), 0)
#undef gsSP1Triangle
#define gsSP1Triangle(v0, v1, v2, flag) ND_GS(_SHIFTL(G_TRI1, 24, 8) | __gsSP1Triangle_w1f(v0, v1, v2, flag), 0)
#undef gSP2Triangles
#define gSP2Triangles(pkt, v00, v01, v02, flag0, v10, v11, v12, flag1) \
    ND_GX(pkt, _SHIFTL(G_TRI2, 24, 8) | __gsSP1Triangle_w1f(v00, v01, v02, flag0), __gsSP1Triangle_w1f(v10, v11, v12, flag1))
#undef gsSP2Triangles
#define gsSP2Triangles(v00, v01, v02, flag0, v10, v11, v12, flag1) \
    ND_GS(_SHIFTL(G_TRI2, 24, 8) | __gsSP1Triangle_w1f(v00, v01, v02, flag0), __gsSP1Triangle_w1f(v10, v11, v12, flag1))
/* The list end: NDO_END (a top-level one is dropped by nd_gx_w: NDC_END ends the task or range). */
#undef gSPEndDisplayList
#define gSPEndDisplayList(pkt) ND_GX(pkt, _SHIFTL(G_ENDDL, 24, 8), 0)
#undef gsSPEndDisplayList
#define gsSPEndDisplayList() ND_GS(_SHIFTL(G_ENDDL, 24, 8), 0)
#define ND_W0_MODV(vtx, where) (_SHIFTL(G_MODIFYVTX, 24, 8) | _SHIFTL((where), 16, 8) | _SHIFTL((vtx) * 2, 0, 16))
#undef gSPModifyVertex
#define gSPModifyVertex(pkt, vtx, where, val) ND_GX(pkt, ND_W0_MODV(vtx, where), (unsigned int)(val))
#undef gsSPModifyVertex
#define gsSPModifyVertex(vtx, where, val) ND_GS(ND_W0_MODV(vtx, where), (unsigned int)(val))
#define ND_W0_PMND(kind, arg) (((u32)G_PM_ND << 24) | ((u32)(kind) << 16) | (arg))
#undef gPmNdSplash
#define gPmNdSplash(pkt, on) ND_GX(pkt, ND_W0_PMND(G_PM_ND_SPLASH, (on) ? 1u : 0u), 0)
#undef gPmNdGrabHold
#define gPmNdGrabHold(pkt, on) ND_GX(pkt, ND_W0_PMND(G_PM_ND_GRABHOLD, (on) ? 1u : 0u), 0)
#undef gPmNdGutter
#undef gPmNdSprShade
#define gPmNdSprShade(pkt, on) ND_GX(pkt, ND_W0_PMND(G_PM_ND_SPRSHADE, (on) ? 1u : 0u), 0)
#undef gPmNdGutter
#define gPmNdGutter(pkt, a) ND_GX(pkt, ND_W0_PMND(G_PM_ND_GUTTER, (u32)(a) & 0xffu), 0) /* PAUSEOPEN: through nd_gx_w (the ND recorder); the gbi_custom.h form writes the word raw and the walker never sees it */
/* The texture rectangle: the rectangle unit (NDO_RECT with NDO_RECT_TEX), then the two RDPHALF no-op units (their b: s, t and dsdx, dtdy), as gbi.h writes the words. */
#define ND_W0_TR(op, xh, yh) (_SHIFTL(op, 24, 8) | _SHIFTL(xh, 12, 12) | _SHIFTL(yh, 0, 12))
#define ND_W1_TR(tile, xl, yl) (_SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) | _SHIFTL(yl, 0, 12))
#define ND_TR_H1(s, t) (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16))
#define ND_TR_H2(dsdx, dtdy) (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16))
#undef gSPTextureRectangle
#define gSPTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    { ND_GX(pkt, ND_W0_TR(G_TEXRECT, xh, yh), ND_W1_TR(tile, xl, yl)); \
      gImmp1(pkt, G_RDPHALF_1, ND_TR_H1(s, t)); \
      gImmp1(pkt, G_RDPHALF_2, ND_TR_H2(dsdx, dtdy)); }
#undef gsSPTextureRectangle
#define gsSPTextureRectangle(xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    ND_GS(ND_W0_TR(G_TEXRECT, xh, yh), ND_W1_TR(tile, xl, yl)), gsImmp1(G_RDPHALF_1, ND_TR_H1(s, t)), gsImmp1(G_RDPHALF_2, ND_TR_H2(dsdx, dtdy))
#undef gSPTextureRectangleFlip
#define gSPTextureRectangleFlip(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    { ND_GX(pkt, ND_W0_TR(G_TEXRECTFLIP, xh, yh), ND_W1_TR(tile, xl, yl)); \
      gImmp1(pkt, G_RDPHALF_1, ND_TR_H1(s, t)); \
      gImmp1(pkt, G_RDPHALF_2, ND_TR_H2(dsdx, dtdy)); }
#undef gsSPTextureRectangleFlip
#define gsSPTextureRectangleFlip(xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    ND_GS(ND_W0_TR(G_TEXRECTFLIP, xh, yh), ND_W1_TR(tile, xl, yl)), gsImmp1(G_RDPHALF_1, ND_TR_H1(s, t)), gsImmp1(G_RDPHALF_2, ND_TR_H2(dsdx, dtdy))
#undef gSPScisTextureRectangle
#define gSPScisTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy) \
    { ND_GX(pkt, _SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(PM_SCIS_XH(xh), 12, 12) | _SHIFTL(MAX((s16)(yh), 0), 0, 12), \
            _SHIFTL((tile), 24, 3) | _SHIFTL(PM_SCIS_XL(xl), 12, 12) | _SHIFTL(MAX((s16)(yl), 0), 0, 12)); \
      gImmp1(pkt, G_RDPHALF_1, \
             (_SHIFTL(((s) - (PM_SCIS_CLIP(xl) ? (((s16)(dsdx) < 0) ? (MAX((((s16)(xl) * (s16)(dsdx)) >> 7), 0)) \
                                                                    : (MIN((((s16)(xl) * (s16)(dsdx)) >> 7), 0))) : 0)), 16, 16) | \
              _SHIFTL(((t) - (((yl) < 0) ? (((s16)(dtdy) < 0) ? (MAX((((s16)(yl) * (s16)(dtdy)) >> 7), 0)) \
                                                              : (MIN((((s16)(yl) * (s16)(dtdy)) >> 7), 0))) : 0)), 0, 16))); \
      gImmp1(pkt, G_RDPHALF_2, (_SHIFTL((dsdx), 16, 16) | _SHIFTL((dtdy), 0, 16))); }
#endif
