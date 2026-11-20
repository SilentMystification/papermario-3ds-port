/* nd_at_test.c: host test of the O forms of the setters (nd_at.h, nd_tx.h, nd_nr.h; NSTREAM3). Each shape: the gbi macro (A) and the O form (B)
 * must give the same units. Prints "ns3unit ok N/N", or "ns3unit FAIL <shape>" for the first shape that differs, and exits 1. The stubs are
 * those of nd_tx_test.c (TEXUNIT). */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ultra64.h"
#include "variables.h"
#include "pm_port.h"
#include "gbi_custom.h"
#include "nd.h"
#include "nd_api.h"
#include "nd_gtab.h"
#include "nd_cmd.h"
#include "nd_gbi.h"
#include "nd_tx.h"
#include "nd_at.h"

/* the redirect target of the macro form: gMainGfxPos is the top-level packet pointer (nd_gbi.h D9) */
Gfx* gMainGfxPos;
s32 gCurrentDisplayContextIndex;
const NdNDev* const nd_rm_dev[0x3d]; /* nd_rmtab.c: the render mode lists, all NULL here */
int pm_wide;
int is_host(u32 p) { (void)p; return 0; }
char __end__[1];
u8 gMapShapeData[1];
void pm_log(const char* fmt, ...) { (void)fmt; }
void pm_log_e(const char* fmt, ...) { (void)fmt; }
void pm_log_evt(const char* fmt, ...) { (void)fmt; }
void pm_fatal_stop(const char* msg) { (void)msg; }
int pm_debug_has(const char* name) { (void)name; return 0; } /* the switches are off in the host test (nd_model.c PM5) */
unsigned pm_dbg_gen;
int pm_interp_repl;
const float (*pm_interp_find(const void* m))[4] { (void)m; return 0; }
uint32_t f32tof24(float f) { (void)f; return 0; } /* nd_tmpl.c: libctru, not used here */
void nd_walk_rec_model(const Gfx* q) { (void)q; }
void nd_walk_spr(const Gfx* q) { (void)q; }
void nd_walk_ent(const Gfx* q, unsigned words) { (void)q; (void)words; }
void nd_texrect_run(const uint32_t* p) { (void)p; }
void nd_parts_run(const uint32_t* rec) { (void)rec; }
void nd_parts_run_v(const uint32_t* rec) { (void)rec; }
const NdGTab* nd_gtab_find(const void* list) { (void)list; return 0; }
uint32_t nd_mkey_a(const uint32_t* rec) { (void)rec; return 0; }

/* the walker side: nd_stream_run hands each state run to nd_walk_state; the double logs each unit (word pair) */
static uint32_t ulog[512]; static unsigned nulog;
void nd_nr_run(const uint32_t* q) { const unsigned n = (q[-2] >> 16) & 0xffu;
    for (unsigned i = 0; i < n && nulog < 256u; i++) { ulog[2u * nulog] = q[2u * i]; ulog[2u * nulog + 1u] = q[2u * i + 1u]; nulog++; }
}
#include "nd_rec_log.h" /* NAT6G1 */

/* nd_model.c defines nd_r11_state (MDL-5f); nd_recs.c reads it */
int nd_r11_state(uint32_t addr, const uint32_t** nr, uint32_t* cw, int* has_cb, uint32_t* mo, unsigned* nmo);


static uint8_t gTimg[1024];

/* the gbi side (A) writes its units into pk (nd_gx_w keeps the words of a local packet, nd_rec.c); the O side (B) writes into bb through nrp.
 * The gbi side keeps the typed units. Its RDP sync units (ND_NOP_A) are dropped, except the two RDPHALF units after a texture rectangle. */
static Gfx pk[16];
static Gfx* p;
static uint32_t bb[64];
static uint32_t* nrp;
static int a_is_nr; /* the shape ho: the A side is an O form too (ND_NR_AT into bb) */

/* the gbi side (A) and the O side (B) of each shape: fixed nonzero values */
static void a_rm(void) { gDPSetRenderMode(p++, G_RM_OPA_SURF, G_RM_OPA_SURF2); }
static void b_rm(void) { NDST_RM(ND_NR_AT, G_RM_OPA_SURF, G_RM_OPA_SURF2); }
static void a_lut(void) { gDPSetTextureLUT(p++, G_TT_RGBA16); }
static void b_lut(void) { NDST_LUT(ND_NR_AT, G_TT_RGBA16); }
static void a_filt(void) { gDPSetTextureFilter(p++, G_TF_BILERP); }
static void b_filt(void) { NDST_FILT(ND_NR_AT, G_TF_BILERP); }
static void a_dither(void) { gDPSetColorDither(p++, G_CD_BAYER); }
static void b_dither(void) { NDST_DITHER(ND_NR_AT, G_CD_BAYER); }
static void a_cc(void) { gDPSetCombineMode(p++, G_CC_DECALRGBA, G_CC_DECALRGBA); }
static void b_cc(void) { NDST_CC(ND_NR_AT, G_CC_DECALRGBA, G_CC_DECALRGBA); }
static void a_comb(void) { ND_GX(p++, 0xfc123456u, 0x7890abcdu); }
static void b_comb(void) { NDST_COMB(ND_NR_AT, 0xfc123456u, 0x7890abcdu); }
static void a_prim(void) { gDPSetPrimColor(p++, 3, 4, 10, 20, 30, 40); }
static void b_prim(void) { NDVAL_PRIM(ND_NR_AT, 3, 4, 10, 20, 30, 40); }
static void a_env(void) { gDPSetEnvColor(p++, 10, 20, 30, 40); }
static void b_env(void) { NDVAL_ENV(ND_NR_AT, 10, 20, 30, 40); }
static Mtx gMtxAt;
static void a_push(void) { gSPMatrix(p++, &gMtxAt, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_LOAD); }
static void b_push(void) { NDXF_PUSH(ND_NR_AT, &gMtxAt, G_MTX_MODELVIEW | G_MTX_PUSH | G_MTX_LOAD); }
static void a_pop(void) { gSPPopMatrix(p++, G_MTX_MODELVIEW); }
static void b_pop(void) { NDXF_POP(ND_NR_AT); }
static Vtx gVtxAt[8];
static void a_vtx(void) { gSPVertex(p++, gVtxAt, 4, 2); }
static void b_vtx(void) { NDGEO_VTX(ND_NR_AT, gVtxAt, 4, 2); }
static void a_tri1(void) { gSP1Triangle(p++, 1, 2, 3, 0); }
static void b_tri1(void) { NDGEO_TRI1(ND_NR_AT, 1, 2, 3, 0); }
static void a_tri2(void) { gSP2Triangles(p++, 1, 2, 3, 0, 4, 5, 6, 0); }
static void b_tri2(void) { NDGEO_TRI2(ND_NR_AT, 1, 2, 3, 0, 4, 5, 6, 0); }
static Vp gVpAt;
static void a_viewport(void) { gSPViewport(p++, &gVpAt); }
static void b_viewport(void) { NDTX_VIEWPORT(ND_NR_AT, &gVpAt); }
static void a_fill(void) { gDPFillRectangle(p++, 0, 0, 319, 239); }
static void b_fill(void) { NDTX_FILL(ND_NR_AT, 0, 0, 319, 239); }
static void a_ltile(void) { gDPLoadTextureTile(p++, gTimg, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, 0, 31, 31, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void b_ltile(void) { NDTX_LOAD_TILE(ND_NR_AT, gTimg, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, 0, 31, 31, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void a_ltile4(void) { gDPLoadTextureTile_4b(p++, gTimg, G_IM_FMT_CI, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void b_ltile4(void) { NDTX_LOAD_TILE_4B(ND_NR_AT, gTimg, G_IM_FMT_CI, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void a_lblk(void) { gDPLoadTextureBlock(p++, gTimg, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void b_lblk(void) { NDTX_LOAD_BLOCK(ND_NR_AT, gTimg, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void a_lblk4(void) { gDPLoadTextureBlock_4b(p++, gTimg, G_IM_FMT_CI, 32, 32, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void b_lblk4(void) { NDTX_LOAD_BLOCK_4B(ND_NR_AT, gTimg, G_IM_FMT_CI, 32, 32, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD); }
static void a_tlut16(void) { gDPLoadTLUT_pal16(p++, 3, gTimg); }
static void b_tlut16(void) { NDTX_LOAD_TLUT16(ND_NR_AT, 3, gTimg); }
static void a_sblk4(void) { gDPScrollTextureBlock_4b(p++, gTimg, G_IM_FMT_CI, 32, 32, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD, 2, 3); }
static void b_sblk4(void) { NDTX_SCROLL_BLOCK_4B(ND_NR_AT, gTimg, G_IM_FMT_CI, 32, 32, 1, G_TX_WRAP, G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD, 2, 3); }
static void a_rtex(void) { gSPTextureRectangle(p++, 10, 20, 300, 200, 0, 0x8000, 0x8000, 0x400, 0x400); }
static void b_rtex(void) { NDTX_RECT_TEX(ND_NR_AT, 10, 20, 300, 200, 0, 0x8000, 0x8000, 0x400, 0x400); }
static void a_rscis(void) { gSPScisTextureRectangle(p++, 10, 20, 300, 200, 0, 0x8000, 0x8000, 0x400, 0x400); }
static void b_rscis(void) { NDTX_RECT_SCIS(ND_NR_AT, 10, 20, 300, 200, 0, 0x8000, 0x8000, 0x400, 0x400); }
static uint32_t gRw0, gRw1, gRhs, gRhd; /* set in main: the texture rectangle words of screen_overlays.c */
static void a_rw(void) { ND_GX(p++, gRw0, gRw1); gImmp1(p++, G_RDPHALF_1, gRhs); gImmp1(p++, G_RDPHALF_2, gRhd); }
static void b_rw(void) { NDTX_RECT_W(ND_NR_AT, gRw0, gRw1, gRhs, gRhd); }
static void a_ndsp(void) { gPmNdSplash(p++, 1); }
static void b_ndsp(void) { NDCTL_ND(ND_NR_AT, G_PM_ND_SPLASH, 1); }
static void a_ndgut(void) { gPmNdGutter(p++, 0x1a7); }
static void b_ndgut(void) { NDCTL_ND(ND_NR_AT, G_PM_ND_GUTTER, 0x1a7); }
static void a_ho(void) { nrp = bb; ND_NR_AT(0x12345678u, 0x9abcdef0u); a_is_nr = 1; }
static void b_ho(void) { uint32_t* q = nrp; nd_nr_o(&q, 0x12345678u, 0x9abcdef0u); nrp = q; }

typedef struct { const char* name; void (*a)(void); void (*b)(void); } Shape;
static const Shape shapes[] = {
    { "rm", a_rm, b_rm }, { "lut", a_lut, b_lut }, { "filt", a_filt, b_filt }, { "dither", a_dither, b_dither },
    { "cc", a_cc, b_cc }, { "comb", a_comb, b_comb }, { "prim", a_prim, b_prim }, { "env", a_env, b_env },
    { "push", a_push, b_push }, { "pop", a_pop, b_pop }, { "vtx", a_vtx, b_vtx }, { "tri1", a_tri1, b_tri1 },
    { "tri2", a_tri2, b_tri2 }, { "viewport", a_viewport, b_viewport }, { "fill", a_fill, b_fill },
    { "ltile", a_ltile, b_ltile }, { "ltile4", a_ltile4, b_ltile4 }, { "lblk", a_lblk, b_lblk }, { "lblk4", a_lblk4, b_lblk4 },
    { "tlut16", a_tlut16, b_tlut16 }, { "sblk4", a_sblk4, b_sblk4 }, { "rtex", a_rtex, b_rtex }, { "rscis", a_rscis, b_rscis },
    { "rw", a_rw, b_rw }, { "ho", a_ho, b_ho }, { "ndsp", a_ndsp, b_ndsp }, { "ndgut", a_ndgut, b_ndgut },
};

/* one shape: the gbi side units (the sync units dropped, the two halves after a texture rectangle kept) against the O side units */
static int run_shape(const Shape* s) {
    static uint32_t ua[64], ub[64];
    unsigned na = 0, nb = 0, keep = 0;
    memset(pk, 0, sizeof pk);
    memset(bb, 0, sizeof bb);
    p = pk;
    nrp = bb;
    a_is_nr = 0;
    s->a();
    if (a_is_nr) {
        na = (unsigned)(nrp - bb) / 2u;
        memcpy(ua, bb, na * 2u * sizeof(uint32_t));
        memset(bb, 0, sizeof bb);
        nrp = bb;
    } else {
        for (Gfx* g = pk; g < p; g++) {
            const uint32_t w0 = g->words.w0, w1 = g->words.w1;
            if (keep) keep--;
            else if (w0 == ND_NOP_A) continue;
            else if ((w0 >> 24) == ND_FS_RECT && (w1 & ND_FS_RECT_TEX)) keep = 2u;
            ua[2u * na] = w0;
            ua[2u * na + 1u] = w1;
            na++;
        }
    }
    s->b();
    nb = (unsigned)(nrp - bb) / 2u;
    memcpy(ub, bb, nb * 2u * sizeof(uint32_t));
    return na == nb && memcmp(ua, ub, na * 2u * sizeof(uint32_t)) == 0;
}

int main(void) {
    static Gfx dummy[4];
    unsigned n = (unsigned)(sizeof shapes / sizeof shapes[0]), ok = 0;
    gMainGfxPos = dummy + 1;
    gRw0 = ND_W0_TR(G_TEXRECT, 300, 200);
    gRw1 = ND_W1_TR(0, 10, 20);
    gRhs = ND_TR_H1(0x8000, 0x8000);
    gRhd = ND_TR_H2(0x400, 0x400);
    for (unsigned i = 0; i < n; i++) {
        if (!run_shape(&shapes[i])) { printf("ns3unit FAIL %s\n", shapes[i].name); return 1; }
        ok++;
    }
    printf("ns3unit ok %u/%u\n", ok, n);
    return 0;
}
