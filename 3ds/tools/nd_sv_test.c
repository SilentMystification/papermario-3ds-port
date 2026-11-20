/* nd_sv_test.c: host check of the native value records (NSET1, nd_sv_enc and nd_sv_sync of nd_ds.c). 15 cases: each applies 1 to 3 units to a zeroed NdDs, then nd_sv_enc and nd_sv_sync of the last unit.
 * Prints `nset1 cases <n> bad <b>`. --fault adds 1 to the expected v[1] of case 1 (bad must be above 0). */
#include <stdio.h>
#include <string.h>
#include "nd_ds.h"
#include "nd_op.h"
#include "nd_gtab.h"
#include "nd_fs.h"
#include "nd_sv.h"
#include <stdint.h>
#include <stdlib.h>

int pm_wide;
unsigned short* nuGfxZBuffer; /* NDR0B: the z-buffer address of the window rule in ds_td0 */
int is_host(u32 p) { return p >= 0x00100000u; }
void tex_win_s25(int* a, int* b, int* c, int* d, unsigned e, unsigned f, unsigned g, unsigned h) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; }
/* NST2A: no device table and no v1 table in the host test. */
const NdNDev* nd_ntab_find(const void* list) { (void)list; return NULL; }
const NdGTab* nd_gtab_find(const void* list) { (void)list; return NULL; }

typedef struct { unsigned n; uint32_t u[3][2]; NdNapiRun want; uint32_t v0, v1; unsigned sync; } SvCase;
static const SvCase cases[15] = {
    { 1, { { ND_FS_W0(ND_FS_OMH, 12 | 2 << 8), 0x2000u } }, nd_sv_omh, 0x3000u, 0x2000u, 0 },
    { 1, { { ND_FS_W0(ND_FS_OMH, 20 | 2 << 8), 0x100000u } }, nd_sv_omh, 0x300000u, 0x100000u, 1 },
    { 1, { { ND_FS_W0(ND_FS_OML, 3 | 29 << 8), 0xffffffffu } }, nd_sv_oml, 0xfffffff8u, 0xfffffff8u, 0 },
    { 1, { { ND_FS_W0(ND_FS_PRIM, 0x1ff), 0x11223344u } }, nd_sv_prim, 0x1ffu, 0x11223344u, 1 },
    { 1, { { ND_FS_W0(ND_FS_PRIMZ, 0), 0x12345678u } }, nd_sv_primz, 0u, 0x1234u, 0 },
    { 1, { { ND_FS_W0(ND_FS_SCISSOR, 8 << 12 | 16), 1280u << 12 | 960u } }, nd_sv_sci, 0x40002u, 0xf00140u, 0 },
    { 1, { { ND_FS_W0(ND_FS_MW, ND_FS_MW_NUMLIGHT << 16), 72u } }, nd_sv_nlt, 0u, 3u, 0 },
    { 1, { { ND_FS_W0(ND_FS_MW, ND_FS_MW_SEG << 16 | 6), 0x200000u } }, nd_sv_seg, 6u, 0x200000u, 0 },
    { 1, { { ND_FS_W0(ND_FS_RARE, ND_FS_RARE_KEYR), 0x1234u } }, nd_sv_keyr, 0x12000000u, 0x34000000u, 1 },
    { 1, { { ND_FS_W0(ND_FS_COMB, 0x123456), 0x789abcdeu } }, nd_sv_comb, 0xfc123456u, 0x789abcdeu, 1 },
    { 1, { { ND_FS_W0(ND_FS_TIMG, 2 << 21 | 1 << 19 | 63), 0x300000u } }, nd_sv_timg, 0x48003fu, 0x300000u, 0 },
    { 1, { { ND_FS_W0(ND_FS_TILESIZE, 4 << 12 | 8), 2u << 24 | 100u << 12 | 60u } }, nd_sv_tsz, 0x20010008u, 0x3c0064u, 0 },
    { 3, { { ND_FS_W0(ND_FS_TILE, 304), 7u << 24 }, { ND_FS_W0(ND_FS_TIMG, 0), 0x400000u }, { ND_FS_W0(ND_FS_LOADTLUT, 0), 7u << 24 | 15u << 14 } }, nd_sv_tlut, 0x1003u, 0x400000u, 0 },
    { 1, { { ND_FS_W0(ND_FS_LOADTLUT, 0), 0u } }, nd_sv_tlutn, 0x1f0u, 0u, 0 },
    { 1, { { ND_FS_W0(ND_FS_LOADBLOCK, 0), 0u } }, nd_sv_nop, 0u, 0u, 0 },
};

int main(int argc, char** argv) {
    const int fault = argc > 1 && strcmp(argv[1], "--fault") == 0;
    unsigned bad = 0;
    for (unsigned c = 0; c < 15u; c++) {
        const SvCase* k = &cases[c];
        NdDs s; memset(&s, 0, sizeof s);
        for (unsigned i = 0; i < k->n; i++) nd_ds_apply(&s, k->u[i][0], k->u[i][1]);
        const uint32_t w0 = k->u[k->n - 1u][0], w1 = k->u[k->n - 1u][1];
        uint32_t v[2] = { 0u, 0u };
        const NdNapiRun f = nd_sv_enc(&s, w0, w1, v);
        const uint32_t e1 = k->v1 + (fault && c == 0u ? 1u : 0u);
        if (f != k->want || v[0] != k->v0 || v[1] != e1 || nd_sv_sync(w0) != k->sync) { bad++; printf("BAD case %u\n", c + 1u); }
    }
    printf("nset1 cases 15 bad %u\n", bad);
    return bad > 0 ? 1 : 0;
}
