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
#include "nd_at.h"
#include "nd_geo.h"
#include "nd_fs.h"

extern unsigned short* nuGfxCfb_ptr;
void nd_snap_tgt(uint32_t w0, uint32_t w1); /* NDR0C: prototype (fix: implicit-declaration warning in geounit); nd_snap.c strong, weak no-op below */
/* the record: one unit, w0 = the G_SETCIMG fields of the walker (siz << 12 | width - 1), w1 = the physical address (the identity for host addresses) */
void nd_target_beginw(uint32_t img, uint32_t siz, uint32_t width) {
    const uint32_t rec[2] = { (siz & 3u) << 12 | ((width - 1u) & 0xfffu), img };
    nd_rec_napi(nd_tgt_run, rec, 1u);
    nd_snap_tgt(rec[0], rec[1]);
}
void nd_target_end(void) {
    nd_target_beginw((uint32_t)(uintptr_t)nuGfxCfb_ptr, G_IM_SIZ_16b, 320u);
}
void nd_pso_zsrc(uint32_t src) {
    nd_nr(ND_NR_OM(0, G_MDSFT_ZSRCSEL, 1), src);
}
void nd_val_primz(uint32_t z, uint32_t dz) {
    nd_nr(ND_FS_W0(ND_FS_PRIMZ, 0u), ND_NR_HL(z, dz));
}
void nd_mesh_vtxw(uint32_t v, unsigned n, unsigned v0) {
    NDGEO_VTX(nd_nr, v, n, v0);
}
/* NDR0A: the snapchk take (nd_snap.c, strong). This weak no-op serves the host units that link nd_geo.c without nd_snap.c. */
__attribute__((weak)) void nd_snap_mesh(void) {}
/* NDR0C: the target record of the snapshot (nd_snap.c, strong). Weak no-op for the host units. */
__attribute__((weak)) void nd_snap_tgt(uint32_t w0, uint32_t w1) { (void)w0; (void)w1; }
void nd_mesh_tri1(unsigned a, unsigned b, unsigned c, unsigned f) {
    NDGEO_TRI1(nd_nr, a, b, c, f);
}
void nd_mesh_tri2(unsigned a, unsigned b, unsigned c, unsigned f, unsigned d, unsigned e, unsigned g, unsigned h) {
    NDGEO_TRI2(nd_nr, a, b, c, f, d, e, g, h);
}
void nd_lit_1(uint32_t l0, uint32_t a) {
    nd_nr(ND_NR_MW(ND_FS_MW_NUMLIGHT, 0u), NUML(NUMLIGHTS_1));
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, 1 * 24 + 24), l0);
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, 2 * 24 + 24), a);
}
void nd_lit_2(uint32_t l0, uint32_t l1, uint32_t a) {
    nd_nr(ND_NR_MW(ND_FS_MW_NUMLIGHT, 0u), NUML(NUMLIGHTS_2));
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, 1 * 24 + 24), l0);
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, 2 * 24 + 24), l1);
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, 3 * 24 + 24), a);
}
void nd_lit_lookatw(uint32_t la) {
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, G_MVO_LOOKATX), la);
    nd_nr(ND_NR_MOVEMEM(sizeof(Light), G_MV_LIGHT, G_MVO_LOOKATY), la + 16u);
}
void nd_xf_pnorm(unsigned s) {
    nd_nr(ND_NR_MW(ND_FS_MW_NONE, 0u), (unsigned)(s));
}
void nd_xf_clipw(uint32_t neg, uint32_t pos) {
    nd_nr(ND_NR_MW(ND_FS_MW_NONE, 0u), neg);
    nd_nr(ND_NR_MW(ND_FS_MW_NONE, 0u), neg);
    nd_nr(ND_NR_MW(ND_FS_MW_NONE, 0u), pos);
    nd_nr(ND_NR_MW(ND_FS_MW_NONE, 0u), pos);
}
