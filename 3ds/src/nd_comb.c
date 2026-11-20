/* nd_comb.c: the combine group (plan_s10_path group C, ST-2, cut from the former pm_gbi.c, pure cut). 3DS only: the decision and the
 * nd_comb_sync work call pm_gpu_*. The decision itself is nd_decide.c (NPSO2). */
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
#include "nd_dual.h"
#include "nd_api.h"
#include "nd_snap.h"
#include "nd_psochk.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
#include "nd_ops.h"

/* GLOB: definitions moved here from the former pm_gbi.c (layout only). */
int box_pat; /* D03: (PRIM - ENV) * TEXEL0 + ENV with alpha from TEXEL1 only: window with a tile 0 pattern */
int plan_active; /* S25: the general combiner (mode 8) draws, not the legacy mode */
int nd_cw_ashade(uint32_t w0, uint32_t w1) { return !w0 || ((w0 >> 12) & 7) == 4 || ((w0 >> 9) & 7) == 4 || ((w1 >> 12) & 7) == 4 || ((w1 >> 9) & 7) == 4; } /* NWS2: comb_alpha_shade of the applied words (1 before the first combine) */
int nd_cw_box_prod(uint32_t w0, uint32_t w1) { uint32_t t[16]; nd_comb_tst(w0, w1, 0, 0, t); return t[0] == 1 && t[1] >= 7 && t[2] == 2 && t[3] == 7 && t[4] == 7 && t[5] == 7 && t[6] == 7 && t[7] == 2 &&
    t[8] == 3 && t[9] == 5 && t[10] == 0 && t[11] == 5 && t[12] == 7 && t[13] == 7 && t[14] == 7 && t[15] == 0; } /* NWS2: box_prod of the applied words */
int nd_cw_t1only(uint32_t w0, uint32_t w1) { uint32_t t[16]; nd_comb_tst(w0, w1, 0, 0, t); /* NWS4C: the TEXEL1 rename test of nd_comb_set on the applied words (use_t1) */
    const int t0 = t[0] == 1 || t[1] == 1 || t[3] == 1 || t[2] == 1 || t[2] == 8 || t[4] == 1 || t[5] == 1 || t[6] == 1 || t[7] == 1;
    const int t1 = t[0] == 2 || t[1] == 2 || t[3] == 2 || t[2] == 2 || t[2] == 9 || t[4] == 2 || t[5] == 2 || t[6] == 2 || t[7] == 2;
    const int c2 = t[8] == 1 || t[9] == 1 || t[11] == 1 || t[10] == 1 || t[10] == 8 || t[12] == 1 || t[13] == 1 || t[14] == 1 || t[15] == 1 ||
                   t[8] == 2 || t[9] == 2 || t[11] == 2 || t[10] == 2 || t[10] == 9 || t[12] == 2 || t[13] == 2 || t[14] == 2 || t[15] == 2;
    return t1 && !t0 && !c2; }
PmPlan cur_plan;
extern int pm_comb_vprim; /* S4COL: the particle plan flag (pm_comb.c), in the memo keys */
const char* const cc_nm_a[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "1", "NOISE" };
const char* const cc_nm_b[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "KEYC", "K4" };
const char* const cc_nm_c[16] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "KEYS", "CMBA", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "LODF", "PLODF", "K5" };
const char* const cc_nm_d[8] = { "CMB", "T0", "T1", "PRIM", "SHADE", "ENV", "1", "0" };
const char* const cc_nm_aa[8] = { "CMBA", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "1", "0" };
const char* const cc_nm_ac[8] = { "LODF", "T0A", "T1A", "PRIMA", "SHADEA", "ENVA", "PLODF", "0" };
int ls_use = -9, ls_why, ls_cyc, ls_dual, ls_frame;
unsigned aux_key;


/* globals of the combine state, owned by the former pm_gbi.c until their group moves (D10 debt, listed in the ST-2 commit) */
extern unsigned aux_key;
extern int box_pat, plan_active;
extern int ls_use, ls_why, ls_cyc, ls_dual, ls_frame;
extern PmPlan cur_plan;
extern const char* const cc_nm_a[8], * const cc_nm_aa[8], * const cc_nm_ac[8];
extern float vp_sx, vp_sy, vp_tx, vp_ty;
int ph_pick(void); void grab_cur(const void* src, int w, int h); void ens_clear(void);
void unpack8(unsigned c, float o[4]);

#define GP_SET(k, same) do { if (gp_on()) gp_note(k, (same)); } while (0)
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__) /* PG0: the flush counter by cause (prof mode) */

int combchk_on(void);
void combchk_run(const NdCombIn* in, int worked);
extern const char* const cc_nm_b[8], * const cc_nm_c[16], * const cc_nm_d[8];


/* The constant colour mode of the walker state (nd_decide.c nd_const_rgb on the walker's inputs). */
int const_rgb_tex_alpha(unsigned* out) { /* NWS4B */
    NdCombIn c;
    c.omode_h = nd_ap->in.omode_h; c.comb_set = nd_ap->in.w0 != 0; /* NWS2 */ c.tex_on = nd_c0.tex_on; c.prim = nd_ap->in.prim; c.env = nd_ap->in.env; c.t1_valid = nd_c0.td[1].valid; c.conv_k4 = nd_ap->in.conv_k4; /* NWS4B */
    memcpy(c.tst, nd_ap->in.tst, sizeof c.tst); /* NWS4B */
    return nd_const_rgb(&c, out);
}

/* ST-3: leg_mode is the mode pm_gpu got last; sl_* the gpu calls of the last nd_comb_sync work (the legacy mode and the audit are in nd_decide.c, NPSO2). */
int leg_mode;               /* S25: the combine mode nd_comb_sync() last gave pm_gpu */
int comb_mode5; /* (PRIM - ENV) * TEXEL + ENV */
int sl_const, sl_env; /* OPT-SC: the gpu calls of the last sync work, kept in the memo */
unsigned sl_cc;


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
void cc_cycle1(const float* t0, const float* t1, float out[4]) { /* NWS4B */
    float pr[4], en[4];
    unpack8(nd_ap->in.prim, pr); /* NWS4B */
    unpack8(nd_ap->in.env, en); /* NWS4B */
    for (int ch = 0; ch < 3; ch++) {
        float a = cc_rgb_in(nd_ap->in.tst[0], ch, t0, t1, pr, en, 1); /* NWS4B */
        float b = cc_rgb_in(nd_ap->in.tst[1], ch, t0, t1, pr, en, 0); /* NWS4B */
        float c;
        switch (nd_ap->in.tst[2]) { /* NWS4B */
        case 8: c = t0[3]; break;
        case 9: c = t1[3]; break;
        case 10: c = pr[3]; break;
        case 11: c = 1.f; break;
        case 12: c = en[3]; break;
        default: c = cc_rgb_in(nd_ap->in.tst[2], ch, t0, t1, pr, en, 0); break; /* NWS4B */
        }
        float d = cc_rgb_in(nd_ap->in.tst[3], ch, t0, t1, pr, en, 1); /* NWS4B */
        float v = (a - b) * c + d;
        out[ch] = v < 0.f ? 0.f : v > 1.f ? 1.f : v;
    }
    {
        unsigned sel[4] = { nd_ap->in.tst[4], nd_ap->in.tst[5], nd_ap->in.tst[6], nd_ap->in.tst[7] }; /* NWS4B */
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

/* NPSO4: the walker's description of the draw, the NdPsoDesc fields of the last applied snap (NCS1) (the psochk compare, nd_psochk.c). */
void nd_walker_desc(NdPsoDesc* d) { /* NCS1: the fields of the last applied snap */
    const NdSnapChk* s = nd_snap_cur(); const NdCombIn in = s->in; memset(d, 0, sizeof *d); /* NCS1 */
    d->cw0 = in.w0; d->cw1 = in.w1; d->omh = in.omode_h; d->oml = s->oml; d->geom = s->geom; /* NCS1 */
    d->key_c = in.key_c; d->key_s = in.key_s; d->conv_k4 = in.conv_k4;
    d->tex_on = (uint8_t)in.tex_on; d->use_t1 = (uint8_t)in.use_t1; d->t1_valid = (uint8_t)in.t1_valid; d->dual_on = (uint8_t)in.dual_on;
    d->box_prod = (uint8_t)in.box_prod; d->box_folded = (uint8_t)in.box_folded; d->comb_set = (uint8_t)in.comb_set; d->comb_alpha_shade = (uint8_t)in.comb_alpha_shade;
    d->leg_prev = (uint8_t)in.leg_mode; d->vprim = (uint8_t)pm_comb_vprim; d->zimg = (uint8_t)(ND_ROLE(0) == ROLE_DEPTHQ); d->plod = (uint8_t)s->plod; /* NCS1 */
}

/* NPSO4: the psochk debug switch (read once). Each draw that states a description compares it with the walker's; the first 8 bad ones are logged, and a line every 100 frames. Off: one test. */
static int psochk_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("psochk"); return v; }
static unsigned psochk_draws, psochk_bad, psochk_nlog, psochk_bucket;
static unsigned pk_hit, pk_miss; /* PSOLK: table hits and misses */
extern unsigned pm_frame_n; /* pm_main.c: the logic step counter */
void nd_psochk_draw(const NdPsoDesc* stated) {
    if (!psochk_on()) return;
    NdPsoDesc w; char txt[96];
    nd_walker_desc(&w);
    psochk_draws++;
    if (nd_psochk_cmp(stated, &w, txt, sizeof txt)) {
        psochk_bad++;
        if (psochk_nlog < 8) { psochk_nlog++; pm_log_evt("psochk bad %s", txt); }
    }
    if (pm_frame_n / 100 != psochk_bucket) {
        psochk_bucket = pm_frame_n / 100;
        pm_log_evt("psochk f%u draws %u bad %u lk %u miss %u", pm_frame_n, psochk_draws, psochk_bad, pk_hit, pk_miss);
    }
}

void nd_psochk_end(void) { if (psochk_on()) pm_log_evt("psochk end f%u draws %u bad %u lk %u miss %u", pm_frame_n, psochk_draws, psochk_bad, pk_hit, pk_miss); }

/* PSOSEEN: one log entry per distinct missed key (256 at most) */
static __attribute__((cold, noinline)) void pso_seen_note(const NdCombIn* in) {
    static uint32_t seen[256]; static unsigned n; uint32_t h = 2166136261u;
    if ((unsigned)(in->tex_on | in->use_t1 | in->t1_valid | in->dual_on | in->box_prod | in->box_folded | in->comb_set | in->comb_alpha_shade) > 1u) {
        static int once; if (!once) { once = 1; pm_log_evt("psomiss flags above 1: %d %d %d %d %d %d %d %d", in->tex_on, in->use_t1, in->t1_valid, in->dual_on, in->box_prod, in->box_folded, in->comb_set, in->comb_alpha_shade); } return;
    }
    const uint32_t k[9] = { in->w0, in->w1, in->omode_h, in->prim_lo8 & 0xffu, in->key_c, in->key_s, in->conv_k4, (uint32_t)pm_comb_vprim,
        nd_pso_flags(in->tex_on, in->use_t1, in->t1_valid, in->dual_on, in->box_prod, in->box_folded, in->comb_set, in->comb_alpha_shade) };
    for (unsigned i = 0; i < 9u; i++) h = (h ^ k[i]) * 16777619u;
    for (unsigned i = 0; i < n; i++) { if (seen[i] == h) return; }
    if (n >= 256u) return;
    seen[n++] = h;
    pm_log_evt("psoseen 0x%x, 0x%x, 0x%x, 0x0, 0x0, 0x%x, 0x%x, 0x%x, %d, %d, %d, %d, %d, %d, %d, %d, 0, %u, 0, %u", (unsigned)in->w0, (unsigned)in->w1,
        (unsigned)in->omode_h, (unsigned)in->key_c, (unsigned)in->key_s, (unsigned)in->conv_k4, in->tex_on, in->use_t1, in->t1_valid, in->dual_on,
        in->box_prod, in->box_folded, in->comb_set, in->comb_alpha_shade, (unsigned)pm_comb_vprim, (unsigned)(in->prim_lo8 & 0xffu));
}

/* PSOLK: the combine decision of the walker inputs in: the pso table entry (nd_pso_find) with the walker's colours, or nd_comb_core on a miss. */
static int psooff_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("psooff"); return v; } /* HWFIX1 gate: every key misses */
static void nd_comb_pick(const NdCombIn* in, NdCombOut* o) {
    if (psooff_on() || !nd_pso_find(in, (unsigned)pm_comb_vprim, o)) { pso_seen_note(in); pk_miss++; nd_comb_core(in, (unsigned)pm_comb_vprim, comb_mode5, o); return; }
    pk_hit++;
    nd_pso_fix(o, in, comb_mode5);
}

uint32_t gvk_bits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }

unsigned pm_gbi_gvkey(uint32_t* o, unsigned max) { /* NWS4B */
    uint32_t v[48]; unsigned n = 0;
    v[n++] = nd_ap->in.env; v[n++] = (uint32_t)comb_mode5; v[n++] = (uint32_t)plan_active; v[n++] = cur_plan.vtxname; v[n++] = cur_plan.vtx; /* NWS4B */
    v[n++] = (uint32_t)nd_ap->in.dual_on; v[n++] = nd_ap->in.tst[8]; v[n++] = nd_ap->in.tst[9]; v[n++] = nd_ap->in.tst[10]; v[n++] = nd_ap->in.tst[11]; /* NWS4B */
    v[n++] = (uint32_t)nd_cw_ashade(nd_ap->in.w0, nd_ap->in.w1); /* NWS2 */ v[n++] = (uint32_t)nd_ap->in.use_t1; v[n++] = (uint32_t)nd_c0.tex_on; v[n++] = (uint32_t)nd_c0.tile_x0; v[n++] = (uint32_t)nd_c0.tile_y0; /* NWS4B */
    v[n++] = gvk_bits(nd_c0.tile_fx); v[n++] = gvk_bits(nd_c0.tile_fy);
    v[n++] = nd_c0.tst1.tile_shift[0][0]; v[n++] = nd_c0.tst1.tile_shift[0][1]; v[n++] = nd_c0.tst1.tile_shift[1][0]; v[n++] = nd_c0.tst1.tile_shift[1][1];
    v[n++] = gvk_bits(vp_sx); v[n++] = gvk_bits(vp_sy); v[n++] = gvk_bits(vp_tx); v[n++] = gvk_bits(vp_ty);
    v[n++] = nd_ap->oml; v[n++] = nd_ap->primz; v[n++] = (uint32_t)nd_c0.fog_fm; v[n++] = (uint32_t)nd_c0.fog_fo; v[n++] = (uint32_t)fogmix_on; /* NWS4B */
    v[n++] = (uint32_t)ND_ROLE(0);
    if (n > max) n = max;
    memcpy(o, v, n * sizeof(uint32_t));
    return n;
}

/* S3N-3d: the walker's sticky state at a draw (ndmcheck compare only: the native draw takes these as inputs): the TEXTURE scale word, the geometry mode and the combine words. */
void pm_gbi_sticky(unsigned* tss, unsigned* geo, unsigned* cw0, unsigned* cw1) { /* NWS4B */
    *tss = ND_TSS(); *geo = ND_GEOM(); *cw0 = nd_ap->in.w0; *cw1 = nd_ap->in.w1; /* NWS4B */
}

/* S3N-3d: nd_comb_dec. The combine work on the inputs in (nd_comb_core, nd_decide.c, no walker global), then the plan colours of in->prim and in->env.
 * The fields the work leaves as they were keep the walker's value as before (mode5 when leg 4, why without a decision, cc without the constant mode, plan without a plan). */
void nd_comb_dec(const NdCombIn* in, NdCombOut* out) {
    nd_comb_pick(in, out);
    if (out->plan_on) pm_comb_colours(&out->plan, in->prim, in->env, in->prim_lo8, in->key_c, in->key_s);
    else out->plan = cur_plan;
    if (out->use < 0) out->why = ls_why;
    if (!out->cc_on) out->cc = sl_cc;
}

/* NDR0E1: the GPU part of a combine decision. o is the decision (plan coloured by the caller). The snapshot (nd_snap.c) does not call it. */
void nd_comb_apply(const NdCombOut* o, uint32_t w0, uint32_t w1, int use_t1, uint32_t env) {
    sl_const = o->cc_on; sl_env = o->env_on;
    if (o->cc_on) { sl_cc = o->cc; pm_gpu_set_const(o->cc); }
    if (o->env_on) pm_gpu_set_env(env);
    leg_mode = o->leg; comb_mode5 = o->mode5; box_pat = o->pat;
    plan_active = 0;
    ls_use = o->use; if (o->use >= 0) ls_why = o->why; ls_cyc = o->cyc; ls_dual = o->dual; ls_frame = rm_frame;
    if (o->plan_on) {
        cur_plan = o->plan;
        pm_gpu_plan_key(w0, w1, o->cyc, use_t1);
        pm_gpu_set_plan(&cur_plan);
        plan_active = 1;
        return;
    }
    pm_gpu_set_combine(leg_mode);
}


float t1_p[4]; /* the unit 1 uv map set by dual_bind: scale s t, offset s t */
int t1_ox, t1_oy, t1_w, t1_h; /* window origin in the slot, texture size of unit 1 */
static __attribute__((cold, noinline)) void dual_bind_load(unsigned key) { /* HCCUT4: a new tile 1 texture */
    pm_gpu_flush();
    pm_tex_set_wrap(ND_BWR(1)[0], ND_BWR(1)[1], ND_BWR(1)[2], ND_BWR(1)[3]);
    if (ND_BTD(1)->fmt == 0 && ND_BTD(1)->siz == 2) grab_cur(ND_BTD(1)->src, ND_BTD(1)->w, ND_BTD(1)->h);
    pm_tex_load(ND_BTD(1)->src, ND_BTD(1)->fmt, ND_BTD(1)->siz, ND_BTD(1)->w, ND_BTD(1)->h, ND_BTD(1)->pal, ND_BTD(1)->pal_n, ND_BTD(1)->stride);
    pm_tex_set_aux(ND_BWR(1)[0], ND_BWR(1)[1], ND_BWR(1)[2], ND_BWR(1)[3]);
    aux_key = key;
    nd_c0.cur_tile = -1;
    ens_clear();
}

void dual_bind(void) {
    unsigned key = 2166136261u, i;
    int tw0, th0, tw1, th1;
    if (!plan_active || !cur_plan.uses_tex1 || !ND_BTD(0)->valid || !ND_BTD(1)->valid) return;
    {
        unsigned parts[] = { (unsigned)ND_BTD(1)->src, ND_BTD(1)->fmt, ND_BTD(1)->siz, (unsigned)ND_BTD(1)->w, (unsigned)ND_BTD(1)->h, (unsigned)ND_BTD(1)->stride,
                             (unsigned)ND_BTD(1)->pal, ND_BWR(1)[0], ND_BWR(1)[1], ND_BWR(1)[2], ND_BWR(1)[3] };
        for (i = 0; i < sizeof parts / sizeof parts[0]; i++) key = (key ^ parts[i]) * 16777619u;
    }
    if (key != aux_key || !pm_tex_aux_valid()) dual_bind_load(key); /* HCCUT4 */
    nd_tile_bind(0);
    pm_tex_size(&tw0, &th0);
    pm_tex_aux_size(&tw1, &th1);
    {   /* V12c: the two tiles may have different shifts (bulb glow: tile 0 shift 1, tile 1 shift 0) and TEXEL1 may be a slot with a window origin */
        int ox1, oy1;
        float rs = shift_mul(ND_BSH(1)[0]) / shift_mul(ND_BSH(0)[0]), rt = shift_mul(ND_BSH(1)[1]) / shift_mul(ND_BSH(0)[1]); if (PMDL("texrt1")) rt = 1.f; /* TEXGARBLE test switch (scratch): ignore the t shift of tile 1 */
        pm_tex_aux_origin(&ox1, &oy1);
        t1_p[0] = (float)tw0 / (float)tw1 * rs;
        t1_p[1] = (float)th0 / (float)th1 * rt;
        /* WATER (F1): both tile origins with their quarter-texel fractions (tile 0: tile_fx, which unit 0's offset holds; tile 1: SetTileSize uls & 3,
         * which td[1].x0 drops). Without them a panned overlay was off by up to 0.75 texel against tile 0, a different amount every 1 to 3 frames. */
        const float fx1 = (float)(ND_BSZ(1)[0] & 3) * 0.25f, fy1 = (float)(ND_BSZ(1)[1] & 3) * 0.25f;
        t1_p[2] = (((float)nd_c0.tile_x0 + nd_c0.tile_fx) * rs - ((float)(ND_BTD(1)->x0 - ox1) + fx1)) / (float)tw1;
        t1_p[3] = (((float)nd_c0.tile_y0 + nd_c0.tile_fy) * rt - ((float)(ND_BTD(1)->y0 - oy1) + fy1)) / (float)th1;
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
        unsigned mask = ND_BWR(1)[2 + a];
        float win, ws, wsz, wrapped;
        if (!mask || mask > 9) continue;
        if (ND_BWR(1)[a] & 2u) continue; /* V21: a CLAMP axis has no wrap or mirror: the RDP reads (s - sl) clamped to the window, the linear map of dual_bind is right. Folding moved the star icons on the Bowser cards by 9 texels. */
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
void t1_fold_tri(const unsigned ri[3], unsigned tss) { /* NWS4B */
    nd_bsrc = nd_ep_bs; /* NDR0G2: the tile words of the open epoch */
    float u[3], v[3];
    int tw = 1, th = 1;
    pm_tex_size(&tw, &th);
    if (tw < 1) tw = 1;
    if (th < 1) th = 1;
    unsigned st = nd_ap->in.use_t1 ? 1 : 0; /* NWS4B */
    float ms = shift_mul(ND_BSH(st)[0]);
    float mt = shift_mul(ND_BSH(st)[1]);
    for (int k = 0; k < 3; k++) {
        const Vtx* r = (const Vtx*)(pm_gpu_gring() + 16u * ri[k]);
        float s0 = nd_c0.tex_on ? (float)r->v.tc[0] * (float)(tss >> 16) / 65536.f : 0.f;
        float t0 = nd_c0.tex_on ? (float)r->v.tc[1] * (float)(tss & 0xffffu) / 65536.f : 0.f;
        u[k] = (s0 / 32.f * ms - (float)nd_c0.tile_x0 - nd_c0.tile_fx) / (float)tw;
        v[k] = (t0 / 32.f * mt - (float)nd_c0.tile_y0 - nd_c0.tile_fy) / (float)th;
        if (!nd_c0.tex_on) { u[k] += 0.5f / (float)tw; v[k] += 0.5f / (float)th; }
    }
    pm_gpu_set_tex1(t1_p[0], t1_p[1], t1_p[2], t1_p[3]);
    t1_fold(fminf(u[0], fminf(u[1], u[2])), fmaxf(u[0], fmaxf(u[1], u[2])), fminf(v[0], fminf(v[1], v[2])), fmaxf(v[0], fmaxf(v[1], v[2])));
    nd_bsrc = NULL; /* NDR0G2 */
}



/* ---- MV-10: debug switch combchk, cut from the former pm_gbi.c (no change) ---- */
/* S3N-3d: debug switch combchk. Each combine work of nd_comb_sync runs nd_comb_dec on its inputs and compares the outputs with the walker state. */
int combchk_on(void) { static int v = -1; if (v < 0) v = pm_debug_has("combchk"); return v; }

unsigned cc_runs, cc_diff; static int cc_last_b = -1; /* the 300 frame bucket of the last log line */
void pm_log_prof_line(const char* fmt, ...); /* pm_log.c: the one line of prof=4 mode */
__attribute__((cold, noinline)) void combchk_run(const NdCombIn* in, int worked) { /* worked: the work ran (sl_* are set); a memo hit replays the outputs only */
    NdCombOut o;
    nd_comb_dec(in, &o);
    cc_runs++;
    int bad = o.leg != leg_mode || o.mode5 != comb_mode5 || o.pat != box_pat || o.use != ls_use || o.why != ls_why || o.cyc != ls_cyc ||
              o.dual != ls_dual || o.plan_on != plan_active || (worked && (o.cc_on != sl_const || o.env_on != sl_env || (sl_const && o.cc != sl_cc))) ||
              (plan_active && memcmp(&o.plan, &cur_plan, sizeof cur_plan) != 0);
    if (bad) cc_diff++;
    unsigned f = pm_frame_count(), b = f / 300u;
    if ((int)b != cc_last_b) { cc_last_b = (int)b; pm_log_prof_line("combchk f%u runs %u diff %u", f, cc_runs, cc_diff); }
}


/* SPR4-3: the dual boost of the walker (draw_tri path): 1 when cycle 2 is (SHADE - ENV) * COMBINED + COMBINED with the shaded bake live. nd_model/sprite native share it. */
int dual_boost_on(void) { return nd_ap->in.dual_on && nd_ap->in.tst[8] == G_CCMUX_SHADE && nd_ap->in.tst[9] == G_CCMUX_ENVIRONMENT && nd_ap->in.tst[10] == G_CCMUX_COMBINED && nd_ap->in.tst[11] == G_CCMUX_COMBINED; } /* NWS4B */
