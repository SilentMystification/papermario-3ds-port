/* nd_geo.c: the target, depth, vertex, triangle and light calls of a call site (nd_geo.h). Each setter builds the typed unit(s) of the
 * macro's words (ND_X0, ND_X1 of nd_op.h) and records them into the open NDC_STATE run (nd_rec_state), the units the nd_gbi.h redirect
 * records for the macro, so the frame stream is the one the macro gave (the host test 3ds/tools/nd_geo_test.c, geounit). */
#include <stdint.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_op.h"
#include "nd_api.h"
#include "nd_cmd.h"
#include "nd_gbi.h"
#include "nd_geo.h"

static void geo_rec(uint32_t w0, uint32_t w1) {
    nd_rec_state(ND_X0(w0, w1), ND_X1(w0, w1));
}

void nd_tgt_cimgw(uint32_t fmt, uint32_t siz, uint32_t width, uint32_t img) {
    geo_rec(ND_W0_IMG(G_SETCIMG, fmt, siz, width), img);
}
void nd_pso_zsrc(uint32_t src) {
    geo_rec(ND_W0_SOM(G_SETOTHERMODE_L, G_MDSFT_ZSRCSEL, 1), src);
}
void nd_val_primz(uint32_t z, uint32_t dz) {
    geo_rec(_SHIFTL(G_SETPRIMDEPTH, 24, 8), _SHIFTL(z, 16, 16) | _SHIFTL(dz, 0, 16));
}
void nd_mesh_vtxw(uint32_t v, unsigned n, unsigned v0) {
    geo_rec(ND_W0_VTX(n, v0), v);
}
void nd_mesh_tri1(unsigned a, unsigned b, unsigned c, unsigned f) {
    geo_rec(_SHIFTL(G_TRI1, 24, 8) | __gsSP1Triangle_w1f(a, b, c, f), 0);
}
void nd_mesh_tri2(unsigned a, unsigned b, unsigned c, unsigned f, unsigned d, unsigned e, unsigned g, unsigned h) {
    geo_rec(_SHIFTL(G_TRI2, 24, 8) | __gsSP1Triangle_w1f(a, b, c, f), __gsSP1Triangle_w1f(d, e, g, h));
}
void nd_lit_1(uint32_t l0, uint32_t a) {
    geo_rec(ND_W0_MW(G_MW_NUMLIGHT, G_MWO_NUMLIGHT), NUML(NUMLIGHTS_1));
    geo_rec(ND_W0_D2(G_MOVEMEM, sizeof(Light), G_MV_LIGHT, 1 * 24 + 24), l0);
    geo_rec(ND_W0_D2(G_MOVEMEM, sizeof(Light), G_MV_LIGHT, 2 * 24 + 24), a);
}
void nd_lit_2(uint32_t l0, uint32_t l1, uint32_t a) {
    geo_rec(ND_W0_MW(G_MW_NUMLIGHT, G_MWO_NUMLIGHT), NUML(NUMLIGHTS_2));
    geo_rec(ND_W0_D2(G_MOVEMEM, sizeof(Light), G_MV_LIGHT, 1 * 24 + 24), l0);
    geo_rec(ND_W0_D2(G_MOVEMEM, sizeof(Light), G_MV_LIGHT, 2 * 24 + 24), l1);
    geo_rec(ND_W0_D2(G_MOVEMEM, sizeof(Light), G_MV_LIGHT, 3 * 24 + 24), a);
}
void nd_lit_lookatw(uint32_t la) {
    geo_rec(ND_W0_D2(G_MOVEMEM, sizeof(Light), G_MV_LIGHT, G_MVO_LOOKATX), la);
    geo_rec(ND_W0_D2(G_MOVEMEM, sizeof(Light), G_MV_LIGHT, G_MVO_LOOKATY), la + 16u);
}
void nd_xf_pnorm(unsigned s) {
    geo_rec(ND_W0_MW(G_MW_PERSPNORM, 0), (unsigned)(s));
}
void nd_xf_clipw(uint32_t neg, uint32_t pos) {
    geo_rec(ND_W0_MW(G_MW_CLIP, G_MWO_CLIP_RNX), neg);
    geo_rec(ND_W0_MW(G_MW_CLIP, G_MWO_CLIP_RNY), neg);
    geo_rec(ND_W0_MW(G_MW_CLIP, G_MWO_CLIP_RPX), pos);
    geo_rec(ND_W0_MW(G_MW_CLIP, G_MWO_CLIP_RPY), pos);
}
