/* nd_decide_test.c: NPSO2 host compare. For every state record of the ndref captures (combine words, other mode H and L, depth image flag) and a fixed grid of
 * the walker flags and colours, the old path (the walker NdCombIn, the combine decision of nd_comb_decide, nd_render_state, nd_tev_build) is run, and the
 * NdPsoDesc made from the same state goes through nd_pso_build, nd_pso_cc and nd_pso_colour. Every output must be equal byte for byte.
 * The NdCombIn of the old path is made here by its own copy of the walker rules (op_setcombine decode and rename, box_fold_sync): not by nd_comb_tst.
 * nd_comb_decide itself is in nd_comb.c (walker globals, not host linked): old_path is its code (nd_comb_core with the walker's vprim, then the plan colours).
 * Usage: nd_decide_test [--fault] <ndref.bin> ...   --fault: the desc tex_on is not copied (the compare must then report bad above 0). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"
#include "nd_api.h"

uint32_t f32tof24(float vf) { /* host copy of libctru f32tof24, as 3ds/tools/nd_cmd_test.c */
    union { float f; uint32_t i; } v;
    v.f = vf;
    uint32_t sign = v.i >> 31, mant = (v.i & 0x7FFFFFu) >> 7;
    int exp = (int)((v.i >> 23) & 0xFF);
    if (exp == 0) return sign << 23;
    exp = exp - 127 + 63;
    if (exp < 0) return sign << 23;
    if (exp > 0x7F) return sign << 23 | 0x7Fu << 16;
    return mant | (uint32_t)exp << 16 | sign << 23;
}

void unpack8(unsigned c, float o[4]) {
    o[0] = (float)((c >> 24) & 255u) / 255.f; o[1] = (float)((c >> 16) & 255u) / 255.f;
    o[2] = (float)((c >> 8) & 255u) / 255.f;  o[3] = (float)(c & 255u) / 255.f;
}

static uint32_t rd32(const unsigned char* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

typedef struct { uint32_t w0, w1, omh, oml; int zimg; } St;
static St* sts;
static unsigned nst;

static void add_state(St s) {
    for (unsigned i = 0; i < nst; i++) if (!memcmp(&sts[i], &s, sizeof s)) return;
    sts = realloc(sts, (nst + 1) * sizeof *sts);
    sts[nst++] = s;
}

/* the walker rules (op_setcombine) for one G_SETCOMBINE */
static void walker_comb(uint32_t w0, uint32_t w1, uint32_t t[16], int* use_t1, int* box_prod, int* ashade) {
    t[0] = (w0 >> 20) & 0xf;  t[2] = (w0 >> 15) & 0x1f; t[1] = (w1 >> 28) & 0xf; t[3] = (w1 >> 15) & 7;
    t[4] = (w0 >> 12) & 7;    t[6] = (w0 >> 9) & 7;     t[5] = (w1 >> 12) & 7;   t[7] = (w1 >> 9) & 7;
    t[8] = (w0 >> 5) & 0xf;   t[10] = w0 & 0x1f;        t[9] = (w1 >> 24) & 0xf; t[11] = (w1 >> 6) & 7;
    t[12] = (w1 >> 21) & 7;   t[14] = (w1 >> 18) & 7;   t[13] = (w1 >> 3) & 7;   t[15] = w1 & 7;
    int t0 = t[0] == 1 || t[1] == 1 || t[3] == 1 || t[2] == 1 || t[2] == 8 || t[4] == 1 || t[5] == 1 || t[6] == 1 || t[7] == 1;
    int t1 = t[0] == 2 || t[1] == 2 || t[3] == 2 || t[2] == 2 || t[2] == 9 || t[4] == 2 || t[5] == 2 || t[6] == 2 || t[7] == 2;
    int c2 = 0;
    for (int i = 8; i < 16; i++) if (t[i] == 1 || t[i] == 2) c2 = 1;
    if (t[10] == 8 || t[10] == 9) c2 = 1;
    *use_t1 = 0;
    if (t1 && !t0 && !c2) {
        *use_t1 = 1;
        if (t[0] == 2) t[0] = 1;
        if (t[1] == 2) t[1] = 1;
        if (t[3] == 2) t[3] = 1;
        if (t[2] == 2) t[2] = 1;
        if (t[2] == 9) t[2] = 8;
        if (t[4] == 2) t[4] = 1;
        if (t[5] == 2) t[5] = 1;
        if (t[6] == 2) t[6] = 1;
        if (t[7] == 2) t[7] = 1;
    }
    *box_prod = t[0] == 1 && t[1] >= 7 && t[2] == 2 && t[3] == 7 && t[4] == 7 && t[5] == 7 && t[6] == 7 && t[7] == 2 &&
                t[8] == 3 && t[9] == 5 && t[10] == 0 && t[11] == 5 && t[12] == 7 && t[13] == 7 && t[14] == 7 && t[15] == 0;
    *ashade = ((w0 >> 12) & 7) == 4 || ((w0 >> 9) & 7) == 4 || ((w1 >> 12) & 7) == 4 || ((w1 >> 9) & 7) == 4;
}

/* the old path: nd_comb_decide of nd_comb.c (core with the walker vprim, then the plan colours of the inputs), then the model state recipe for the TEV */
static void old_path(const NdCombIn* in, const St* s, unsigned vprim, NdCombOut* co, NdRender* r, NdTevOut* to) {
    nd_comb_core(in, vprim, 0, co);
    if (co->plan_on) pm_comb_colours(&co->plan, in->prim, in->env, in->prim_lo8, in->key_c, in->key_s);
    nd_render_state(s->omh, s->oml, s->zimg, r);
    const int combine = co->plan_on ? 8 : co->leg;
    const int tex = in->tex_on && combine >= 2;
    const int nz = combine == 8 && co->plan.uses_noise;
    const int t1n = combine == 8 && co->plan.uses_tex1;
    NdTmplKey k;
    memset(&k, 0, sizeof k);
    k.z = 1; k.zcmp = r->zcmp; k.zupd = r->zupd; k.nocol = r->nocol; k.decal = r->decal; k.atest_on = r->atest >= 0; k.aref = (uint8_t)(r->atest >= 0 ? r->atest : 0);
    k.combine = (uint8_t)combine; k.tex = (uint8_t)tex; k.tex1 = (uint8_t)t1n; k.r_memblend = (uint8_t)(r->set_memb ? r->memb : 0); k.r_blend = r->blend; k.noise = (uint8_t)nz;
    k.w0 = combine == 8 ? in->w0 : 0; k.w1 = combine == 8 ? in->w1 : 0; k.two = combine == 8 ? (uint8_t)(((in->omode_h >> 20) & 3u) == 1u) : 0;
    NdTevIn ti = { &k, combine == 8 ? &co->plan : NULL, in->prim, co->cc_on ? co->cc : 0, 0 };
    nd_tev_build(&ti, to);
}

static unsigned n_ent, n_bad, n_plan, n_c4, n_tex;
static int fault;

static void one(const St* s, int tex_on, int t1v, int dual, int cset, uint32_t kc, uint32_t ks, uint32_t k4, int vprim, unsigned plod, int folded, int leg, uint32_t prim, uint32_t env) {
    uint32_t t[16];
    int use_t1, box_prod, ashade;
    walker_comb(s->w0, s->w1, t, &use_t1, &box_prod, &ashade);
    if (folded && !box_prod) return;
    if (folded) { t[0] = 3; t[1] = 5; t[2] = 1; t[3] = 5; t[4] = 7; t[5] = 7; t[6] = 7; t[7] = 1; use_t1 = 1; }
    NdCombIn in;
    memset(&in, 0, sizeof in);
    in.w0 = s->w0; in.w1 = s->w1; in.omode_h = s->omh; in.prim = prim; in.env = env; in.prim_lo8 = plod; in.key_c = kc; in.key_s = ks; in.conv_k4 = k4;
    memcpy(in.tst, t, sizeof t);
    in.leg_mode = leg; in.tex_on = tex_on; in.use_t1 = use_t1; in.t1_valid = t1v; in.dual_on = dual; in.box_prod = box_prod; in.box_pat = 0; in.box_folded = folded;
    in.comb_set = cset; in.comb_alpha_shade = ashade;
    NdPsoDesc d;
    memset(&d, 0, sizeof d);
    d.cw0 = s->w0; d.cw1 = s->w1; d.omh = s->omh; d.oml = s->oml; d.key_c = kc; d.key_s = ks; d.conv_k4 = k4;
    d.tex_on = fault ? 0 : (uint8_t)tex_on; d.use_t1 = (uint8_t)use_t1; d.t1_valid = (uint8_t)t1v; d.dual_on = (uint8_t)dual;
    d.box_prod = (uint8_t)box_prod; d.box_folded = (uint8_t)folded; d.comb_set = (uint8_t)cset; d.comb_alpha_shade = (uint8_t)ashade;
    d.leg_prev = (uint8_t)leg; d.vprim = (uint8_t)vprim; d.zimg = (uint8_t)s->zimg; d.plod = (uint8_t)plod;

    NdCombOut co;
    NdRender r;
    NdTevOut to;
    pm_comb_set_k4(k4);
    old_path(&in, s, (unsigned)vprim, &co, &r, &to);
    NdPsoOut o;
    nd_pso_build(&d, &o);
    uint32_t cc = 0;
    int cc_ok = nd_pso_cc(&d, prim, env, &cc);
    uint32_t tev_col[6], buf_col, vtx;
    nd_pso_colour(&o, prim, env, plod, kc, ks, cc, tev_col, &buf_col, &vtx);
    int bad = 0;
    const NdCombOut* c = &o.comb;
    bad |= c->leg != co.leg || c->mode5 != co.mode5 || c->pat != co.pat || c->use != co.use || c->why != co.why || c->cyc != co.cyc || c->dual != co.dual ||
           c->plan_on != co.plan_on || c->cc_on != co.cc_on || c->env_on != co.env_on;
    if (co.cc_on) bad |= !cc_ok || cc != co.cc;
    if (co.plan_on) {
        PmPlan p = c->plan;
        pm_comb_colours(&p, prim, env, plod, kc, ks);
        bad |= memcmp(&p, &co.plan, sizeof p) != 0;
        bad |= vtx != co.plan.vtx;
    }
    bad |= memcmp(&o.rnd, &r, sizeof r) != 0;
    for (int i = 0; i < 6; i++) {
        bad |= tev_col[i] != to.st[i][3];
        for (int w = 0; w < 5; w++) if (w != 3) bad |= o.tev.st[i][w] != to.st[i][w];
    }
    bad |= buf_col != (to.buf_set ? to.buf_color : 0) || o.tev.buf_set != to.buf_set || o.tev.n != to.n || o.tev.memb != to.memb || o.tev.mix_st != to.mix_st;
    n_ent++; n_plan += (unsigned)co.plan_on; n_c4 += co.leg == 4 && co.cc_on; n_tex += (unsigned)tex_on;
    if (bad) {
        n_bad++;
        if (n_bad < 6) printf("BAD cw %08x %08x omh %08x oml %08x tex %d t1v %d dual %d leg %d/%d use %d/%d plan %d/%d\n", s->w0, s->w1, s->omh, s->oml, tex_on, t1v, dual, c->leg, co.leg, c->use, co.use, c->plan_on, co.plan_on);
    }
}

int main(int argc, char** argv) {
    int a = 1;
    if (a < argc && !strcmp(argv[a], "--fault")) { fault = 1; a++; }
    if (a >= argc) { fprintf(stderr, "usage: nd_decide_test [--fault] <ndref.bin> ...\n"); return 2; }
    for (; a < argc; a++) {
        FILE* f = fopen(argv[a], "rb");
        if (!f) { printf("FAIL: cannot open %s\n", argv[a]); return 1; }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        unsigned char* buf = malloc((size_t)sz);
        assert(buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz);
        fclose(f);
        assert(memcmp(buf, "NDR1", 4) == 0 && rd32(buf + 4) == 4);
        for (size_t pos = 20, end = 20 + rd32(buf + 16); pos < end;) {
            uint32_t kind = rd32(buf + pos), size = rd32(buf + pos + 4);
            const unsigned char* b = buf + pos + 8;
            if (kind == 4) { St s = { rd32(b + 32), rd32(b + 36), rd32(b + 48), rd32(b + 52), rd32(b + 56) != 0 }; add_state(s); }
            pos += 8 + size;
        }
        free(buf);
    }
    { /* the captures hold no constant colour combine (leg 4): one synthetic state, colour PRIM, alpha PRIM alpha * TEXEL0, one cycle and two cycles */
        St c1 = { 0xFC000000u | 1u << 12 | 3u << 9, 3u << 15 | 7u << 12 | 7u << 9, 0u, 0x00552048u, 0 };
        St c2 = c1; c2.omh = 1u << 20; c2.w1 |= 7u << 21 | 7u << 18 | 7u << 3; c2.w0 |= 7u << 5;
        add_state(c1); add_state(c2);
    }
    static const uint32_t pe[4][2] = { { 0, 0 }, { 0xff8040c0u, 0x20a0e010u }, { 0x12345678u, 0xfedcba98u }, { 0x000000ffu, 0xffffff00u } };
    for (unsigned i = 0; i < nst; i++) {
        const St* s = &sts[i];
        for (int tex = 0; tex < 2; tex++) for (int t1v = 0; t1v < 2; t1v++) for (int dual = 0; dual < 2; dual++) for (int folded = 0; folded < 2; folded++)
        for (int v = 0; v < 3; v++) {
            const uint32_t kc = v == 0 ? 0 : 0x80402000u, ks = v == 0 ? 0 : 0x10203000u, k4 = v == 0 ? 0 : 0x55u;
            for (int p = 0; p < 4; p++) one(s, tex, t1v, dual, 1, kc, ks, k4, v == 2, v == 0 ? 0u : 0x7fu, folded, v == 1 ? 3 : 0, pe[p][0], pe[p][1]);
        }
        one(s, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, pe[1][0], pe[1][1]); /* comb_set 0 */
    }
    printf("decide states %u entries %u bad %u (plans %u, leg 4 %u, textured %u)\n", nst, n_ent, n_bad, n_plan, n_c4, n_tex);
    printf("decide entries %u bad %u\n", n_ent, n_bad);
    return n_bad ? 1 : 0;
}
