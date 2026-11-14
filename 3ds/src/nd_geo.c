/* nd_geo.c: the target, depth, vertex, triangle and light calls of a call site (nd_geo.h). Each setter records its native record (nd_nr.h)
 * from its own fields into the open native record run (nd_rec_nr, through nd_nr): the same unit words the macro gave
 * (nd_op.h), so the frame stream is the one the macro gave (the host test 3ds/tools/nd_geo_test.c, geounit). */
#include <stdint.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_op.h"
#include "nd_api.h"
#include "nd_cmd.h"
#include "nd_nr.h"
#include "nd_geo.h"

void nd_tgt_cimgw(uint32_t fmt, uint32_t siz, uint32_t width, uint32_t img) {
    (void)fmt;
    nd_nr(ND_NR_CIMG(siz, width), img);
}
void nd_pso_zsrc(uint32_t src) {
    nd_nr(ND_NR_OM(0, G_MDSFT_ZSRCSEL, 1), src);
}
void nd_val_primz(uint32_t z, uint32_t dz) {
    nd_nr(NDO_W0(NDO_PRIMZ, 0u), ND_NR_HL(z, dz));
}
void nd_mesh_vtxw(uint32_t v, unsigned n, unsigned v0) {
    nd_nr(ND_NR_VTX(n, v0), v);
}
void nd_mesh_tri1(unsigned a, unsigned b, unsigned c, unsigned f) {
    nd_nr(NDO_W0(NDO_TRI1, __gsSP1Triangle_w1f(a, b, c, f)), 0u);
}
void nd_mesh_tri2(unsigned a, unsigned b, unsigned c, unsigned f, unsigned d, unsigned e, unsigned g, unsigned h) {
    nd_nr(NDO_W0(NDO_TRI2, __gsSP1Triangle_w1f(a, b, c, f)), __gsSP1Triangle_w1f(d, e, g, h));
}
void nd_lit_1(uint32_t l0, uint32_t a) {
    nd_nr(ND_NR_MW(NDO_MW_NUMLIGHT, 0u), NUML(NUMLIGHTS_1));
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, 1 * 24 + 24), l0);
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, 2 * 24 + 24), a);
}
void nd_lit_2(uint32_t l0, uint32_t l1, uint32_t a) {
    nd_nr(ND_NR_MW(NDO_MW_NUMLIGHT, 0u), NUML(NUMLIGHTS_2));
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, 1 * 24 + 24), l0);
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, 2 * 24 + 24), l1);
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, 3 * 24 + 24), a);
}
void nd_lit_lookatw(uint32_t la) {
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, G_MVO_LOOKATX), la);
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, G_MVO_LOOKATY), la + 16u);
}
void nd_xf_pnorm(unsigned s) {
    nd_nr(ND_NR_MW(NDO_MW_NONE, 0u), (unsigned)(s));
}
void nd_xf_clipw(uint32_t neg, uint32_t pos) {
    nd_nr(ND_NR_MW(NDO_MW_NONE, 0u), neg);
    nd_nr(ND_NR_MW(NDO_MW_NONE, 0u), neg);
    nd_nr(ND_NR_MW(NDO_MW_NONE, 0u), pos);
    nd_nr(ND_NR_MW(NDO_MW_NONE, 0u), pos);
}
