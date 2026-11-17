/* nd_st.c: the state calls of a call site (nd_st.h). Each setter records its native record (nd_nr.h) from its own fields into the open
 * native record run (nd_rec_nr, through nd_nr): the same unit words the macro gave (nd_op.h), so the frame stream is the
 * one the macro gave (the host test 3ds/tools/nd_st_test.c, stunit). */
#include <stdint.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_op.h"
#include "nd_api.h"
#include "nd_cmd.h"
#include "nd_nr.h"
#include "nd_at.h"
#include "nd_st.h"

void nd_pso_comb(uint32_t w0, uint32_t w1) {
    NDST_COMB(nd_nr, w0, w1);
}
void nd_pso_unit(uint32_t x0, uint32_t x1) {
    if (x0 == ND_NOP_A) return; /* nd_gx_w drops a no-op unit (nd_rec.c IS_NOP) */
    nd_nr(x0, x1);
}
void nd_pso_rm(unsigned c0, unsigned c1) {
    NDST_RM(nd_nr, c0, c1);
}
void nd_pso_cyc(unsigned t) {
    nd_nr(ND_NR_OM(1, G_MDSFT_CYCLETYPE, 2), (unsigned)(t));
}
void nd_pso_filt(unsigned t) {
    NDST_FILT(nd_nr, t);
}
void nd_pso_persp(unsigned t) {
    nd_nr(ND_NR_OM(1, G_MDSFT_TEXTPERSP, 1), (unsigned)(t));
}
void nd_pso_lut(unsigned t) {
    NDST_LUT(nd_nr, t);
}
void nd_pso_dither(unsigned t) {
    NDST_DITHER(nd_nr, t);
}
void nd_pso_alcmp(unsigned t) {
    nd_nr(ND_NR_OM(0, G_MDSFT_ALPHACOMPARE, 2), (unsigned)(t));
}
void nd_pso_aldith(unsigned t) {
    nd_nr(ND_NR_OM(1, G_MDSFT_ALPHADITHER, 2), (unsigned)(t));
}
void nd_pso_lod(unsigned t) {
    nd_nr(ND_NR_OM(1, G_MDSFT_TEXTLOD, 1), (unsigned)(t));
}
void nd_pso_detail(unsigned t) {
    nd_nr(ND_NR_OM(1, G_MDSFT_TEXTDETAIL, 2), (unsigned)(t));
}
void nd_pso_conv(unsigned t) {
    nd_nr(ND_NR_OM(1, G_MDSFT_TEXTCONV, 3), (unsigned)(t));
}
void nd_pso_combkey(unsigned t) {
    nd_nr(ND_NR_OM(1, G_MDSFT_COMBKEY, 1), (unsigned)(t));
}
void nd_pso_som(unsigned cmd, unsigned sft, unsigned len, unsigned data) {
    nd_nr(ND_NR_OM((cmd) == G_SETOTHERMODE_H, sft, len), (unsigned int)(data));
}
void nd_pso_geom_set(unsigned w) {
    nd_nr(ND_NR_GEOM(0), (u32)(w));
}
void nd_pso_geom_clr(unsigned w) {
    nd_nr(ND_NR_GEOM(w), (u32)(0));
}
void nd_tex_on(unsigned s, unsigned t, unsigned level, unsigned tile, unsigned on) {
    (void)level;
    (void)tile;
    nd_nr(ND_NR_TEXON(on), ND_NR_HL(s, t));
}
void nd_pso_scis(unsigned mode, float ulx, float uly, float lrx, float lry) {
    nd_nr(NDO_W0(NDO_SCISSOR, ND_NR_XY12((int)((float)(ulx) * 4.0F), (int)((float)(uly) * 4.0F))),
          ND_NR_XY12((int)((float)(lrx) * 4.0F), (int)((float)(lry) * 4.0F)));
    (void)mode;
}
