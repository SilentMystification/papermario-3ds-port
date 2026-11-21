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

/* L1Q1: the setter unit queue (nd_rec.c, nd_rec_q, nd_uq_sync) against the immediate form nd_rec_nr. Each shape runs once with the
 * queue form and once with the immediate form; the stream words, the unit log, the core 1 state and the stream position must agree.
 * Prints "uqunit ok 3/3", or "uqunit FAIL <shape>" for the first shape that differs (exit 1). The queue must hold no hole (nd_uq_holes 0). */
#include "nd_fs.h"
#include "nd_nr.h"

extern unsigned nd_uq_holes; /* nd_rec.c: units that reached the recorder past a queue (a hole) */
static const uint32_t* g_mk; /* the stream mark of the run in progress */
static unsigned g_pos;       /* the stream position of the pos shape, from g_mk */

typedef struct {
    uint32_t w[4096]; unsigned nw;
    uint32_t u[1024]; unsigned nu;
    NdDs ds; unsigned pos;
} UqRes;

static void run_uq(void (*fn)(void), UqRes* out) {
    memset(out, 0, sizeof *out);
    { static NdDs d0; static int have; if (!have) { d0 = nd_ds; have = 1; } nd_ds = d0; }
    gMainGfxPos = nd_rec_task();
    g_mk = nd_rec_mark();
    g_pos = 0;
    fn();
    nd_rec_end();
    memcpy(&out->ds, &nd_ds, sizeof nd_ds);
    const uint32_t* en = nd_rec_pos();
    out->nw = (unsigned)(en - g_mk);
    if (out->nw > 4096u) out->nw = 4096u;
    memcpy(out->w, g_mk, out->nw * sizeof(uint32_t));
    nulog = 0;
    nd_stream_run(g_mk);
    out->nu = nulog * 2u;
    memcpy(out->u, ulog, out->nu * sizeof(uint32_t));
    out->pos = g_pos;
}

static int uq_same(const UqRes* a, const UqRes* b) {
    return a->nw == b->nw && memcmp(a->w, b->w, a->nw * sizeof(uint32_t)) == 0
        && a->nu == b->nu && memcmp(a->u, b->u, a->nu * sizeof(uint32_t)) == 0
        && a->pos == b->pos && memcmp(&a->ds, &b->ds, sizeof a->ds) == 0;
}

static void t_run(const uint32_t* q) { (void)q; } /* the no-op runner of the napi shape */
static const uint32_t r12[2] = { 1u, 2u };

/* q300: 300 setter units; napi: 5 setter units then one napi record; pos: 5 setter units, then the stream position from the mark */
static void sq_q(void) { for (unsigned k = 0; k < 300u; k++) nd_nr(ND_FS_W0(ND_FS_PRIM, 0u), k * 0x01010101u); }
static void sq_n(void) { for (unsigned k = 0; k < 300u; k++) nd_rec_nr(ND_FS_W0(ND_FS_PRIM, 0u), k * 0x01010101u); }
static void sn_q(void) { for (unsigned k = 0; k < 5u; k++) nd_nr(ND_FS_W0(ND_FS_PRIM, 0u), k * 0x01010101u); nd_rec_napi(t_run, r12, 1u); }
static void sn_n(void) { for (unsigned k = 0; k < 5u; k++) nd_rec_nr(ND_FS_W0(ND_FS_PRIM, 0u), k * 0x01010101u); nd_rec_napi(t_run, r12, 1u); }
static void sp_q(void) { for (unsigned k = 0; k < 5u; k++) nd_nr(ND_FS_W0(ND_FS_PRIM, 0u), k * 0x01010101u); g_pos = (unsigned)(nd_rec_pos() - g_mk); }
static void sp_n(void) { for (unsigned k = 0; k < 5u; k++) nd_rec_nr(ND_FS_W0(ND_FS_PRIM, 0u), k * 0x01010101u); g_pos = (unsigned)(nd_rec_pos() - g_mk); }

int main(void) {
    static UqRes a, b;
    static const struct { const char* name; void (*q)(void); void (*n)(void); } sh[3] = {
        { "q300", sq_q, sq_n }, { "napi", sn_q, sn_n }, { "pos", sp_q, sp_n } };
    unsigned ok = 0;
    for (unsigned i = 0; i < 3u; i++) {
        run_uq(sh[i].q, &a);
        run_uq(sh[i].n, &b);
        if (!uq_same(&a, &b)) { printf("uqunit FAIL %s\n", sh[i].name); return 1; }
        ok++;
    }
    if (nd_uq_holes) { printf("uqunit FAIL holes %u\n", nd_uq_holes); return 1; }
    printf("uqunit ok %u/3\n", ok);
    return 0;
}
