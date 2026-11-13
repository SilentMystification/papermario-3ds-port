/* nd_comb.c: the combine group (plan_s10_path group C, ST-2, cut from the former pm_gbi.c, pure cut). 3DS only: the decision and the
 * sync_combine work call pm_gpu_*. The typedef ScMemo is a copy of the former pm_gbi.c one (ST-2). The decision itself is nd_decide.c (NPSO2). */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_int.h"
#include "pm_gpu.h"
#include "nd.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
#include "nd_ops.h"

/* GLOB: definitions moved here from the former pm_gbi.c (layout only). */
unsigned conv_k4; /* G_SETCONVERT K4 (8 bit part), the combiner input K4 */
unsigned keyc, keys; /* B6: G_SETKEYR/G_SETKEYGB chroma key CENTER, 0xRRGGBB00. Only the combiner input CENTER reads it. */
int dual_on;
int box_prod; PM_HOTD int box_folded;
unsigned comb_gen, cmb_gen_seen;
int box_pat; /* D03: (PRIM - ENV) * TEXEL0 + ENV with alpha from TEXEL1 only: window with a tile 0 pattern */
unsigned box_save[4];
int comb_set;
int comb_alpha_shade = 1;
int plan_active; /* S25: the general combiner (mode 8) draws, not the legacy mode */
PmPlan cur_plan;
void sync_combine(void);
extern int pm_comb_vprim; /* S4COL: the particle plan flag (pm_comb.c), in the memo keys */
const char* const cc_nm_a[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "1", "NOISE" };
const char* const cc_nm_b[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "KEYC", "K4" };
const char* const cc_nm_c[16] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "KEYS", "CMBA", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "LODF", "PLODF", "K5" };
const char* const cc_nm_d[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "1", "0" };
const char* const cc_nm_aa[8] = { "CMBA", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "1", "0" };
const char* const cc_nm_ac[8] = { "LODF", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "PLODF", "0" };
int ls_use = -9, ls_why, ls_cyc, ls_dual, ls_frame;
unsigned sc_sig; PM_HOTD int sc_ok; /* DIP2: the inputs of the last sync_combine and whether its result is still what pm_gpu holds */
typedef struct { unsigned sg; int ok, leg, mode5, pat, use, why, cyc, dual, plan_on, cc_on, env_on; unsigned cc; PmPlan plan; } ScMemo;
unsigned* const tst_f[16] = { &nd_ws.tst1.comb_a, &nd_ws.tst1.comb_b, &nd_ws.tst1.comb_c, &nd_ws.tst1.comb_d, &nd_ws.tst1.comb_aa, &nd_ws.tst1.comb_ab, &nd_ws.tst1.comb_ac, &nd_ws.tst1.comb_ad,
    &nd_ws.tst1.comb2_a, &nd_ws.tst1.comb2_b, &nd_ws.tst1.comb2_c, &nd_ws.tst1.comb2_d, &nd_ws.tst1.comb2_aa, &nd_ws.tst1.comb2_ab, &nd_ws.tst1.comb2_ac, &nd_ws.tst1.comb2_ad };
unsigned aux_key;


/* globals of the combine state, owned by the former pm_gbi.c until their group moves (D10 debt, listed in the ST-2 commit) */
extern unsigned aux_key, box_save[4];
extern int box_pat, box_prod, comb_alpha_shade, dual_on, plan_active;
extern int ls_use, ls_why, ls_cyc, ls_dual, ls_frame;
extern PmPlan cur_plan;
extern const char* const cc_nm_a[8], * const cc_nm_aa[8], * const cc_nm_ac[8];
extern unsigned* const tst_f[16];
extern float vp_sx, vp_sy, vp_tx, vp_ty;
int ph_pick(void); void grab_cur(const void* src, int w, int h); void ens_clear(void);
void unpack8(unsigned c, float o[4]);
static void sync_combine_work(void);

#define GP_SET(k, same) do { if (gp_on()) gp_note(k, (same)); } while (0)
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__) /* PG0: the flush counter by cause (prof mode) */

int combchk_on(void);
void combchk_run(const NdCombIn* in, int worked);
ScMemo scm[16];
unsigned scm_next;
extern const char* const cc_nm_b[8], * const cc_nm_c[16], * const cc_nm_d[8];


/* The constant colour mode of the walker state (nd_decide.c nd_const_rgb on the walker's inputs). */
int const_rgb_tex_alpha(unsigned* out) {
    NdCombIn c;
    c.omode_h = nd_ws.omode_h; c.comb_set = comb_set; c.tex_on = nd_ws.tex_on; c.prim = nd_ws.prim; c.env = nd_ws.envc; c.t1_valid = nd_ws.td[1].valid; c.conv_k4 = conv_k4;
    for (int i = 0; i < 16; i++) c.tst[i] = *tst_f[i];
    return nd_const_rgb(&c, out);
}

/* ST-3: leg_mode is the mode pm_gpu got last; sl_* the gpu calls of the last sync_combine work (the legacy mode and the audit are in nd_decide.c, NPSO2). */
int leg_mode;               /* S25: the combine mode sync_combine() last gave pm_gpu */
int comb_mode5; /* (PRIM - ENV) * TEXEL + ENV */
int sl_const, sl_env; /* OPT-SC: the gpu calls of the last sync work, kept in the memo */
unsigned sl_cc;

void cc_text(const unsigned s[2][8], int cyc, char* o, int n) {
    snprintf(o, n, "(%s-%s)*%s+%s / (%s-%s)*%s+%s",
             s[cyc][0] < 8 ? cc_nm_a[s[cyc][0]] : "0", s[cyc][1] < 8 ? cc_nm_b[s[cyc][1]] : "0",
             s[cyc][2] < 16 ? cc_nm_c[s[cyc][2]] : "0", cc_nm_d[s[cyc][3]],
             cc_nm_aa[s[cyc][4]], cc_nm_aa[s[cyc][5]], cc_nm_ac[s[cyc][6]], cc_nm_aa[s[cyc][7]]);
}

/* 1 if the combiner reads texel tile t (0 or 1). */
int cc_uses(const unsigned s[2][8], int two, int t) {
    int u = 0;
    for (int cyc = 0; cyc < (two ? 2 : 1); cyc++) {
        for (int i = 0; i < 4; i++) {
            unsigned v = s[cyc][i];
            if (v == (unsigned)t + 1 || (i == 2 && v == (unsigned)t + 8)) u = 1;
        }
        for (int i = 4; i < 8; i++)
            if (s[cyc][i] == (unsigned)t + 1) u = 1;
    }
    return u;
}

float cc_rgb_in(unsigned sel, int ch, const float* t0, const float* t1, const float* pr,
                       const float* en, int a_ok) {
    switch (sel) {
    case 1: return t0[ch];
    case 2: return t1[ch];
    case 3: return pr[ch];
    case 4: return 1.f; /* SHADE: none off screen */
    case 5: return en[ch];
    case 6: return a_ok ? 1.f : 0.f;
    default: return 0.f;
    }
}

/* (A - B) * C + D, cycle 1 only. COMBINED reads as 0 there. */
void cc_cycle1(const float* t0, const float* t1, float out[4]) {
    float pr[4], en[4];
    unpack8(nd_ws.prim, pr);
    unpack8(nd_ws.envc, en);
    for (int ch = 0; ch < 3; ch++) {
        float a = cc_rgb_in(nd_ws.tst1.comb_a, ch, t0, t1, pr, en, 1);
        float b = cc_rgb_in(nd_ws.tst1.comb_b, ch, t0, t1, pr, en, 0);
        float c;
        switch (nd_ws.tst1.comb_c) {
        case 8: c = t0[3]; break;
        case 9: c = t1[3]; break;
        case 10: c = pr[3]; break;
        case 11: c = 1.f; break;
        case 12: c = en[3]; break;
        default: c = cc_rgb_in(nd_ws.tst1.comb_c, ch, t0, t1, pr, en, 0); break;
        }
        float d = cc_rgb_in(nd_ws.tst1.comb_d, ch, t0, t1, pr, en, 1);
        float v = (a - b) * c + d;
        out[ch] = v < 0.f ? 0.f : v > 1.f ? 1.f : v;
    }
    {
        unsigned sel[4] = { nd_ws.tst1.comb_aa, nd_ws.tst1.comb_ab, nd_ws.tst1.comb_ac, nd_ws.tst1.comb_ad };
        float v[4];
        for (int i = 0; i < 4; i++) {
            switch (sel[i]) {
            case 1: v[i] = t0[3]; break;
            case 2: v[i] = t1[3]; break;
            case 3: v[i] = pr[3]; break;
            case 4: v[i] = 1.f; break;
            case 5: v[i] = en[3]; break;
            case 6: v[i] = (i == 2) ? 0.f : 1.f; break;
            default: v[i] = 0.f; break;
            }
        }
        float r = (v[0] - v[1]) * v[2] + v[3];
        out[3] = r < 0.f ? 0.f : r > 1.f ? 1.f : r;
    }
}

unsigned scm_sig(void) {
    unsigned parts[] = { nd_ws.comb_w0, nd_ws.comb_w1, nd_ws.omode_h, nd_ws.omode_l, nd_ws.prim, nd_ws.envc, (unsigned)(u8)nd_ws.prim_lod, nd_ws.tst1.tile_fmt[0] * 4 + nd_ws.tst1.tile_siz[0],
        (unsigned)(nd_ws.tex_on | nd_ws.use_t1 << 1 | dual_on << 2 | box_prod << 3 | box_pat << 4 | comb_set << 5), nd_ws.geom, (unsigned)pm_comb_vprim };
    unsigned sg = 2166136261u;
    for (unsigned k = 0; k < sizeof parts / sizeof parts[0]; k++) sg = (sg ^ parts[k]) * 16777619u;
    return sg;
}

ScMemo* scm_find(unsigned sg) {
    for (unsigned i = 0; i < 16; i++) if (scm[i].ok && scm[i].sg == sg) return &scm[i];
    return NULL;
}

void scm_store(unsigned sg) {
    ScMemo* m = &scm[scm_next++ & 15];
    m->sg = sg; m->ok = 1; m->leg = leg_mode; m->mode5 = comb_mode5; m->pat = box_pat; m->use = ls_use; m->why = ls_why; m->cyc = ls_cyc; m->dual = ls_dual;
    m->plan_on = plan_active; m->cc_on = sl_const; m->cc = sl_cc; m->env_on = sl_env;
    if (plan_active) m->plan = cur_plan;
}

void scm_replay(const ScMemo* m) {
    if (m->cc_on) pm_gpu_set_const(m->cc);
    if (m->env_on) pm_gpu_set_env(nd_ws.envc);
    leg_mode = m->leg; comb_mode5 = m->mode5; box_pat = m->pat;
    ls_use = m->use; ls_why = m->why; ls_cyc = m->cyc; ls_dual = m->dual; ls_frame = rm_frame;
    plan_active = m->plan_on;
    if (m->plan_on) { cur_plan = m->plan; pm_gpu_plan_key(nd_ws.comb_w0, nd_ws.comb_w1, ((nd_ws.omode_h >> G_MDSFT_CYCLETYPE) & 3) == 1, nd_ws.use_t1); pm_gpu_set_plan(&cur_plan); }
    else pm_gpu_set_combine(leg_mode);
}

/* R2B: the sync_combine input hash (the fields the skip test reads); sync_combine and the g2_same skip use the same value */
unsigned sc_sig_now(void) {
    unsigned parts[] = { nd_ws.comb_w0, nd_ws.comb_w1, nd_ws.omode_h, nd_ws.prim, nd_ws.envc, (unsigned)(u8)nd_ws.prim_lod, keyc, keys, conv_k4, (unsigned)leg_mode,
                         (unsigned)(nd_ws.tex_on | nd_ws.use_t1 << 1 | nd_ws.td[1].valid << 2 | dual_on << 3 | box_prod << 4 | box_pat << 5 | box_folded << 6 | comb_set << 7 | comb_alpha_shade << 8, (unsigned)pm_comb_vprim << 9) };
    unsigned sg = 2166136261u;
    for (unsigned k = 0; k < sizeof parts / sizeof parts[0]; k++) sg = (sg ^ parts[k]) * 16777619u;
    return sg;
}

void comb_in_walker(NdCombIn* in) {
    in->w0 = nd_ws.comb_w0; in->w1 = nd_ws.comb_w1; in->omode_h = nd_ws.omode_h; in->prim = nd_ws.prim; in->env = nd_ws.envc; in->prim_lo8 = (u8)nd_ws.prim_lod;
    in->key_c = keyc; in->key_s = keys; in->conv_k4 = conv_k4;
    for (int i = 0; i < 16; i++) in->tst[i] = *tst_f[i];
    in->leg_mode = leg_mode; in->tex_on = nd_ws.tex_on; in->use_t1 = nd_ws.use_t1; in->t1_valid = nd_ws.td[1].valid; in->dual_on = dual_on;
    in->box_prod = box_prod; in->box_pat = box_pat; in->box_folded = box_folded; in->comb_set = comb_set; in->comb_alpha_shade = comb_alpha_shade;
}

uint32_t gvk_bits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }

unsigned pm_gbi_gvkey(uint32_t* o, unsigned max) {
    uint32_t v[48]; unsigned n = 0;
    v[n++] = nd_ws.envc; v[n++] = (uint32_t)comb_mode5; v[n++] = (uint32_t)plan_active; v[n++] = cur_plan.vtxname; v[n++] = cur_plan.vtx;
    v[n++] = (uint32_t)dual_on; v[n++] = nd_ws.tst1.comb2_a; v[n++] = nd_ws.tst1.comb2_b; v[n++] = nd_ws.tst1.comb2_c; v[n++] = nd_ws.tst1.comb2_d;
    v[n++] = (uint32_t)comb_alpha_shade; v[n++] = (uint32_t)nd_ws.use_t1; v[n++] = (uint32_t)nd_ws.tex_on; v[n++] = (uint32_t)nd_ws.tile_x0; v[n++] = (uint32_t)nd_ws.tile_y0;
    v[n++] = gvk_bits(nd_ws.tile_fx); v[n++] = gvk_bits(nd_ws.tile_fy);
    v[n++] = nd_ws.tst1.tile_shift[0][0]; v[n++] = nd_ws.tst1.tile_shift[0][1]; v[n++] = nd_ws.tst1.tile_shift[1][0]; v[n++] = nd_ws.tst1.tile_shift[1][1];
    v[n++] = gvk_bits(vp_sx); v[n++] = gvk_bits(vp_sy); v[n++] = gvk_bits(vp_tx); v[n++] = gvk_bits(vp_ty);
    v[n++] = nd_ws.omode_l; v[n++] = nd_ws.prim_z; v[n++] = (uint32_t)fog_fm; v[n++] = (uint32_t)fog_fo; v[n++] = (uint32_t)fogmix_on;
    v[n++] = (uint32_t)img_role(0);
    if (n > max) n = max;
    memcpy(o, v, n * sizeof(uint32_t));
    return n;
}

/* S3N-3d: the walker's sticky state at a draw (ndmcheck compare only: the native draw takes these as inputs): the TEXTURE scale word, the geometry mode and the combine words. */
void pm_gbi_sticky(unsigned* tss, unsigned* geo, unsigned* cw0, unsigned* cw1) {
    *tss = ((unsigned)nd_ws.tex_ss << 16) | ((unsigned)nd_ws.tex_ts & 0xffffu); *geo = nd_ws.geom; *cw0 = nd_ws.comb_w0; *cw1 = nd_ws.comb_w1;
}

/* S3N-3d: nd_comb_decide. The combine work on the inputs in (nd_comb_core, nd_decide.c, no walker global), then the plan colours of in->prim and in->env.
 * The fields the work leaves as they were keep the walker's value as before (mode5 when leg 4, why without a decision, cc without the constant mode, plan without a plan). */
void nd_comb_decide(const NdCombIn* in, NdCombOut* out) {
    nd_comb_core(in, (unsigned)pm_comb_vprim, comb_mode5, out);
    if (out->plan_on) pm_comb_colours(&out->plan, in->prim, in->env, in->prim_lo8, in->key_c, in->key_s);
    else out->plan = cur_plan;
    if (out->use < 0) out->why = ls_why;
    if (!out->cc_on) out->cc = sl_cc;
}

void sync_combine_work_timed(void) {
    if (!pm_ph_cen || !ph_pick()) { sync_combine_work(); return; } /* PROF2-2: phase mode only */
    unsigned t0 = pm_ticks();
    sync_combine_work();
    pm_gbi_sync_ticks += (pm_ticks() - t0) * 8u; /* W2: one call in 8 timed */
}

static __attribute__((cold, noinline)) void scw_census(void) { /* PG0 census, prof mode only */
    static unsigned last_sig;
    unsigned sg = scm_sig();
    GP_SET(5, sg == last_sig);
    last_sig = sg;
}

void sync_combine_work(void) {
    NdCombIn in;
    NdCombOut o;
    if (gp_on()) scw_census(); /* PG0: an estimate of how many calls repeat the inputs of the call before (hash of the combiner, render and tile state the function reads) */
    comb_in_walker(&in);
    nd_comb_core(&in, (unsigned)pm_comb_vprim, comb_mode5, &o);
    sl_const = o.cc_on; sl_env = o.env_on;
    if (o.cc_on) { sl_cc = o.cc; pm_gpu_set_const(o.cc); }
    if (o.env_on) pm_gpu_set_env(nd_ws.envc);
    leg_mode = o.leg; comb_mode5 = o.mode5; box_pat = o.pat;
    plan_active = 0;
    ls_use = o.use; if (o.use >= 0) ls_why = o.why; ls_cyc = o.cyc; ls_dual = o.dual; ls_frame = rm_frame;
    if (o.plan_on) {
        cur_plan = o.plan;
        pm_comb_colours(&cur_plan, nd_ws.prim, nd_ws.envc, (u8)nd_ws.prim_lod, keyc, keys);
        pm_gpu_plan_key(nd_ws.comb_w0, nd_ws.comb_w1, o.cyc, nd_ws.use_t1);
        pm_gpu_set_plan(&cur_plan);
        plan_active = 1;
        return;
    }
    pm_gpu_set_combine(leg_mode);
}

void sync_combine(void) {
    {   /* DIP2: the same inputs as the call before give the same combine mode or plan. Skip the work. The inputs are everything sync_legacy, const_rgb_tex_alpha, decide and
         * pm_comb_colours read. pm_gpu_set_combine at the box pattern draw (below) and the start of a display list clear sc_ok. Switch nodip2 turns it off. */
        unsigned sg = sc_sig_now();
        if (sc_ok && sg == sc_sig) { GP_SET(5, 1); return; }
        sc_sig = sg;
        sc_ok = 0; sr_ok = 0; /* set again below when the work is done */
        nd_ent_cfull += nd_ent_in;
    }
    int leg0 = leg_mode, pat0 = box_pat;
    NdCombIn cin; int chk = combchk_on() ? (comb_in_walker(&cin), 1) : 0; /* S3N-3d: inputs before the work */
    int hit = 0;
    { unsigned sig = scm_sig(); ScMemo* m = scm_find(sig); if (m) { scm_replay(m); hit = 1; } else { sync_combine_work_timed(); scm_store(sig); } }
    if (chk) combchk_run(&cin, !hit);
    sc_ok = 1;
    if (leg_mode != leg0 || box_pat != pat0) {   /* the work may change leg_mode and box_pat (inputs): the signature of the next call must start from the new value (PG4: no other input changes, so the hash is made again only then) */
        sc_sig = sc_sig_now();
    }
}

/* All texels of tile 0 are 1.0 (I4 0xF or I8 0xFF)? Then TEXEL0 * TEXEL1 is TEXEL1. */
int tile0_white(void) {
    const u8* src = nd_ws.td[0].src;
    if (!nd_ws.td[0].valid || !src || nd_ws.td[0].fmt != G_IM_FMT_I || nd_ws.td[0].siz > 1) return 0;
    for (int y = 0; y < nd_ws.td[0].h; y++) {
        for (int x = 0; x < nd_ws.td[0].w; x++) {
            if (nd_ws.td[0].siz == 0) {
                unsigned b = src[(y * nd_ws.td[0].stride + x) >> 1];
                if (((x & 1) ? (b & 15u) : (b >> 4)) != 15u) return 0;
            } else if (src[y * nd_ws.td[0].stride + x] != 255) {
                return 0;
            }
        }
    }
    return 1;
}

/* Called before each textured draw. Tiles 0 and 1 are known by then, the combine mode came
 * before them. */
void box_fold_sync(void) {
    int want = box_prod && nd_ws.td[1].valid && tile0_white();
    if (want == box_folded) return;
    box_folded = want;
    comb_gen++;
    if (want) {
        box_save[0] = nd_ws.tst1.comb_a; box_save[1] = nd_ws.tst1.comb_b; box_save[2] = nd_ws.tst1.comb_c; box_save[3] = nd_ws.tst1.comb_d;
        nd_ws.tst1.comb_a = G_CCMUX_PRIMITIVE; nd_ws.tst1.comb_b = G_CCMUX_ENVIRONMENT; nd_ws.tst1.comb_c = G_CCMUX_TEXEL0; nd_ws.tst1.comb_d = G_CCMUX_ENVIRONMENT;
        nd_ws.tst1.comb_aa = 7; nd_ws.tst1.comb_ab = 7; nd_ws.tst1.comb_ac = 7; nd_ws.tst1.comb_ad = 1; /* TEXEL1 alpha, renamed TEXEL0 */
        nd_ws.use_t1 = 1;
    } else {
        nd_ws.tst1.comb_a = box_save[0]; nd_ws.tst1.comb_b = box_save[1]; nd_ws.tst1.comb_c = box_save[2]; nd_ws.tst1.comb_d = box_save[3];
        nd_ws.tst1.comb_aa = 7; nd_ws.tst1.comb_ab = 7; nd_ws.tst1.comb_ac = 7; nd_ws.tst1.comb_ad = 2;
        nd_ws.use_t1 = 0;
    }
    sync_combine();
}

float t1_p[4]; /* the unit 1 uv map set by dual_bind: scale s t, offset s t */
int t1_ox, t1_oy, t1_w, t1_h; /* window origin in the slot, texture size of unit 1 */
static __attribute__((cold, noinline)) void dual_bind_load(unsigned key) { /* HCCUT4: a new tile 1 texture */
    pm_gpu_flush();
    pm_tex_set_wrap(nd_ws.tst1.tile_wrap[1][0], nd_ws.tst1.tile_wrap[1][1], nd_ws.tst1.tile_wrap[1][2], nd_ws.tst1.tile_wrap[1][3]);
    if (nd_ws.td[1].fmt == 0 && nd_ws.td[1].siz == 2) grab_cur(nd_ws.td[1].src, nd_ws.td[1].w, nd_ws.td[1].h);
    pm_tex_load(nd_ws.td[1].src, nd_ws.td[1].fmt, nd_ws.td[1].siz, nd_ws.td[1].w, nd_ws.td[1].h, nd_ws.td[1].pal, nd_ws.td[1].pal_n, nd_ws.td[1].stride);
    pm_tex_set_aux(nd_ws.tst1.tile_wrap[1][0], nd_ws.tst1.tile_wrap[1][1], nd_ws.tst1.tile_wrap[1][2], nd_ws.tst1.tile_wrap[1][3]);
    aux_key = key;
    nd_ws.cur_tile = -1;
    ens_clear();
}

void dual_bind(void) {
    unsigned key = 2166136261u, i;
    int tw0, th0, tw1, th1;
    if (!plan_active || !cur_plan.uses_tex1 || !nd_ws.td[0].valid || !nd_ws.td[1].valid) return;
    {
        unsigned parts[] = { (unsigned)nd_ws.td[1].src, nd_ws.td[1].fmt, nd_ws.td[1].siz, (unsigned)nd_ws.td[1].w, (unsigned)nd_ws.td[1].h, (unsigned)nd_ws.td[1].stride,
                             (unsigned)nd_ws.td[1].pal, nd_ws.tst1.tile_wrap[1][0], nd_ws.tst1.tile_wrap[1][1], nd_ws.tst1.tile_wrap[1][2], nd_ws.tst1.tile_wrap[1][3] };
        for (i = 0; i < sizeof parts / sizeof parts[0]; i++) key = (key ^ parts[i]) * 16777619u;
    }
    if (key != aux_key || !pm_tex_aux_valid()) dual_bind_load(key); /* HCCUT4 */
    ensure_tile(0);
    pm_tex_size(&tw0, &th0);
    pm_tex_aux_size(&tw1, &th1);
    {   /* V12c: the two tiles may have different shifts (bulb glow: tile 0 shift 1, tile 1 shift 0) and TEXEL1 may be a slot with a window origin */
        int ox1, oy1;
        float rs = shift_mul(nd_ws.tst1.tile_shift[1][0]) / shift_mul(nd_ws.tst1.tile_shift[0][0]), rt = shift_mul(nd_ws.tst1.tile_shift[1][1]) / shift_mul(nd_ws.tst1.tile_shift[0][1]); if (PMDL("texrt1")) rt = 1.f; /* TEXGARBLE test switch (scratch): ignore the t shift of tile 1 */
        pm_tex_aux_origin(&ox1, &oy1);
        t1_p[0] = (float)tw0 / (float)tw1 * rs;
        t1_p[1] = (float)th0 / (float)th1 * rt;
        /* WATER (F1): both tile origins with their quarter-texel fractions (tile 0: tile_fx, which unit 0's offset holds; tile 1: SetTileSize uls & 3,
         * which td[1].x0 drops). Without them a panned overlay was off by up to 0.75 texel against tile 0, a different amount every 1 to 3 frames. */
        const float fx1 = (float)(nd_ws.tst1.tile_sz[1][0] & 3) * 0.25f, fy1 = (float)(nd_ws.tst1.tile_sz[1][1] & 3) * 0.25f;
        t1_p[2] = (((float)nd_ws.tile_x0 + nd_ws.tile_fx) * rs - ((float)(nd_ws.td[1].x0 - ox1) + fx1)) / (float)tw1;
        t1_p[3] = (((float)nd_ws.tile_y0 + nd_ws.tile_fy) * rt - ((float)(nd_ws.td[1].y0 - oy1) + fy1)) / (float)th1;
        t1_ox = ox1; t1_oy = oy1; t1_w = tw1; t1_h = th1;
        pm_gpu_set_tex1(t1_p[0], t1_p[1], t1_p[2], t1_p[3]);
    }
}

/* R13b: a tile 1 window that wraps (G_TX_WRAP, mask bits) is read at (s - sl) & (size - 1). The draw may start outside the window
 * (water_splash: window origin 65, droplet s from 0, so the RDP reads the window at 31, 0, 1 ...). dual_bind maps s linearly, which
 * reads the slot far from the window and finds nothing grabbed there. Fold the start of this draw into the window. The draw is
 * kept inside the window when it fits. The wrap inside one draw is not reproduced, the part past the window edge reads the
 * neighbour pixels of the slot. Tile 0 uv range of the draw: u0..u1 and v0..v1. */
unsigned t1fold_n;
void t1_fold(float u0, float u1, float v0, float v1) {
    if (pm_tex_aux_off_idx() < 0) return; /* an ordinary texture: the GPU repeat and mirror give the RDP wrap, the fold moved the start by the mask size (wrong for a mirrored axis, period twice the mask) */
    float o[2] = { t1_p[2], t1_p[3] };
    const float lo[2] = { u0, v0 }, ex[2] = { (u1 - u0) * t1_p[0] * (float)t1_w, (v1 - v0) * t1_p[1] * (float)t1_h };
    const float sc[2] = { t1_p[0], t1_p[1] };
    const int org[2] = { t1_ox, t1_oy }, dim[2] = { t1_w, t1_h };
    int changed = 0;
    for (int a = 0; a < 2; a++) {
        unsigned mask = nd_ws.tst1.tile_wrap[1][2 + a];
        float win, ws, wsz, wrapped;
        if (!mask || mask > 9) continue;
        if (nd_ws.tst1.tile_wrap[1][a] & 2u) continue; /* V21: a CLAMP axis has no wrap or mirror: the RDP reads (s - sl) clamped to the window, the linear map of dual_bind is right. Folding moved the star icons on the Bowser cards by 9 texels. */
        wsz = (float)(1u << mask);
        win = (lo[a] * sc[a] + o[a]) * (float)dim[a] - (float)org[a]; /* window position of the draw start, texels */
        if (win >= 0.f && win < wsz) continue;
        ws = win - wsz * floorf(win / wsz);
        { float lim = ex[a] >= wsz && (float)dim[a] > wsz ? (float)dim[a] : wsz; /* FX2a: a draw over the whole window (flame noise scroll) reads the wrapped pad of the texture */
          wrapped = ws + ex[a] > lim ? (lim - ex[a] > 0.f ? lim - ex[a] : 0.f) : ws; }
        o[a] += (wrapped - win) / (float)dim[a];
        changed = 1;
    }
    if (changed) { pm_gpu_set_tex1(t1_p[0], t1_p[1], o[0], o[1]); t1fold_n++; }
}

/* T1F: the CPU path fold for a ring triangle. dual_bind map reset, then the window fold over the tile 0 uv range of the triangle (same floats as gv_xf and vert_uv of the old draw_tri_cpu). */
void t1_fold_tri(const unsigned ri[3], unsigned tss) {
    float u[3], v[3];
    int tw = 1, th = 1;
    pm_tex_size(&tw, &th);
    if (tw < 1) tw = 1;
    if (th < 1) th = 1;
    unsigned st = nd_ws.use_t1 ? 1 : 0;
    float ms = shift_mul(nd_ws.tst1.tile_shift[st][0]);
    float mt = shift_mul(nd_ws.tst1.tile_shift[st][1]);
    for (int k = 0; k < 3; k++) {
        const Vtx* r = (const Vtx*)(pm_gpu_gring() + 16u * ri[k]);
        float s0 = nd_ws.tex_on ? (float)r->v.tc[0] * (float)(tss >> 16) / 65536.f : 0.f;
        float t0 = nd_ws.tex_on ? (float)r->v.tc[1] * (float)(tss & 0xffffu) / 65536.f : 0.f;
        u[k] = (s0 / 32.f * ms - (float)nd_ws.tile_x0 - nd_ws.tile_fx) / (float)tw;
        v[k] = (t0 / 32.f * mt - (float)nd_ws.tile_y0 - nd_ws.tile_fy) / (float)th;
        if (!nd_ws.tex_on) { u[k] += 0.5f / (float)tw; v[k] += 0.5f / (float)th; }
    }
    pm_gpu_set_tex1(t1_p[0], t1_p[1], t1_p[2], t1_p[3]);
    t1_fold(fminf(u[0], fminf(u[1], u[2])), fmaxf(u[0], fmaxf(u[1], u[2])), fminf(v[0], fminf(v[1], v[2])), fmaxf(v[0], fmaxf(v[1], v[2])));
}



void op_setcombine(u32 w0, u32 w1) {
            GP_SET(0, nd_ws.comb_w0 == w0 && nd_ws.comb_w1 == w1 && comb_set);
            /* POND 3a: an open batch is drawn with the combine it was queued with. A new combine closes it first (the copy record's combine closed the blocker batch with the water combine). */
            if (!(nd_ws.comb_w0 == w0 && nd_ws.comb_w1 == w1 && comb_set)) pm_gpu_flush();
            nd_ws.comb_w0 = w0;
            nd_ws.comb_w1 = w1;
            nd_ws.tst1.comb_a = (w0 >> 20) & 0xf;
            nd_ws.tst1.comb_c = (w0 >> 15) & 0x1f;
            nd_ws.tst1.comb_b = (w1 >> 28) & 0xf;
            nd_ws.tst1.comb_d = (w1 >> 15) & 7;
            comb_set = 1;
            nd_ws.tst1.comb_aa = (w0 >> 12) & 7;
            nd_ws.tst1.comb_ac = (w0 >> 9) & 7;
            nd_ws.tst1.comb_ab = (w1 >> 12) & 7;
            nd_ws.tst1.comb_ad = (w1 >> 9) & 7;
            nd_ws.tst1.comb2_a = (w0 >> 5) & 0xf;
            nd_ws.tst1.comb2_c = w0 & 0x1f;
            nd_ws.tst1.comb2_b = (w1 >> 24) & 0xf;
            nd_ws.tst1.comb2_d = (w1 >> 6) & 7;
            nd_ws.tst1.comb2_aa = (w1 >> 21) & 7;
            nd_ws.tst1.comb2_ac = (w1 >> 18) & 7;
            nd_ws.tst1.comb2_ab = (w1 >> 3) & 7;
            nd_ws.tst1.comb2_ad = w1 & 7;
            /* TEXEL1 with no TEXEL0 anywhere: the texture is tile 1. Rename it so one binding serves. */
            {
                int t0 = nd_ws.tst1.comb_a == 1 || nd_ws.tst1.comb_b == 1 || nd_ws.tst1.comb_d == 1 || nd_ws.tst1.comb_c == 1 || nd_ws.tst1.comb_c == 8 ||
                         nd_ws.tst1.comb_aa == 1 || nd_ws.tst1.comb_ab == 1 || nd_ws.tst1.comb_ac == 1 || nd_ws.tst1.comb_ad == 1;
                int t1 = nd_ws.tst1.comb_a == 2 || nd_ws.tst1.comb_b == 2 || nd_ws.tst1.comb_d == 2 || nd_ws.tst1.comb_c == 2 || nd_ws.tst1.comb_c == 9 ||
                         nd_ws.tst1.comb_aa == 2 || nd_ws.tst1.comb_ab == 2 || nd_ws.tst1.comb_ac == 2 || nd_ws.tst1.comb_ad == 2;
                int c2 = nd_ws.tst1.comb2_a == 1 || nd_ws.tst1.comb2_b == 1 || nd_ws.tst1.comb2_d == 1 || nd_ws.tst1.comb2_c == 1 || nd_ws.tst1.comb2_c == 8 ||
                         nd_ws.tst1.comb2_aa == 1 || nd_ws.tst1.comb2_ab == 1 || nd_ws.tst1.comb2_ac == 1 || nd_ws.tst1.comb2_ad == 1 ||
                         nd_ws.tst1.comb2_a == 2 || nd_ws.tst1.comb2_b == 2 || nd_ws.tst1.comb2_d == 2 || nd_ws.tst1.comb2_c == 2 || nd_ws.tst1.comb2_c == 9 ||
                         nd_ws.tst1.comb2_aa == 2 || nd_ws.tst1.comb2_ab == 2 || nd_ws.tst1.comb2_ac == 2 || nd_ws.tst1.comb2_ad == 2;
                nd_ws.use_t1 = 0;
                if (t1 && !t0 && !c2) {
                    nd_ws.use_t1 = 1;
                    if (nd_ws.tst1.comb_a == 2) nd_ws.tst1.comb_a = 1;
                    if (nd_ws.tst1.comb_b == 2) nd_ws.tst1.comb_b = 1;
                    if (nd_ws.tst1.comb_d == 2) nd_ws.tst1.comb_d = 1;
                    if (nd_ws.tst1.comb_c == 2) nd_ws.tst1.comb_c = 1;
                    if (nd_ws.tst1.comb_c == 9) nd_ws.tst1.comb_c = 8;
                    if (nd_ws.tst1.comb_aa == 2) nd_ws.tst1.comb_aa = 1;
                    if (nd_ws.tst1.comb_ab == 2) nd_ws.tst1.comb_ab = 1;
                    if (nd_ws.tst1.comb_ac == 2) nd_ws.tst1.comb_ac = 1;
                    if (nd_ws.tst1.comb_ad == 2) nd_ws.tst1.comb_ad = 1;
                }
            }
            box_folded = 0;
            box_prod = nd_ws.tst1.comb_a == 1 && nd_ws.tst1.comb_b >= 7 && nd_ws.tst1.comb_c == 2 && nd_ws.tst1.comb_d == 7 &&
                       nd_ws.tst1.comb_aa == 7 && nd_ws.tst1.comb_ab == 7 && nd_ws.tst1.comb_ac == 7 && nd_ws.tst1.comb_ad == 2 &&
                       nd_ws.tst1.comb2_a == 3 && nd_ws.tst1.comb2_b == 5 && nd_ws.tst1.comb2_c == 0 && nd_ws.tst1.comb2_d == 5 &&
                       nd_ws.tst1.comb2_aa == 7 && nd_ws.tst1.comb2_ab == 7 && nd_ws.tst1.comb2_ac == 7 && nd_ws.tst1.comb2_ad == 0;
            comb_alpha_shade = ((w0 >> 12) & 7) == 4 || ((w0 >> 9) & 7) == 4 ||
                               ((w1 >> 12) & 7) == 4 || ((w1 >> 9) & 7) == 4;
            cmb_gen_seen = comb_gen; /* R2B */
}

/* ---- MV-10: debug switch combchk, cut from the former pm_gbi.c (no change) ---- */
/* S3N-3d: debug switch combchk. Each combine work of sync_combine runs nd_comb_decide on its inputs and compares the outputs with the walker state. */
int combchk_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("combchk"); return v; }

unsigned cc_runs, cc_diff; static int cc_last_b = -1; /* the 300 frame bucket of the last log line */
void pm_log_prof_line(const char* fmt, ...); /* pm_log.c: the one line of prof=4 mode */
__attribute__((cold, noinline)) void combchk_run(const NdCombIn* in, int worked) { /* worked: the work ran (sl_* are set); a memo hit replays the outputs only */
    NdCombOut o;
    nd_comb_decide(in, &o);
    cc_runs++;
    int bad = o.leg != leg_mode || o.mode5 != comb_mode5 || o.pat != box_pat || o.use != ls_use || o.why != ls_why || o.cyc != ls_cyc ||
              o.dual != ls_dual || o.plan_on != plan_active || (worked && (o.cc_on != sl_const || o.env_on != sl_env || (sl_const && o.cc != sl_cc))) ||
              (plan_active && memcmp(&o.plan, &cur_plan, sizeof cur_plan) != 0);
    if (bad) cc_diff++;
    unsigned f = pm_frame_count(), b = f / 300u;
    if ((int)b != cc_last_b) { cc_last_b = (int)b; pm_log_prof_line("combchk f%u runs %u diff %u", f, cc_runs, cc_diff); }
}


/* SPR4-3: the dual boost of the walker (draw_tri path): 1 when cycle 2 is (SHADE - ENV) * COMBINED + COMBINED with the shaded bake live. nd_model/sprite native share it. */
int dual_boost_on(void) { return dual_on && nd_ws.tst1.comb2_a == G_CCMUX_SHADE && nd_ws.tst1.comb2_b == G_CCMUX_ENVIRONMENT && nd_ws.tst1.comb2_c == G_CCMUX_COMBINED && nd_ws.tst1.comb2_d == G_CCMUX_COMBINED; }
