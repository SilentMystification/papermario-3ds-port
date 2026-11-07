/* nd_gv_test.c: host test of nd_gv_params (nd_model.c, S3N-1). The oracle is the walker's gv_params_calc as it was before the move (pm_gbi.c at 5fc6fb083),
 * written here with the same expressions over the same values. Random inputs; every float of the output must be bit equal. Exit 1 on a difference. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "nd.h"
int pm_debug_has(const char* name) { (void)name; return 0; } /* the switches are off in the host test (nd_model.c PM5) */

/* nd_mtx.c links the interpolation hooks; the gv test does not use them */
int pm_interp_repl;
uint32_t f32tof24(float f) { (void)f; return 0; } /* nd_tmpl.c (nd_model.c calls nd_render_state): libctru, not used here */
const float (*pm_interp_find(const void* m))[4] { (void)m; return 0; }

static uint32_t rs = 0x9e3779b9u;
static uint32_t rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static float rf(void) { return (float)(int)(rnd() % 2001u) / 1000.f - 1.f; }

/* the old walker function, expression for expression (the flags come in as the walker's own values) */
static float old_shift_mul(unsigned n) { return n == 0 ? 1.f : n <= 10 ? 1.f / (float)(1 << n) : (float)(1 << (16 - n)); }
static float old_prim_ndc_z(unsigned pz) { return 2.f * (float)(pz ? pz - 1u : 0u) / 32767.f - 1.f; }
static void old_gv(const NdGvIn* g, const float en[4], const float vc[4], NdGvP* o_) {
    int comb_mode5 = g->comb5, plan_active = g->plan_active, comb_alpha_shade = g->alpha_shade, dual_boost = g->boost;
    int env = comb_mode5 && !plan_active;
    int plan = plan_active && g->plan_vtx_ok;
    int a255 = !plan_active && !plan && !comb_alpha_shade;
    int boost = dual_boost;
    int primz = g->primz;
    int tw = g->tw, th = g->th;
    if (tw < 1) tw = 1;
    if (th < 1) th = 1;
    float ms = old_shift_mul(g->shs), mt = old_shift_mul(g->sht);
    NdGvP* o2 = o_;
    o2->vp[0] = g->vp[0]; o2->vp[1] = g->vp[1]; o2->vp[2] = g->vp[2]; o2->vp[3] = g->vp[3];
    for (int q = 0; q < 4; q++) { o2->cm[q] = 1.f; o2->ca[q] = 0.f; }
    o2->zm[0] = 1.f; o2->zm[1] = 0.f;
    if (g->tex_on) {
        float itw = 1.f / (float)tw, ith = 1.f / (float)th;
        o2->uvso[0] = (float)(g->tss >> 16) / 65536.f / 32.f * ms * itw;
        o2->uvso[1] = (float)(g->tss & 0xffffu) / 65536.f / 32.f * mt * ith;
        o2->uvso[2] = -((float)g->tile_x0 + g->tile_fx) * itw;
        o2->uvso[3] = -((float)g->tile_y0 + g->tile_fy) * ith;
    } else {
        o2->uvso[0] = o2->uvso[1] = 0.f;
        o2->uvso[2] = -((float)g->tile_x0 + g->tile_fx) / (float)tw + 0.5f / (float)tw;
        o2->uvso[3] = -((float)g->tile_y0 + g->tile_fy) / (float)th + 0.5f / (float)th;
    }
    if (env) { for (int q = 0; q < 4; q++) { o2->cm[q] = 0.f; o2->ca[q] = en[q]; } }
    else if (plan) { for (int q = 0; q < 4; q++) { o2->cm[q] = 0.f; o2->ca[q] = vc[q]; } }
    if (a255) { o2->cm[3] = 0.f; o2->ca[3] = 1.f; }
    if (boost) for (int q = 0; q < 3; q++) { o2->ca[q] = o2->ca[q] * 0.5f + (1.f - en[q]) * 0.5f; o2->cm[q] *= 0.5f; }
    if (primz) { o2->zm[0] = 0.f; o2->zm[1] = old_prim_ndc_z(g->prim_z); }
    o2->boost = boost;
    o2->fxon = 0;
    if (g->lighting) { nd_lights(&o2->fx, g->texgen != 0, g->lt, g->lt_n, g->lk); o2->fxon = 1; }
    if (g->fogmix && g->fog) {
        if (g->zc_ok) {
            if (!o2->fxon) memset(&o2->fx, 0, sizeof o2->fx);
            o2->fx.lflag[2] = 1.f;
            o2->fx.fogu[0] = (float)g->fog_fm / g->za;
            o2->fx.fogu[1] = (float)g->fog_fo - (float)g->fog_fm * g->zb / g->za;
            o2->cm[3] = 1.f; o2->ca[3] = 0.f;
            o2->fxon = 1;
        }
    }
}

static int diff_bytes(const NdGvP* a, const NdGvP* b) {
    if (memcmp(a->uvso, b->uvso, 16) || memcmp(a->vp, b->vp, 16) || memcmp(a->cm, b->cm, 16) || memcmp(a->ca, b->ca, 16) || memcmp(a->zm, b->zm, 8)) return 1;
    if (a->boost != b->boost || a->fxon != b->fxon) return 1;
    if (a->fxon && memcmp(&a->fx, &b->fx, sizeof a->fx)) return 1;
    return 0;
}

int main(void) {
    NdLight lt[9];
    int8_t lk[2][3];
    unsigned n = 200000, bad = 0;
    for (unsigned it = 0; it < n; it++) {
        NdGvIn g; memset(&g, 0, sizeof g);
        NdGvP a, b; memset(&a, 0, sizeof a); memset(&b, 0, sizeof b);
        float en[4], vc[4];
        for (int i = 0; i < 4; i++) { en[i] = (float)(rnd() & 255u) / 255.f; vc[i] = (float)(rnd() & 255u) / 255.f; }
        g.tss = rnd(); g.lighting = rnd() & 1; g.texgen = rnd() & 1; g.fog = rnd() & 1;
        g.comb5 = rnd() & 1; g.plan_active = (rnd() % 3) == 0; g.plan_vtx_ok = rnd() & 1; g.alpha_shade = rnd() & 1; g.boost = (rnd() % 4) == 0;
        g.primz = (rnd() % 3) == 0; g.prim_z = rnd() & 0x7fffu;
        g.tw = (int)(rnd() % 9u) - 1; g.th = (int)(rnd() % 9u) - 1;
        g.shs = rnd() % 13u; g.sht = rnd() % 13u;
        g.tex_on = rnd() & 1;
        for (int i = 0; i < 4; i++) g.vp[i] = rf() * 300.f;
        g.tile_x0 = (int)(rnd() % 512u) - 256; g.tile_y0 = (int)(rnd() % 512u) - 256;
        g.tile_fx = (float)(rnd() & 3u) * 0.25f; g.tile_fy = (float)(rnd() & 3u) * 0.25f;
        g.fogmix = rnd() & 1; g.zc_ok = rnd() & 1; g.za = rf() + 1.5f; g.zb = rf();
        g.fog_fm = (int)(rnd() % 2000u) - 1000; g.fog_fo = (int)(rnd() % 2000u) - 1000;
        for (int k = 0; k < 9; k++) { lt[k].col[0] = rnd() & 255; lt[k].col[1] = rnd() & 255; lt[k].col[2] = rnd() & 255; lt[k].dir[0] = (int8_t)rnd(); lt[k].dir[1] = (int8_t)rnd(); lt[k].dir[2] = (int8_t)rnd(); }
        for (int i = 0; i < 2; i++) for (int j = 0; j < 3; j++) lk[i][j] = (int8_t)rnd();
        g.lt = lt; g.lt_n = rnd() % 8u; g.lk = (const int8_t (*)[3])lk;
        old_gv(&g, en, vc, &a);
        /* the new function reads en and vc from its input struct */
        memcpy(g.en, en, sizeof en); memcpy(g.vc, vc, sizeof vc);
        nd_gv_params(&g, &b);
        if (diff_bytes(&a, &b)) { if (bad < 5) printf("nd_gv diff at case %u\n", it); bad++; }
    }
    /* the unpack and the shift helpers */
    for (unsigned c = 0; c < 17; c++) if (nd_shift_mul(c) != old_shift_mul(c)) { printf("shift diff %u\n", c); bad++; }
    printf("nd_gv: %u random cases, %u differ %s\n", n, bad, bad ? "FAIL" : "PASS");
    return bad != 0;
}
