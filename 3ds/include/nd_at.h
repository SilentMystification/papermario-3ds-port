/* nd_at.h: the O forms of the state, value, transform and geometry setters (NSTREAM3). Each NDX_* macro takes an emitter O first:
 * O(k, v) writes one native record (k = the typed id and a field, v = the b field). The setter functions pass nd_nr (the core 1 stream,
 * nd_rec_nr); a core 0 local buffer passes ND_NR_AT (nd_nr.h, into the site's uint32_t* nrp) or HO (hud_element.c, nd_nr_o). The body of
 * each macro is the setter body of nd_st.c, nd_val.c, nd_xf.c and nd_geo.c with nd_nr( replaced by O(. */
#ifndef ND_AT_H
#define ND_AT_H
#include <stdint.h>
#include "nd_nr.h"
#include "nd_gbi.h"
#include "nd_fs.h"

#define NDST_RM(O, c0, c1) O(ND_NR_OM(0, G_MDSFT_RENDERMODE, 29), (unsigned)(c0) | (unsigned)(c1))
#define NDST_LUT(O, t) O(ND_NR_OM(1, G_MDSFT_TEXTLUT, 2), (unsigned)(t))
#define NDST_FILT(O, t) O(ND_NR_OM(1, G_MDSFT_TEXTFILT, 2), (unsigned)(t))
#define NDST_DITHER(O, t) O(ND_NR_OM(1, G_MDSFT_RGBDITHER, 2), (unsigned)(t))
#define NDST_CC(O, ...) O(ND_FS_W0(ND_FS_COMB, ND_W0_CMB(__VA_ARGS__)), ND_W1_CMB(__VA_ARGS__))
#define NDST_COMB(O, w0, w1) O(ND_FS_W0(ND_FS_COMB, (w0)), (w1))
/* NMDL0: texture on or off (gSPTexture: level and tile are not in the unit), alpha dither, an upper other mode word (gSPSetOtherMode H), the end unit
 * of a bind list (gSPEndDisplayList) */
#define NDST_TEX(O, s, t, level, tile, on) O(ND_NR_TEXON(on), ND_NR_HL(s, t))
#define NDST_ADITHER(O, t) O(ND_NR_OM(1, G_MDSFT_ALPHADITHER, 2), (unsigned)(t))
#define NDST_OMH(O, sft, len, data) O(ND_NR_OM(1, sft, len), (unsigned)(data))
#define NDST_END(O) O(ND_FS_W0(ND_FS_END, 0u), 0u)

#define NDVAL_PRIM(O, minlod, lodfrac, r, g, b, a) O(ND_FS_W0(ND_FS_PRIM, (((minlod) & 0xffu) << 8) | ((lodfrac) & 0xffu)), ND_NR_RGBA(r, g, b, a))
/* the b field of gDPSetFogColor and of gSPFogPosition (a value of a record word), and their O forms */
#define ND_NR_FOGPOS(min, max) ND_NR_HL((128000/((max)-(min))), ((500-(min))*256/((max)-(min))))
#define NDVAL_FOG(O, r, g, b, a) O(ND_FS_W0(ND_FS_FOG, 0u), ND_NR_RGBA(r, g, b, a))
#define NDVAL_FOGPOS(O, min, max) O(ND_NR_MW(ND_FS_MW_FOG, 0u), ND_NR_FOGPOS(min, max))
#define NDVAL_ENV(O, r, g, b, a) O(ND_FS_W0(ND_FS_ENV, 0u), ND_NR_RGBA(r, g, b, a))

#define NDXF_PUSH(O, m, p) O(ND_FS_W0(ND_FS_MTX, 0x380000u | (((p) ^ 1u) & 0xffu)), (uint32_t)(uintptr_t)(m))
#define NDXF_POP(O) O(ND_FS_W0(ND_FS_POPMTX, 0x380002u), 64u)

/* PBUF: the push call of a list (gSPDisplayList): the unit nd_gx_w gives for G_DL with G_DL_PUSH */
#define NDCTL_CALL(O, l) O(ND_OP_CALL_W0, (uint32_t)(uintptr_t)(l))
/* GPOSREST: a G_PM_ND mark (gbi_custom.h kinds splash, grab hold, gutter, sprite shade; arg 0 to 255): the unit nd_gx_w gives for gPmNd*(gMainGfxPos++, arg) */
#define NDCTL_ND(O, kind, arg) O(ND_X0(ND_W0_PMND((kind), (uint32_t)(arg) & 0xffu), 0u), ND_X1(ND_W0_PMND((kind), (uint32_t)(arg) & 0xffu), 0u))
#define nd_mark(kind, arg) NDCTL_ND(nd_nr, kind, arg)

#define NDGEO_VTX(O, v, n, v0) O(ND_NR_VTX(n, v0), (uint32_t)(uintptr_t)(v))
#define NDGEO_TRI1(O, a, b, c, f) O(ND_FS_W0(ND_FS_TRI1, __gsSP1Triangle_w1f(a, b, c, f)), 0u)
#define NDGEO_TRI2(O, a, b, c, f, d, e, g, h) O(ND_FS_W0(ND_FS_TRI2, __gsSP1Triangle_w1f(a, b, c, f)), __gsSP1Triangle_w1f(d, e, g, h))
#endif
