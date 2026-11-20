/* nd_geo_test.c: host test of the target, depth, vertex, triangle and light calls (nd_geo.h) against the macros they replace (GEOUNIT).
 * Each shape records the macro form under the redirect of nd_gbi.h (A) and the setter form (B) in a custom gfx range. The two must agree on
 * (1) the stream words from the mark to the NDC_END word, (2) the unit log of nd_stream_run(mark), (3) the nd_r11_state result (return, cw,
 * has_cb, nmo, mo). Prints "geounit ok 14/14", or "geounit FAIL <name>" for the first shape that differs, and exits 1. The depth image
 * shape (zimg) has no setter: its macro records a NOP unit, so both forms are empty. */
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
#include "nd_geo.h"
#include "nd_ds.h" /* NAT4A: the test state of run_one */

/* the redirect target of the macro form: gMainGfxPos is the top-level packet pointer (nd_gbi.h D9) */
Gfx* gMainGfxPos;
s32 gCurrentDisplayContextIndex;
const NdNDev* const nd_rm_dev[0x3d]; /* nd_rmtab.c: the render mode lists, all NULL here */
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
/* NTGT: the runner of nd_target_begin and nd_target_end (nd_walk.c); the double logs its payload */
static uint32_t tlog[8]; static unsigned ntlog;
void nd_tgt_run(const uint32_t* rec) { if (ntlog < 4u) { tlog[2u * ntlog] = rec[0]; tlog[2u * ntlog + 1u] = rec[1]; ntlog++; } }
static uint32_t ulog[512]; static unsigned nulog;
void nd_nr_run(const uint32_t* q) { const unsigned n = (q[-2] >> 16) & 0xffu;
    for (unsigned i = 0; i < n && nulog < 256u; i++) { ulog[2u * nulog] = q[2u * i]; ulog[2u * nulog + 1u] = q[2u * i + 1u]; nulog++; }
}
#include "nd_rec_log.h" /* NAT6G1 */

/* nd_model.c defines nd_r11_state (MDL-5f); nd_recs.c reads it */
int nd_r11_state(uint32_t addr, const uint32_t** nr, uint32_t* cw, int* has_cb, uint32_t* mo, unsigned* nmo);

/* fixtures: the call sites' arguments (sprite_shading.c:389, main.c:153, background_gfx.c:340-341, msg_draw.c:2004-2005, hud_element.c:1905,
 * item_entity.c:210, imgfx.c:1009, lil_oink.c:184, draw_box.c:497, windows.c:376) */
static unsigned char gpal[64];
static unsigned char gzbuf[64];
static Vtx gvtx[4];
static Light glk[2];
static struct { Light l[1]; Light a; } gl1;
static struct { Light l[2]; Light a; } gl2;

/* the macro form (A) and the setter form (B) of each shape */
static void n_begin(void) { nd_target_begin(gpal, G_IM_SIZ_16b, 320); }
static void n_end(void) { nd_target_end(); }
unsigned short* nuGfxCfb_ptr = (unsigned short*)(void*)gpal;
static void m_zimg(void) { gDPSetDepthImage(gMainGfxPos++, gzbuf); }
static void n_zimg(void) { }
static void m_zsrc(void) { gDPSetDepthSource(gMainGfxPos++, G_ZS_PRIM); }
static void n_zsrc(void) { nd_pso_zsrc(G_ZS_PRIM); }
static void m_primz(void) { gDPSetPrimDepth(gMainGfxPos++, 0x1234, 0x5678); }
static void n_primz(void) { nd_val_primz(0x1234, 0x5678); }
static void m_vtx(void) { gSPVertex(gMainGfxPos++, gvtx, 4, 0); }
static void n_vtx(void) { nd_mesh_vtx(gvtx, 4, 0); }
static void m_tri2(void) { gSP2Triangles(gMainGfxPos++, 0, 2, 1, 0, 1, 2, 3, 0); }
static void n_tri2(void) { nd_mesh_tri2(0, 2, 1, 0, 1, 2, 3, 0); }
static void m_tri1(void) { gSP1Triangle(gMainGfxPos++, 0, 1, 2, 1); }
static void n_tri1(void) { nd_mesh_tri1(0, 1, 2, 1); }
static void m_lit1(void) { gSPSetLights1(gMainGfxPos++, gl1); }
static void n_lit1(void) { nd_lit_set1(gl1); }
static void m_lit2(void) { gSPSetLights2(gMainGfxPos++, gl2); }
static void n_lit2(void) { nd_lit_set2(gl2); }
static void m_lookat(void) { gSPLookAt(gMainGfxPos++, glk); }
static void n_lookat(void) { nd_lit_lookat(glk); }
static void m_lookatxy(void) { gSPLookAtX(gMainGfxPos++, &glk[0]); gSPLookAtY(gMainGfxPos++, &glk[1]); }
static void n_lookatxy(void) { nd_lit_lookat(glk); }
static void m_pnorm(void) { gSPPerspNormalize(gMainGfxPos++, 0x0c00); }
static void n_pnorm(void) { nd_xf_pnorm(0x0c00); }
static void m_clip(void) { gSPClipRatio(gMainGfxPos++, FRUSTRATIO_3); }
static void n_clip(void) { nd_xf_clip(FRUSTRATIO_3); }

/* one run of a shape: the words of the range, its unit log, and the r11 result */
typedef struct {
    uint32_t w[256]; unsigned nw;
    uint32_t u[512]; unsigned nu;
    int r; uint32_t cw[2]; int hcb; uint32_t mo[8]; unsigned nmo;
    NdDs ds; /* NAT4K: core 1 state after the shape (a dropped unit still changes it) */
} Res;

static void run_one(void (*fn)(void), Res* out) {
    memset(out, 0, sizeof *out);
    { static NdDs d0; static int have; if (!have) { d0 = nd_ds; have = 1; } nd_ds = d0; } /* NAT4A: each run starts from the same core 1 state (the ep mark reads the change bit) */
    gMainGfxPos = nd_rec_task();
    const uint32_t* mk = nd_rec_mark();
    fn();
    nd_rec_end();
    memcpy(&out->ds, &nd_ds, sizeof nd_ds); /* NAT4K */
    const uint32_t* en = nd_rec_pos();
    out->nw = (unsigned)(en - mk);
    if (out->nw > 256u) out->nw = 256u;
    memcpy(out->w, mk, out->nw * sizeof(uint32_t));
    nulog = 0;
    nd_stream_run(mk);
    out->nu = nulog * 2u;
    memcpy(out->u, ulog, out->nu * sizeof(uint32_t));
    const uint32_t* nr = 0;
    out->r = nd_r11_state((uint32_t)(uintptr_t)mk, &nr, out->cw, &out->hcb, out->mo, &out->nmo);
}

static int same(const Res* a, const Res* b) {
    return a->nw == b->nw && memcmp(a->w, b->w, a->nw * sizeof(uint32_t)) == 0
        && a->nu == b->nu && memcmp(a->u, b->u, a->nu * sizeof(uint32_t)) == 0
        && a->r == b->r && memcmp(a->cw, b->cw, sizeof a->cw) == 0 && a->hcb == b->hcb
        && a->nmo == b->nmo && memcmp(a->mo, b->mo, sizeof a->mo) == 0 && memcmp(&a->ds, &b->ds, sizeof a->ds) == 0; /* NAT4K */
}

typedef struct { const char* name; void (*mac)(void); void (*nat)(void); } Shape;
static const Shape shapes[] = {
    { "zimg", m_zimg, n_zimg }, { "zsrc", m_zsrc, n_zsrc }, { "primz", m_primz, n_primz },
    { "vtx", m_vtx, n_vtx }, { "tri2", m_tri2, n_tri2 }, { "tri1", m_tri1, n_tri1 }, { "lit1", m_lit1, n_lit1 },
    { "lit2", m_lit2, n_lit2 }, { "lookat", m_lookat, n_lookat }, { "lookatxy", m_lookatxy, n_lookatxy }, { "pnorm", m_pnorm, n_pnorm }, { "clip", m_clip, n_clip },
};

int main(void) {
    static Res a, b;
    unsigned n = (unsigned)(sizeof shapes / sizeof shapes[0]), ok = 0;
    for (unsigned i = 0; i < n; i++) {
        run_one(shapes[i].mac, &a);
        run_one(shapes[i].nat, &b);
        if (!same(&a, &b)) { printf("geounit FAIL %s\n", shapes[i].name); return 1; }
        ok++;
    }
    /* NTGT: nd_target_begin and nd_target_end each record one NDC_NAPI record: header (1 unit), the runner nd_tgt_run, the payload (siz << 12 | width - 1, address) */
    static const struct { const char* name; void (*fn)(void); } tg[2] = { { "tbegin", n_begin }, { "tend", n_end } };
    for (unsigned i = 0; i < 2u; i++) {
        const uint32_t want = i == 0 ? (uint32_t)(uintptr_t)gpal : (uint32_t)(uintptr_t)nuGfxCfb_ptr;
        ntlog = 0;
        run_one(tg[i].fn, &b);
        const uint32_t pw[4] = { NDC_HDR(NDC_NAPI, 1, 0), (uint32_t)(uintptr_t)nd_tgt_run, (2u << 12) | 319u, want };
        if (b.nw < 4u || memcmp(b.w, pw, sizeof pw) != 0 || ntlog != 1u || tlog[0] != pw[2] || tlog[1] != pw[3]) { printf("geounit FAIL %s\n", tg[i].name); return 1; }
        ok++; n++;
    }
    printf("geounit ok %u/%u\n", ok, n);
    return 0;
}
