/* nd_scomp.c: the state compiler (NATIVEAPI S6T, nd_scomp.h; design note: git show b16f91c46:docs/handoff/row_S6T.md). A def (a producer's
 * range of typed state units, static data) is analysed once on core 1 into its folded writes; on core 0 each run applies them to the live
 * walker words, binds the combine decision of the range under the inherited inputs (made once per key, cold: nd_comb_decide, the walker's own
 * decision) and runs the value units. The old per-op path (skip tests, setters, a combine sync per op) does not run for the static units.
 * Host test: nd_scomp_test.c builds the pure part (ND_SCOMP_HOST). */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_op.h"
#include "nd_scomp.h"

static unsigned def_next; /* core 1: the ids of the analysed defs (a hint only: last[]; the key word 0 is the def pointer) */

/* The class of one unit: 0 refused, 1 static, 2 value, 3 no-op (dropped), 4 ordered value (SCOMP0: PRIM, ENV, texture setters; must not lie between two static units; class 2 too must not lie between two static units (S6VORD: sc_exec runs it after the block)). */
static unsigned unit_class(uint32_t w0) {
    const unsigned id = w0 >> 24, a = w0 & 0xffffffu;
    if (w0 == ND_NOP_A) return 3u;
    switch (id) {
    case NDO_OMH: case NDO_OML: case NDO_GEOM: case NDO_TEXTURE: case NDO_COMB: return 1u;
    case NDO_FOG: case NDO_BLEND: case NDO_FILL: case NDO_PRIMZ: case NDO_SCISSOR: case NDO_MTX: case NDO_POPMTX: case NDO_MOVEMEM: return 2u;
    case NDO_MW: return (a >> 16) == NDO_MW_NONE ? 3u : 2u; /* S6VORD: clip ratio, persp normalize: no field, a no-op */
    case NDO_MISC: return ((a >> 16) == NDO_MISC_CIMG || (a >> 16) == NDO_MISC_ND) ? 2u : 0u;
    case NDO_PRIM: case NDO_ENV: case NDO_TIMG: case NDO_TILE: case NDO_TILESIZE:
    case NDO_LOADBLOCK: case NDO_LOADTILE: case NDO_LOADTLUT: return 4u; /* SCOMP0: an ordered value unit */
    default: return 0u; /* RARE, draws and control */
    }
}

/* S6SYNC: 1 when the op path runs sync_combine on this static unit (set_om over the cycle bits, nd_op_apply TEXTURE and COMB). */
static int unit_sync(uint32_t w0) {
    const unsigned a = w0 & 0xffffffu;
    switch (w0 >> 24) {
    case NDO_OMH: { const unsigned sft = a & 0xffu, len = (a >> 8) & 0xffu; return sft <= G_MDSFT_CYCLETYPE && sft + len > G_MDSFT_CYCLETYPE; }
    case NDO_TEXTURE: case NDO_COMB: return 1;
    default: return 0;
    }
}

int nd_scomp_analyse(NdSDef* d) {
    const uint32_t* u = (const uint32_t*)d->u;
    int ok = d->n >= 1u && d->n <= 32u, seen = 0;
    unsigned first = 32u, last = 0u; uint32_t om = 0;
    d->nv = d->pre = 0; d->vmask = 0;
    d->omh_m = d->omh_v = d->oml_m = d->oml_v = 0;
    d->geom_c = ~0u; d->geom_s = 0;
    d->has_geom = d->has_tex = d->has_comb = 0; d->last_op = 0;
    d->nsync = 0; memset(d->sync_k, 0, sizeof d->sync_k);
    for (unsigned k = 0; k < d->n && k < 32u; k++) {
        const uint32_t w0 = u[2u * k], b = u[2u * k + 1u], a = w0 & 0xffffffu;
        const unsigned c = unit_class(w0), id = w0 >> 24;
        if (c == 3u) continue;
        if (c == 0u) { ok = 0; continue; }
        if (c == 2u || c == 4u) om |= 1u << k; /* S6VORD: no value unit between statics */
        if (c == 2u || c == 4u) {
            d->vmask |= 1u << k; d->nv++;
            if (!seen) d->pre++;
            continue;
        }
        seen = 1;
        if (first == 32u) first = k;
        last = k;
        d->last_op = nd_op_n64[id - NDO_BASE];
        if (unit_sync(w0)) { if (d->nsync >= 4u) ok = 0; else d->sync_k[d->nsync++] = (uint8_t)k; }
        switch (id) {
        case NDO_OMH: { const uint32_t m = nd_op_om_mask(a); d->omh_v = (d->omh_v & ~m) | (b & m); d->omh_m |= m; break; }
        case NDO_OML: { const uint32_t m = nd_op_om_mask(a); d->oml_v = (d->oml_v & ~m) | (b & m); d->oml_m |= m; break; }
        case NDO_GEOM: d->geom_c &= a; d->geom_s = (d->geom_s & a) | b; d->has_geom = 1; break; /* set_geom: geom = (geom & a) | b, composed */
        case NDO_TEXTURE: d->tex_a = a; d->tex_b = b; d->has_tex = 1; break;
        case NDO_COMB: d->cw0 = NDO_COMB_W0(a); d->cw1 = b; d->has_comb = 1; break;
        default: break; /* MW none */
        }
    }
    if (!seen) ok = 0; /* only value units: nothing to compile */
    for (unsigned k = 0; k < 32u; k++) if ((om >> k & 1u) && k > first && k < last) ok = 0; /* between two static units */
    if (ok) d->id = (uint8_t)__atomic_fetch_add(&def_next, 1u, __ATOMIC_RELAXED); /* a hint only (last[]) */
    return ok;
}

/* The key: the def and the combine decision inputs after the def's writes (nd_comb_decide NdCombIn and pm_comb_vprim). A field the def sets
 * takes the def's value; with a combine unit the combine fields derived from the words (tst1, use_t1, box_prod, box_folded 0, comb_set 1,
 * comb_alpha_shade) are a function of the def, so they are left out. */
void nd_scomp_key(const NdSDef* d, const NdCombIn* in, uint32_t vprim, uint32_t key[ND_SC_KEYW]) {
    const uint32_t omh = (in->omode_h & ~d->omh_m) | d->omh_v;
    const uint32_t tex_on = d->has_tex ? (d->tex_a & 1u) : (uint32_t)in->tex_on;
    uint32_t fl = (in->prim_lo8 & 0xffu) | tex_on << 8 | (uint32_t)(in->t1_valid != 0) << 10 | (uint32_t)(in->dual_on != 0) << 11 | (vprim & 0xffffu) << 16;
    key[0] = (uint32_t)(uintptr_t)d;
    if (d->has_comb) {
        key[1] = d->cw0; key[2] = d->cw1;
        for (unsigned i = 0; i < 16u; i++) key[3u + i] = 0;
    } else {
        key[1] = in->w0; key[2] = in->w1;
        for (unsigned i = 0; i < 16u; i++) key[3u + i] = in->tst[i];
        fl |= (uint32_t)(in->use_t1 != 0) << 9 | (uint32_t)(in->box_prod != 0) << 12 | (uint32_t)(in->box_folded != 0) << 13 | (uint32_t)(in->comb_set != 0) << 14 |
              (uint32_t)(in->comb_alpha_shade != 0) << 15;
    }
    key[19] = (omh >> G_MDSFT_CYCLETYPE) & 3u;
    key[20] = in->prim; key[21] = in->env; key[22] = fl;
    key[23] = in->key_c; key[24] = in->key_s; key[25] = in->conv_k4;
}

unsigned nd_scomp_find(NdSTab* t, unsigned def_id, const uint32_t key[ND_SC_KEYW], int* miss) {
    unsigned i = t->last[def_id & 15u];
    if (t->used[i] && !memcmp(t->key[i], key, sizeof t->key[i])) { *miss = 0; return i; }
    uint32_t h = 2166136261u; unsigned fr; int fr_ok = 0;
    for (unsigned k = 0; k < ND_SC_KEYW; k++) h = (h ^ key[k]) * 16777619u;
    i = (h ^ (h >> 15)) & (ND_SC_N - 1u); fr = i;
    for (unsigned p = 0; p < 8u; p++) {
        const unsigned j = (i + p) & (ND_SC_N - 1u);
        if (!t->used[j]) { if (!fr_ok) { fr = j; fr_ok = 1; } continue; }
        if (t->key[j][0] == key[0] && !memcmp(t->key[j], key, sizeof t->key[j])) { t->last[def_id & 15u] = (uint8_t)j; *miss = 0; return j; }
    }
    memcpy(t->key[fr], key, sizeof t->key[fr]); /* a miss: the first free probe slot, else the home slot */
    t->used[fr] = 1; t->last[def_id & 15u] = (uint8_t)fr; *miss = 1;
    return fr;
}

#ifndef ND_SCOMP_HOST
#include "nd_int.h"
#include "nd_api.h"
#include "pm_gpu.h"
#include "pm_port.h"

#ifndef pm_gpu_flush
#define pm_gpu_flush() pm_gpu_flush_at(__LINE__)
#endif
void comb_in_walker(NdCombIn* in);                     /* nd_comb.c */
void pm_tex_set_filter(int linear);                    /* pm_tex.c */
void nd_rec_state(uint32_t w0, uint32_t w1);           /* nd_rec.c: one typed unit into the open state run */
extern int ls_use, ls_why, ls_cyc, ls_dual, ls_frame;  /* nd_comb.c */
extern int sl_const, sl_env;
extern unsigned sl_cc;
extern int pm_comb_vprim;                              /* pm_comb.c */
extern unsigned* const tst_f[16];                      /* nd_comb.c */

/* ---- core 1: the record ---- */
static __attribute__((cold, noinline)) void rec_ops(const NdSDef* d, const uint32_t* v, unsigned nv) {
    const uint32_t* u = (const uint32_t*)d->u;
    unsigned j = 0;
    if (nv != d->nv) { static int n; if (n++ < 4) pm_log_e("scomp: def %p has %u value units, record %u", (const void*)d, (unsigned)d->nv, nv); }
    for (unsigned k = 0; k < d->n; k++) {
        uint32_t w0 = u[2u * k], w1 = u[2u * k + 1u];
        if (d->vmask >> k & 1u) {
            if (j >= nv) continue;
            w0 = v[2u * j]; w1 = v[2u * j + 1u]; j++;
        }
        if (w0 == ND_NOP_A) continue; /* the syncs are dropped (as nd_gx_w) */
        nd_rec_state(w0, w1);
    }
}
void nd_scomp_rec(NdSDef* d, const void* vv, unsigned nv) {
    const uint32_t* v = (const uint32_t*)vv;
    uint32_t p[2u + 2u * 32u];
    if (!d->st) d->st = nd_scomp_analyse(d) ? 1u : 2u;
    if (d->st != 1u || nv != d->nv || nv > 32u) { rec_ops(d, v, nv); return; }
    p[0] = (uint32_t)(uintptr_t)d;
    p[1] = nv;
    memcpy(p + 2, v, nv * 8u);
    nd_rec_napi(nd_scomp_run, p, 1u + nv);
}

/* ---- core 0: the run ---- */
static NdSTab* sc_tab;       /* the keys (the entry ids) */
static NdCombOut* sc_out;    /* the decision of each entry */
static unsigned sc_made;     /* entries made (cold decisions) */

static __attribute__((cold, noinline)) int sc_alloc(void) {
    sc_tab = (NdSTab*)calloc(1, sizeof *sc_tab);
    sc_out = (NdCombOut*)calloc(ND_SC_N, sizeof *sc_out);
    if (!sc_tab || !sc_out) pm_fatal_stop("scomp: no memory for the compiled ranges");
    return 1;
}
static __attribute__((cold, noinline)) void sc_make(NdCombOut* o, const NdCombIn* in) {
    nd_comb_decide(in, o);
    sc_made++;
}

/* The othermode H write as set_om does it, without the combine sync. */
static void sc_omh(uint32_t m, uint32_t v) {
    const unsigned old_tf = nd_ws.omode_h & (3u << 12);
    nd_ws.omode_h = (nd_ws.omode_h & ~m) | (v & m);
    nd_ws.om_kh |= m;
    if ((nd_ws.omode_h & (3u << 12)) != old_tf) { /* V15: G_MDSFT_TEXTFILT, as set_om */
        pm_gpu_flush();
        pm_tex_set_filter((nd_ws.omode_h & (3u << 12)) != 0);
    }
}

/* The folded writes of a def with no sync unit on the live walker words, as the def's setters leave them. */
static void sc_block(const NdSDef* d) {
    if (d->omh_m) sc_omh(d->omh_m, d->omh_v);
    if (d->oml_m) { nd_ws.omode_l = (nd_ws.omode_l & ~d->oml_m) | d->oml_v; nd_ws.om_kl |= d->oml_m; }
    if (d->has_geom) set_geom(d->geom_c, d->geom_s);
    if (d->has_tex) nd_set_texture(d->tex_a, d->tex_b);
    /* the NDO_COMB skip test without its sync part: equal words, set, not folded, no fold since the last combine */
    if (d->has_comb && (nd_ws.comb_w0 != d->cw0 || nd_ws.comb_w1 != d->cw1 || !comb_set || box_folded || cmb_gen_seen != comb_gen)) op_setcombine(d->cw0, d->cw1);
}

/* The decision of the entry into the walker and the GPU (scm_replay form, with the sync_legacy marks). */
static void sc_replay(const NdCombOut* o) {
    leg_mode = o->leg; comb_mode5 = o->mode5; box_pat = o->pat;
    ls_use = o->use; ls_why = o->why; ls_cyc = o->cyc; ls_dual = o->dual; ls_frame = rm_frame;
    sl_const = o->cc_on; sl_env = o->env_on; sl_cc = o->cc;
    plan_active = o->plan_on;
    if (o->cc_on) pm_gpu_set_const(o->cc);
    if (o->env_on) pm_gpu_set_env(nd_ws.envc);
    if (o->plan_on) {
        cur_plan = o->plan;
        pm_gpu_plan_key(nd_ws.comb_w0, nd_ws.comb_w1, o->cyc, nd_ws.use_t1);
        pm_gpu_set_plan(&cur_plan);
    } else {
        pm_gpu_set_combine(leg_mode);
    }
}

static const NdSDef sc_nul; /* no writes: the key of the live state (nd_scomp_key takes every field from the walker) */

/* S6SYNC: sync_combine of the op path at a sync unit: the DIP2 skip, else the decision of the live state (key of the walker's inputs read now). */
static void sc_decide(const NdSDef* d, unsigned k) {
    NdCombIn in;
    uint32_t key[ND_SC_KEYW];
    int miss;
    if (sc_ok && sc_sig_now() == sc_sig) return;
    if (__builtin_expect(!sc_tab, 0)) sc_alloc();
    comb_in_walker(&in);
    nd_scomp_key(&sc_nul, &in, (uint32_t)pm_comb_vprim, key);
    key[0] = (uint32_t)(uintptr_t)d + k; /* the def and the sync unit */
    key[26] = nd_ws.omode_h; key[27] = nd_ws.omode_l; key[28] = nd_ws.tst1.tile_fmt[0] * 4u + nd_ws.tst1.tile_siz[0]; key[29] = nd_ws.geom;
    const unsigned i = nd_scomp_find(sc_tab, d->id, key, &miss);
    if (__builtin_expect(miss, 0)) sc_make(&sc_out[i], &in);
    sc_replay(&sc_out[i]);
    sc_ok = 1;
    sc_sig = sc_sig_now();
}

/* S6SYNC: the static units in stream order as the op path runs them: nd_op_same skips the unit, else its write; a sync unit then decides. */
static void sc_units(const NdSDef* d) {
    const uint32_t* u = (const uint32_t*)d->u;
    for (unsigned k = 0; k < d->n; k++) {
        const uint32_t* p = u + 2u * k;
        const unsigned id = p[0] >> 24;
        if (d->vmask >> k & 1u) continue;
        if (id != NDO_OMH && id != NDO_OML && id != NDO_GEOM && id != NDO_TEXTURE && id != NDO_COMB) continue; /* no-ops */
        if (nd_op_same(p)) continue;
        if (!unit_sync(p[0])) { nd_op_apply(p); continue; }
        if (id == NDO_OMH) { const uint32_t m = nd_op_om_mask(p[0] & 0xffffffu); sc_omh(m, p[1]); }
        else if (id == NDO_TEXTURE) nd_set_texture(p[0] & 0xffffffu, p[1]);
        else op_setcombine(NDO_COMB_W0(p[0] & 0xffffffu), p[1]);
        sc_decide(d, k);
    }
}

static void sc_exec(const NdSDef* d, const uint32_t* v) {
    if (d->pre) nd_walk_state(v, d->pre);
    if (d->nsync) sc_units(d); else sc_block(d);
    sr_ok = 0; /* the next draw makes the render state again (as after a range sync that worked) */
    gv_epoch++; /* GV1b */
    pm_gbi_cur_op = d->last_op;
    if (d->nv > d->pre) nd_walk_state(v + 2u * d->pre, d->nv - d->pre);
}

/* ---- s6chk (debug switch): both paths on each record, the state fields compared ---- */
#define S6_FIELDS(X) \
    X(omode_h, nd_ws.omode_h) X(omode_l, nd_ws.omode_l) X(om_kh, nd_ws.om_kh) X(om_kl, nd_ws.om_kl) X(geom, nd_ws.geom) \
    X(tex_on, nd_ws.tex_on) X(tex_ss, nd_ws.tex_ss) X(tex_ts, nd_ws.tex_ts) X(comb_w0, nd_ws.comb_w0) X(comb_w1, nd_ws.comb_w1) \
    X(comb_set, comb_set) X(tst0, *tst_f[0]) X(tst1, *tst_f[1]) X(tst2, *tst_f[2]) X(tst3, *tst_f[3]) X(tst4, *tst_f[4]) \
    X(tst5, *tst_f[5]) X(tst6, *tst_f[6]) X(tst7, *tst_f[7]) X(tst8, *tst_f[8]) X(tst9, *tst_f[9]) X(tst10, *tst_f[10]) \
    X(tst11, *tst_f[11]) X(tst12, *tst_f[12]) X(tst13, *tst_f[13]) X(tst14, *tst_f[14]) X(tst15, *tst_f[15]) X(use_t1, nd_ws.use_t1) \
    X(box_prod, box_prod) X(box_folded, box_folded) X(box_pat, box_pat) X(comb_alpha_shade, comb_alpha_shade) X(cmb_gen_seen, cmb_gen_seen) \
    X(leg_mode, leg_mode) X(comb_mode5, comb_mode5) X(plan_active, plan_active) X(ls_use, ls_use) X(ls_why, ls_why) X(ls_cyc, ls_cyc) \
    X(ls_dual, ls_dual) X(prim, nd_ws.prim) X(envc, nd_ws.envc) X(prim_lod, nd_ws.prim_lod) X(keyc, keyc) X(keys, keys) \
    X(conv_k4, conv_k4) X(dual_on, dual_on) \
    X(fogc, fogc) X(blendc, blendc) X(fog_fm, fog_fm) X(fog_fo, fog_fo) X(prim_z, nd_ws.prim_z) X(fill, nd_ws.fill) \
    X(sci_x0, sci_x0) X(sci_y0, sci_y0) X(sci_x1, sci_x1) X(sci_y1, sci_y1) X(lt_n, lt_n) \
    X(sc_ok, sc_ok)
#define S6_N1(nm, lv) S6F_##nm,
enum { S6_FIELDS(S6_N1) S6F_plan, S6F_lt, S6F_N };
#define S6_NM(nm, lv) #nm,
static const char* const s6_name[S6F_N] = { S6_FIELDS(S6_NM) "cur_plan", "lights" };
typedef struct {
    uint32_t f[S6F_N];
    uint32_t sig, srok, slc, sle, slcc, lsf; /* restored, not compared */
    PmPlan plan;
    NdLight lt[9];
} S6Snap;
static void s6_take(S6Snap* s) {
    unsigned i = 0;
#define S6_TK(nm, lv) s->f[i++] = (uint32_t)(lv);
    S6_FIELDS(S6_TK)
    s->sig = sc_sig; s->srok = (uint32_t)sr_ok; s->slc = (uint32_t)sl_const; s->sle = (uint32_t)sl_env; s->slcc = sl_cc; s->lsf = (uint32_t)ls_frame;
    s->plan = cur_plan;
    memcpy(s->lt, lt, sizeof s->lt);
}
static void s6_put(const S6Snap* s) {
    unsigned i = 0;
#define S6_PT(nm, lv) lv = (__typeof__(lv))s->f[i++];
    S6_FIELDS(S6_PT)
    sc_sig = s->sig; sr_ok = (int)s->srok; sl_const = (int)s->slc; sl_env = (int)s->sle; sl_cc = s->slcc; ls_frame = (int)s->lsf;
    cur_plan = s->plan;
    memcpy(lt, s->lt, sizeof lt);
}
static unsigned s6_ranges, s6_bad, s6_next = 100u, s6_fbad[S6F_N];
static __attribute__((cold, noinline)) void s6_run(const NdSDef* d, const uint32_t* v) {
    static S6Snap s0, a, b;
    static uint32_t u[2u * 32u];
    const uint32_t* t = (const uint32_t*)d->u;
    unsigned j = 0, bad = 0;
    s6_take(&s0);
    sc_exec(d, v);
    s6_take(&a);
    s6_put(&s0);
    for (unsigned k = 0; k < d->n; k++) { /* the op path: the def units with the value units in place (as the old record) */
        if (d->vmask >> k & 1u) { u[2u * k] = v[2u * j]; u[2u * k + 1u] = v[2u * j + 1u]; j++; }
        else { u[2u * k] = t[2u * k]; u[2u * k + 1u] = t[2u * k + 1u]; }
    }
    nd_walk_state(u, d->n);
    s6_take(&b);
    for (unsigned i = 0; i < S6F_plan; i++) if (a.f[i] != b.f[i]) { s6_fbad[i]++; bad = 1; }
    if (a.f[S6F_plan_active] && b.f[S6F_plan_active] && memcmp(&a.plan, &b.plan, sizeof a.plan)) { s6_fbad[S6F_plan]++; bad = 1; }
    if (memcmp(a.lt, b.lt, sizeof a.lt)) { s6_fbad[S6F_lt]++; bad = 1; }
    s6_ranges++;
    if (bad) {
        s6_bad++;
        if (s6_bad <= 8u) pm_log_w("s6chk bad f%u def %p", pm_frame_count(), (const void*)d);
    }
    const unsigned f = pm_frame_count();
    if (f >= s6_next) {
        s6_next = (f / 100u + 1u) * 100u;
        for (unsigned i = 0; i < S6F_N; i++) if (s6_fbad[i]) pm_log_evt("s6chk field %s n %u", s6_name[i], s6_fbad[i]);
        pm_log_evt("s6chk f%u ranges %u bad %u made %u", f, s6_ranges, s6_bad, sc_made);
    }
}

static int sc_chk = -1;
static void sc_go(const NdSDef* d, const uint32_t* v) {
    if (__builtin_expect(sc_chk < 0, 0)) sc_chk = pm_debug_has("s6chk") ? 1 : 0;
    if (__builtin_expect(sc_chk, 0)) s6_run(d, v); else sc_exec(d, v);
}
void nd_scomp_run(const uint32_t* rec) { sc_go((const NdSDef*)(uintptr_t)rec[0], rec + 2); }
int nd_scomp_exec(NdSDef* d, const uint32_t* v, unsigned nv) {
    if (__builtin_expect(!d->st, 0)) d->st = nd_scomp_analyse(d) ? 1u : 2u;
    if (d->st != 1u || nv != d->nv) return 0;
    sc_go(d, v);
    return 1;
}
int nd_scomp_exec_own(NdSDef* d) {
    uint32_t v[2u * 32u]; const uint32_t* u = (const uint32_t*)d->u; unsigned j = 0;
    if (__builtin_expect(!d->st, 0)) d->st = nd_scomp_analyse(d) ? 1u : 2u;
    if (d->st != 1u) return 0;
    for (unsigned k = 0; k < d->n; k++) if (d->vmask >> k & 1u) { v[2u * j] = u[2u * k]; v[2u * j + 1u] = u[2u * k + 1u]; j++; }
    return nd_scomp_exec(d, v, j);
}
#endif
