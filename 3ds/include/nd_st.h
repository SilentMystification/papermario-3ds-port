/* nd_st.h: the state calls of a call site (nd_st.c). Each setter takes the macro's arguments minus the packet and records the same typed
 * unit(s) the nd_gbi.h redirect records for the macro (nd_rec_nr; a texture rectangle is nd_rec_texrect). The words are the macro's
 * (gbi.h, the ND_W0_* / ND_W1_* helpers of nd_gbi.h). Shape: gDPSetCycleType(gMainGfxPos++, t) -> nd_pso_cyc(t).
 * gDPSetCombineMode(P, a, b) -> nd_pso_cc(a, b): a macro, because each G_CC_* argument is 8 tokens (the 16 ND_W0_CMB arguments). */
#ifndef ND_ST_H
#define ND_ST_H
#include <stdint.h>
#include "nd_gbi.h"
#define ND_SOM_H 0xe3u /* DEL3C2: G_SETOTHERMODE_H of gbi.h, the high word value of the cmd argument of nd_pso_som */

void nd_pso_comb(uint32_t w0, uint32_t w1);
/* GFXPOS7: one typed unit as ND_CMB_W or ND_CMB_NOP stores it (nd_gbi.h; draw_box.c gBoxCombineModes, the window styles). The no-op unit
 * records nothing; any other unit is one native record. The same stream as nd_gx_w(gMainGfxPos++, x0, x1) of a state unit. */
void nd_pso_unit(uint32_t x0, uint32_t x1);
#define nd_pso_cc(...) nd_pso_comb(ND_W0_CMB(__VA_ARGS__), ND_W1_CMB(__VA_ARGS__))

/* G_SETOTHERMODE_H / _L (gbi.h gDPSet*): the mode words */
void nd_pso_rm(unsigned c0, unsigned c1);      /* gDPSetRenderMode (L, G_MDSFT_RENDERMODE, 29) */
void nd_pso_cyc(unsigned t);                   /* gDPSetCycleType (H, G_MDSFT_CYCLETYPE, 2) */
void nd_pso_filt(unsigned t);                  /* gDPSetTextureFilter (H, G_MDSFT_TEXTFILT, 2) */
void nd_pso_persp(unsigned t);                 /* gDPSetTexturePersp (H, G_MDSFT_TEXTPERSP, 1) */
void nd_pso_lut(unsigned t);                   /* gDPSetTextureLUT (H, G_MDSFT_TEXTLUT, 2) */
void nd_pso_dither(unsigned t);                /* gDPSetColorDither (H, G_MDSFT_RGBDITHER, 2: the gbi.h branch of not _HW_VERSION_1) */
void nd_pso_alcmp(unsigned t);                 /* gDPSetAlphaCompare (L, G_MDSFT_ALPHACOMPARE, 2) */
void nd_pso_aldith(unsigned t);                /* gDPSetAlphaDither (H, G_MDSFT_ALPHADITHER, 2) */
void nd_pso_lod(unsigned t);                   /* gDPSetTextureLOD (H, G_MDSFT_TEXTLOD, 1) */
void nd_pso_detail(unsigned t);                /* gDPSetTextureDetail (H, G_MDSFT_TEXTDETAIL, 2) */
void nd_pso_conv(unsigned t);                  /* gDPSetTextureConvert (H, G_MDSFT_TEXTCONV, 3) */
void nd_pso_combkey(unsigned t);               /* gDPSetCombineKey (H, G_MDSFT_COMBKEY, 1) */
void nd_pso_som(unsigned cmd, unsigned sft, unsigned len, unsigned data); /* gSPSetOtherMode (any cmd, sft, len) */

/* gSPGeometryMode: set (clear mask 0) and clear (set mask 0), as gSPSetGeometryMode / gSPClearGeometryMode */
void nd_pso_geom_set(unsigned w);
void nd_pso_geom_clr(unsigned w);

/* gSPTexture(P, s, t, level, tile, on) */
void nd_tex_on(unsigned s, unsigned t, unsigned level, unsigned tile, unsigned on);

/* gDPSetScissor(P, mode, ulx, uly, lrx, lry): the 10.2 fields as float, as the macro */
void nd_pso_scis(unsigned mode, float ulx, float uly, float lrx, float lry);
#endif
