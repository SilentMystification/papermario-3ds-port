/* nd_direct_test.c: host test of the direct-draw helpers (nd_direct.c): the pso decision cache and the state a bind leaves. The walker functions it
 * calls are stubs that count their calls; nd_direct.c is included so the test reads its decision entries. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_int.h"
#include "nd_api.h"
#include "pm_gpu.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "../src/nd_direct.c"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL nd_direct line %d\n", __LINE__); fails++; } } while (0)

void pm_log_e(const char* fmt, ...) { (void)fmt; }
void pm_log_d(const char* fmt, ...) { (void)fmt; }
unsigned pm_frame_count(void) { return 0; }
void pm_fatal_stop(const char* msg) { printf("FAIL nd_direct fatal %s\n", msg); exit(1); }

NdWs nd_ws;
int dual_on, box_prod, box_pat, box_folded, comb_set, comb_alpha_shade, plan_active, leg_mode, comb_mode5, sc_ok, cimg_zero, rm_frame;
unsigned keyc, keys, conv_k4, gv_epoch;
int ls_use, ls_why, ls_cyc, ls_dual, ls_frame;
int pm_comb_vprim;
PmPlan cur_plan;
M1Rec m1;
struct TszLast tsz_last;

static int n_decide, n_setcomb, n_prim, n_plan, n_leg, n_const, n_rect, n_render, n_forget, n_flush, n_filter, n_end_clear;
static unsigned last_prim, last_const, last_leg;
static int last_rect[4];
static float last_st[4];
static int fake_role, fake_aux = -1;

void comb_in_walker(NdCombIn* in) { memset(in, 0, sizeof *in); in->w0 = nd_ws.comb_w0; in->w1 = nd_ws.comb_w1; in->omode_h = nd_ws.omode_h; }
void nd_comb_decide(const NdCombIn* in, NdCombOut* out) {
    n_decide++;
    memset(out, 0, sizeof *out);
    out->cyc = (int)((in->omode_h >> G_MDSFT_CYCLETYPE) & 3u);
    if (in->w0 == 0xfc000001u) { out->plan_on = 1; out->plan.uses_tex1 = 1; out->leg = 8; return; } /* reads tile 1: not statable */
    if (in->w0 == 0xfc000002u) { out->plan_on = 1; out->leg = 8; return; }
    if (in->w0 == 0xfc000003u) { out->cc_on = 1; out->cc = 0x11223344u; out->leg = 4; return; }
    out->leg = 2;
}
void op_setcombine(u32 w0, u32 w1) { n_setcomb++; nd_ws.comb_w0 = w0; nd_ws.comb_w1 = w1; comb_set = 1; }
void sync_render(void) { n_render++; }
void nd_sr_forget(void) { n_forget++; }
void ens_clear(void) { n_end_clear++; }
int img_role(int for_fill) { (void)for_fill; return fake_role; }
int pm_is_cfb(const void* p) { return p != NULL; }
int pm_tex_aux_off_idx(void) { return fake_aux; }
void pm_tex_set_filter(int linear) { (void)linear; n_filter++; }
void pm_gpu_flush_at(int line) { (void)line; n_flush++; }
void pm_gpu_set_prim(unsigned rgba, unsigned lod) { (void)lod; n_prim++; last_prim = rgba; }
void pm_gpu_set_const(unsigned rgba) { n_const++; last_const = rgba; }
void pm_gpu_set_env(unsigned rgba) { (void)rgba; }
void pm_comb_colours(PmPlan* p, unsigned prim, unsigned env, unsigned plod, unsigned key, unsigned ks) { (void)p; (void)prim; (void)env; (void)plod; (void)key; (void)ks; }
void pm_gpu_plan_key(unsigned w0, unsigned w1, int two, int t1only) { (void)w0; (void)w1; (void)two; (void)t1only; }
void pm_gpu_set_plan(const PmPlan* p) { (void)p; n_plan++; }
void pm_gpu_set_combine(int mode) { n_leg++; last_leg = (unsigned)mode; }
int const_rgb_tex_alpha(unsigned* out) { *out = 0xaabbccddu; return 1; }
int nd_tex_bind_desc(const NdBindDesc* d, int n) { (void)d; (void)n; return 0; }
void gp3_t0_surv(void) {}
void pm_tex_size(int* w, int* h) { *w = 32; *h = 16; }
void pm_gpu_tex_rect(int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1) {
    n_rect++;
    last_rect[0] = x0; last_rect[1] = y0; last_rect[2] = x1; last_rect[3] = y1;
    last_st[0] = s0; last_st[1] = t0; last_st[2] = s1; last_st[3] = t1;
}

int main(void) {
    NdPsoN64 a = { 0xfc000010u, 0x11111111u, 0x00504340u, 0x0u, 0xf000u };
    NdPsoN64 b = { 0xfc000011u, 0x22222222u, 0x00552048u, 0x0u, 0xf000u };
    NdPsoN64 t1 = { 0xfc000001u, 0u, 0x00504340u, 0u, 0u };
    NdPsoN64 pl = { 0xfc000002u, 0u, 0x00504340u, 0u, 0u };
    NdPsoN64 cc = { 0xfc000003u, 0u, 0x00504340u, 0u, 0u };
    NdPso ia, ib, it1, ipl, icc;
    const NdDec *da, *da2, *db;
    memset(&nd_ws, 0, sizeof nd_ws);
    nd_ws.omode_l = 0x5u;
    comb_set = 1;
    ia = nd_pso_n64(&a); ib = nd_pso_n64(&b);
    CHECK(ia != ib);
    /* the pso cache: the same id gives the same decision and runs nd_comb_decide once; two descriptions are two entries */
    da = nd_pso_decide(ia);
    da2 = nd_pso_decide(ia);
    CHECK(da && da == da2);
    CHECK(n_decide == 1);
    db = nd_pso_decide(ib);
    CHECK(db && db != da);
    CHECK(n_decide == 2);
    CHECK(nd_pso_decide(ia) == da && nd_pso_decide(ib) == db);
    CHECK(n_decide == 2);
    /* other inputs (the cycle type the pso runs under) make the decision again */
    nd_ws.omode_h = 1u << G_MDSFT_CYCLETYPE;
    CHECK(nd_pso_decide(ia) == da && n_decide == 3);
    CHECK(da->out.cyc == 1);
    nd_ws.omode_h = 0;
    /* a decision that reads tile 1 is not statable */
    it1 = nd_pso_n64(&t1);
    CHECK(nd_pso_decide(it1) == NULL);
    n_prim = n_setcomb = 0;
    CHECK(nd_direct_bind(it1, 0x01020304u, 0u) == 0);
    /* a bind: the other mode as the stream sets it (the render mode replaces bits 3 to 31, bits 0 to 2 stay), the combine words, the primitive colour */
    nd_ws.omode_h = 0; n_filter = 0;
    CHECK(nd_direct_bind(ia, 0xff804020u, 0x55u) == 1);
    CHECK(nd_ws.omode_l == (0x5u | 0x00504340u));
    CHECK(nd_ws.comb_w0 == 0xfc000010u && nd_ws.comb_w1 == 0x11111111u);
    CHECK(nd_ws.prim == 0xff804020u && last_prim == 0xff804020u && nd_ws.prim_lod == 0u && nd_ws.envc == 0x55u);
    CHECK(n_leg == 1 && last_leg == 2u && leg_mode == 2 && n_plan == 0 && n_render >= 1);
    CHECK(dual_on == 0);
    n_setcomb = 0;
    CHECK(nd_direct_bind(ia, 0xff804020u, 0x55u) == 1);
    CHECK(n_setcomb == 0); /* the same words: no second op_setcombine */
    ipl = nd_pso_n64(&pl);
    CHECK(nd_direct_bind(ipl, 1u, 2u) == 1 && n_plan == 1 && plan_active == 1);
    icc = nd_pso_n64(&cc);
    CHECK(nd_direct_bind(icc, 3u, 4u) == 1 && n_const == 1 && last_const == 0xaabbccddu);
    /* the filter bits (12 to 13) ask pm_tex_set_filter when they change */
    {
        NdPsoN64 f = a;
        NdPso i;
        f.omh = 3u << 12; f.omh_m = 3u << 12; f.cw0 = 0xfc000020u;
        i = nd_pso_n64(&f);
        n_filter = 0;
        CHECK(nd_direct_bind(i, 0u, 0u) == 1 && n_filter == 1 && (nd_ws.omode_h & (3u << 12)) == (3u << 12));
    }
    /* COPY cycle and a G_ZS_PRIM rectangle are not stated */
    nd_ws.omode_h = 2u << G_MDSFT_CYCLETYPE;
    CHECK(nd_direct_bind(ia, 0u, 0u) == 0);
    nd_ws.omode_h = 0;
    /* ready: the main colour image only */
    fake_role = ROLE_MAIN; nd_ws.cimg = NULL;
    CHECK(nd_direct_ready() == 1);
    fake_role = ROLE_SLOT;
    CHECK(nd_direct_ready() < 1);
    fake_role = ROLE_MAIN; m1.valid = 1;
    CHECK(nd_direct_ready() < 1);
    m1.valid = 0; fake_aux = 3;
    CHECK(nd_direct_ready() < 1);
    fake_aux = -1;
    /* a rectangle: upper left (10, 20), lower right (42, 36), s 0, t 0, 1:1 on a 32 x 16 texture (inclusive lower right pixel is exclusive here) */
    nd_ws.tile_x0 = 0; nd_ws.tile_y0 = 0; nd_ws.tile_fx = nd_ws.tile_fy = 0.f;
    nd_direct_rect((42u * 4u) << 12 | (36u * 4u), (10u * 4u) << 12 | (20u * 4u), 0u, (1024u << 16) | 1024u);
    CHECK(n_rect == 1 && last_rect[0] == 10 && last_rect[1] == 20 && last_rect[2] == 41 && last_rect[3] == 35);
    CHECK(last_st[0] == 0.f && last_st[1] == 0.f && last_st[2] == 1.f && last_st[3] == 1.f);
    n_forget = 0; n_end_clear = 0; sc_ok = 1; gv_epoch = 7;
    nd_direct_end();
    CHECK(n_end_clear == 1 && n_forget == 1 && sc_ok == 0 && nd_ws.cur_tile == -1 && gv_epoch == 8);
    if (fails) return 1;
    printf("nd_direct ok\n");
    return 0;
}
