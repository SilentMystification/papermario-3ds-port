/* nd_scomp.c: the state compiler (NATIVEAPI S6T, nd_scomp.h; design note: git show b16f91c46:docs/handoff/row_S6T.md). A def (a producer's
 * range of typed state units, static data) is analysed once on core 1 into its folded writes; on core 0 a record runs its units as ops in def order, then the cut of the old run (DEL3B).
 * Host test: nd_scomp_test.c builds the pure part (ND_SCOMP_HOST). */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_op.h"
#include "nd_fs.h"
#include "nd_scomp.h"

static unsigned def_next; /* core 1: the ids of the analysed defs (a hint only: last[]; the key word 0 is the def pointer) */

/* The class of one unit: 0 refused, 1 static, 2 value, 3 no-op (dropped), 4 ordered value (SCOMP0: PRIM, ENV, texture setters; must not lie between two static units; class 2 too must not lie between two static units (S6VORD: sc_exec runs it after the block)). */
static unsigned unit_class(uint32_t w0) {
    const unsigned id = w0 >> 24, a = w0 & 0xffffffu;
    if (w0 == ND_NOP_A) return 3u;
    switch (id) {
    case ND_FS_OMH: case ND_FS_OML: case ND_FS_GEOM: case ND_FS_TEXTURE: case ND_FS_COMB: return 1u;
    case ND_FS_FOG: case ND_FS_BLEND: case ND_FS_FILL: case ND_FS_PRIMZ: case ND_FS_SCISSOR: case ND_FS_MTX: case ND_FS_POPMTX: case ND_FS_MOVEMEM: return 2u;
    case ND_FS_MW: return (a >> 16) == ND_FS_MW_NONE ? 3u : 2u; /* S6VORD: clip ratio, persp normalize: no field, a no-op */
    case ND_FS_MISC: return ((a >> 16) == ND_FS_MISC_CIMG || (a >> 16) == ND_FS_MISC_ND) ? 2u : 0u;
    case ND_FS_PRIM: case ND_FS_ENV: case ND_FS_TIMG: case ND_FS_TILE: case ND_FS_TILESIZE:
    case ND_FS_LOADBLOCK: case ND_FS_LOADTILE: case ND_FS_LOADTLUT: return 4u; /* SCOMP0: an ordered value unit */
    default: return 0u; /* RARE, draws and control */
    }
}

/* S6SYNC: 1 when the op path runs nd_comb_sync on this static unit (set_om over the cycle bits, nd_unit_apply TEXTURE and COMB). */
static int unit_sync(uint32_t w0) {
    const unsigned a = w0 & 0xffffffu;
    switch (w0 >> 24) {
    case ND_FS_OMH: { const unsigned sft = a & 0xffu, len = (a >> 8) & 0xffu; return sft <= G_MDSFT_CYCLETYPE && sft + len > G_MDSFT_CYCLETYPE; }
    case ND_FS_TEXTURE: case ND_FS_COMB: return 1;
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
        d->last_op = (uint8_t)id; /* DEL1D1: the unit id */
        if (unit_sync(w0)) { if (d->nsync >= 4u) ok = 0; else d->sync_k[d->nsync++] = (uint8_t)k; }
        switch (id) {
        case ND_FS_OMH: { const uint32_t m = nd_op_om_mask(a); d->omh_v = (d->omh_v & ~m) | (b & m); d->omh_m |= m; break; }
        case ND_FS_OML: { const uint32_t m = nd_op_om_mask(a); d->oml_v = (d->oml_v & ~m) | (b & m); d->oml_m |= m; break; }
        case ND_FS_GEOM: d->geom_c &= a; d->geom_s = (d->geom_s & a) | b; d->has_geom = 1; break; /* set_geom: geom = (geom & a) | b, composed */
        case ND_FS_TEXTURE: d->tex_a = a; d->tex_b = b; d->has_tex = 1; break;
        case ND_FS_COMB: d->cw0 = ND_FS_COMB_W0(a); d->cw1 = b; d->has_comb = 1; break;
        default: break; /* MW none */
        }
    }
    if (!seen) ok = 0; /* only value units: nothing to compile */
    for (unsigned k = 0; k < 32u; k++) if ((om >> k & 1u) && k > first && k < last) ok = 0; /* between two static units */
    if (ok) d->id = (uint8_t)__atomic_fetch_add(&def_next, 1u, __ATOMIC_RELAXED); /* a hint only (last[]) */
    return ok;
}

/* The key: the def and the combine decision inputs after the def's writes (nd_comb_dec NdCombIn and pm_comb_vprim). A field the def sets
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
#include "nd_ds.h" /* NST2C: the core 1 state of a compiled def */
#include "pm_gpu.h"
#include "pm_port.h"

void nd_rec_nr(uint32_t w0, uint32_t w1);           /* nd_rec.c: one typed unit into the open state run */

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
        nd_rec_nr(w0, w1);
    }
}
void nd_scomp_rec(NdSDef* d, const void* vv, unsigned nv) {
    const uint32_t* v = (const uint32_t*)vv;
    if (!d->st) d->st = nd_scomp_analyse(d) ? 1u : 2u;
    if (d->st != 1u || nv != d->nv || nv > 32u) { rec_ops(d, v, nv); return; }
    for (unsigned j = 0; j < d->pre; j++) if (v[2u * j] != ND_NOP_A) nd_rec_nr(v[2u * j], v[2u * j + 1u]); /* NAT5C: the order of the old nd_scomp_run: the pre value units */
    { const uint32_t* t = (const uint32_t*)d->u; for (unsigned k = 0; k < d->n; k++) if (!(d->vmask >> k & 1u) && t[2u * k] != ND_NOP_A) nd_rec_nr(t[2u * k], t[2u * k + 1u]); } /* NAT5C: the static units */
    { const uint32_t c[2] = { (uint32_t)d->last_op, 0u }; nd_rec_napi(nd_scut_run, c, 1u); } /* NAT5C: the cut and the render resync of the old run (a command, as the old record) */
    for (unsigned j = d->pre; j < nv; j++) if (v[2u * j] != ND_NOP_A) nd_rec_nr(v[2u * j], v[2u * j + 1u]); /* NAT5C: the post value units */
    { static unsigned char seen; if (!seen) { seen = 1; pm_log_evt("scomp rec f%u", pm_frame_count()); } } /* DSDUP: a compiled record was taken */
}

/* ---- core 0: the run ---- */
/* DEL3B: a compiled record in the order of the old sc_exec: the pre value units, the static and no-op units of d in def order (nd_walk_state
 * skips a no-op), the cut and the render resync, the post value units. The static units take the walker decision (nd_comb_sync), as a list def (NREC5). */
void nd_scut_run(const uint32_t* r) { sr_ok = 0; gv_epoch++; pm_gbi_cur_op = r[0]; } /* NAT5C: the cut and the render resync of the old compiled run (DEL3B) */
/* NREC5, core 0: 1 when the compiler takes d (st 1). The callers run the units of d and keep the cut and the render resync of the old compiled run (sc_exec). */
int nd_scomp_ok(NdSDef* d) {
    if (__builtin_expect(!d->st, 0)) d->st = nd_scomp_analyse(d) ? 1u : 2u;
    return d->st == 1u;
}
#endif
