/* nd_st.c: the state calls of a call site (nd_st.h). Each setter builds the typed unit(s) of the macro's words (ND_X0, ND_X1 of nd_op.h)
 * and records them into the open NDC_STATE run (nd_rec_state), the units the nd_gbi.h redirect records for the macro, so the frame stream is
 * the one the macro gave (the host test 3ds/tools/nd_st_test.c, stunit). */
#include <stdint.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_op.h"
#include "nd_api.h"
#include "nd_cmd.h"
#include "nd_gbi.h"
#include "nd_st.h"

static void st_rec(uint32_t w0, uint32_t w1) {
    nd_rec_state(ND_X0(w0, w1), ND_X1(w0, w1));
}

void nd_pso_comb(uint32_t w0, uint32_t w1) {
    st_rec(w0, w1);
}
void nd_pso_rm(unsigned c0, unsigned c1) {
    st_rec(ND_W0_SOM(G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29), (unsigned)(c0) | (unsigned)(c1));
}
void nd_pso_cyc(unsigned t) {
    st_rec(ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_CYCLETYPE, 2), (unsigned)(t));
}
void nd_pso_filt(unsigned t) {
    st_rec(ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_TEXTFILT, 2), (unsigned)(t));
}
void nd_pso_persp(unsigned t) {
    st_rec(ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_TEXTPERSP, 1), (unsigned)(t));
}
void nd_pso_lut(unsigned t) {
    st_rec(ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_TEXTLUT, 2), (unsigned)(t));
}
void nd_pso_dither(unsigned t) {
    st_rec(ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_RGBDITHER, 2), (unsigned)(t));
}
void nd_pso_alcmp(unsigned t) {
    st_rec(ND_W0_SOM(G_SETOTHERMODE_L, G_MDSFT_ALPHACOMPARE, 2), (unsigned)(t));
}
void nd_pso_aldith(unsigned t) {
    st_rec(ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_ALPHADITHER, 2), (unsigned)(t));
}
void nd_pso_lod(unsigned t) {
    st_rec(ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_TEXTLOD, 1), (unsigned)(t));
}
void nd_pso_detail(unsigned t) {
    st_rec(ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_TEXTDETAIL, 2), (unsigned)(t));
}
void nd_pso_conv(unsigned t) {
    st_rec(ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_TEXTCONV, 3), (unsigned)(t));
}
void nd_pso_combkey(unsigned t) {
    st_rec(ND_W0_SOM(G_SETOTHERMODE_H, G_MDSFT_COMBKEY, 1), (unsigned)(t));
}
void nd_pso_som(unsigned cmd, unsigned sft, unsigned len, unsigned data) {
    st_rec(ND_W0_SOM(cmd, sft, len), (unsigned int)(data));
}
void nd_pso_geom_set(unsigned w) {
    st_rec(ND_W0_GEOM(0), (u32)(w));
}
void nd_pso_geom_clr(unsigned w) {
    st_rec(ND_W0_GEOM(w), (u32)(0));
}
void nd_tex_on(unsigned s, unsigned t, unsigned level, unsigned tile, unsigned on) {
    st_rec(ND_W0_TEX(level, tile, on), ND_W1_TEX(s, t));
}
void nd_pso_scis(unsigned mode, float ulx, float uly, float lrx, float lry) {
    st_rec(ND_W0_SCIS(ulx, uly), ND_W1_SCIS(mode, lrx, lry));
}
