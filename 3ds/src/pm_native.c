/* NDS1 phase A: see pm_native.h. Pure C, no citro3d. The emit loop is the hot part (the hot section is set in phase B).
 * ponytail: template lookup is a linear scan of 64 entries. Runs at build time only. Upgrade to a hash if producers look up per draw. */
#include <string.h>
#include "pm_native.h"

NatTmpl nat_tmpl[NAT_TMPL_MAX];
uint32_t nat_drw[NAT_DRAW_WORDS] __attribute__((aligned(32)));
uint32_t nat_mtx[2][NAT_MTX_MAX][16] __attribute__((aligned(32)));
uint32_t nat_tex_word[NAT_TEX_MAX];
uint32_t nat_idx_base;

static NatPatch nat_pt[NAT_PAT_MAX];
static unsigned nat_npt;

void nat_layout_set(const NatPatch* pt, unsigned n, const uint32_t* drw) {
    nat_npt = n > NAT_PAT_MAX ? NAT_PAT_MAX : n;
    memcpy(nat_pt, pt, nat_npt * sizeof nat_pt[0]);
    memcpy(nat_drw, drw, sizeof nat_drw);
}

/* pm_gbi.c:504 and :1070: two = the cycle type is 2 CYCLE (1 << G_MDSFT_CYCLETYPE in omode_h, include/PR/gbi.h:579); t1only = the bound
 * texture is TEXEL1 (pm_gbi.c:632, 636). */
#define NAT_MDSFT_CYCLETYPE 20
static int nat_pm_comb(uint32_t cw0, uint32_t cw1, uint32_t oh, int t1only, PmPlan* p) {
    int two = ((oh >> NAT_MDSFT_CYCLETYPE) & 3) == 1;
    return pm_comb_build(cw0, cw1, two, t1only, p);
}

int nat_tmpl_add(uint32_t cw0, uint32_t cw1, uint32_t geom, uint32_t oh, uint32_t ol, int z, int decal, int blend, int atest,
    int t1only, PmPlan* plan_out, int* rc_out) {
    int id = -1;
    for (int i = 0; i < NAT_TMPL_MAX; i++) {
        NatTmpl* t = &nat_tmpl[i];
        if (t->used) {
            if (t->cw0 == cw0 && t->cw1 == cw1 && t->geom == geom && t->oh == oh && t->ol == ol && t->z == !!z &&
                t->decal == !!decal && t->blend == !!blend && t->atest == !!atest && t->t1only == !!t1only) {
                if (plan_out || rc_out) {
                    PmPlan p; int rc = nat_pm_comb(cw0, cw1, oh, t1only, &p);
                    if (plan_out) *plan_out = p;
                    if (rc_out) *rc_out = rc;
                }
                return i;
            }
        } else if (id < 0) id = i;
    }
    if (id < 0) return -1;
    NatTmpl* t = &nat_tmpl[id];
    PmPlan p; int rc = nat_pm_comb(cw0, cw1, oh, t1only, &p);
    if (plan_out) *plan_out = p;
    if (rc_out) *rc_out = rc;
    memset(t, 0, sizeof *t);
    t->cw0 = cw0; t->cw1 = cw1; t->geom = geom; t->oh = oh; t->ol = ol;
    t->z = !!z; t->decal = !!decal; t->blend = !!blend; t->atest = !!atest; t->t1only = !!t1only; t->oh = oh;
    /* Opaque depth-tested draws may be reordered. Blend, decal and 2D (z off) keep game order. Alpha test writes depth, so it may be reordered. */
    t->ordered = (blend || decal || !z) ? 1 : 0;
    t->used = 1;
    return id;
}

/* One LSD pass: stable counting sort of in[] by tex0 (by_tex) or by tmpl. Keys: tex0 < NAT_TEX_MAX, tmpl < NAT_TMPL_MAX. */
static void nat_pass(const NatRec* rec, const uint16_t* in, uint16_t* out, unsigned len, int by_tex) {
    unsigned cnt[NAT_TEX_MAX + 1];
    unsigned nb = by_tex ? NAT_TEX_MAX : NAT_TMPL_MAX;
    memset(cnt, 0, (nb + 1) * sizeof cnt[0]);
    for (unsigned k = 0; k < len; k++) {
        const NatRec* r = &rec[in[k]];
        cnt[(by_tex ? r->tex0 : r->tmpl) + 1]++;
    }
    for (unsigned b = 0; b < nb; b++) cnt[b + 1] += cnt[b];
    for (unsigned k = 0; k < len; k++) {
        const NatRec* r = &rec[in[k]];
        out[cnt[by_tex ? r->tex0 : r->tmpl]++] = in[k];
    }
}

unsigned nat_order(const NatRec* rec, unsigned n, uint16_t* ord) {
    uint16_t tmp[NAT_REC_MAX];
    unsigned i = 0;
    while (i < n) {
        if (nat_tmpl[rec[i].tmpl].ordered) { ord[i] = (uint16_t)i; i++; continue; }
        unsigned j = i;
        while (j < n && !nat_tmpl[rec[j].tmpl].ordered) j++;
        /* run [i, j): index list in game order, then tex0 pass, then tmpl pass (LSD, stable) */
        for (unsigned k = i; k < j; k++) ord[k] = (uint16_t)k;
        nat_pass(rec, ord + i, tmp, j - i, 1);
        nat_pass(rec, tmp, ord + i, j - i, 0);
        i = j;
    }
    return n;
}

static void nat_patch(uint32_t* d, const NatPatch* q, const NatRec* r, unsigned eye, uint32_t at, NatMtxLog* log, unsigned* nl) {
    switch (q->kind) {
    case NAT_P_TEX0: d[q->off] = nat_tex_word[r->tex0]; break;
    case NAT_P_TEX1: d[q->off] = nat_tex_word[r->tex1]; break;
    case NAT_P_IDX:  d[q->off] = nat_idx_base + r->idx_off; break;
    case NAT_P_NIDX: d[q->off] = r->nidx; break;
    case NAT_P_MTX:
        memcpy(d + q->off, nat_mtx[eye][r->mtx], 16 * sizeof(uint32_t));
        log[*nl].at = at + q->off; log[*nl].slot = r->mtx; (*nl)++;
        break;
    }
}

unsigned nat_emit(const NatRec* rec, unsigned n, const uint16_t* ord, uint32_t* out, unsigned cap, unsigned eye,
    NatMtxLog* log, unsigned* nlog) {
    unsigned o = 0, nl = 0;
    int cur = -1;
    for (unsigned k = 0; k < n; k++) {
        const NatRec* r = &rec[ord[k]];
        if (r->tmpl != cur) {
            if (o + NAT_TMPL_WORDS > cap) return 0;
            memcpy(out + o, nat_tmpl[r->tmpl].blk, sizeof nat_tmpl[0].blk);
            o += NAT_TMPL_WORDS;
            cur = r->tmpl;
        }
        if (o + NAT_DRAW_WORDS > cap) return 0;
        uint32_t* d = out + o;
        memcpy(d, nat_drw, sizeof nat_drw);
        for (unsigned p = 0; p < nat_npt; p++) nat_patch(d, &nat_pt[p], r, eye, o, log, &nl);
        o += NAT_DRAW_WORDS;
    }
    *nlog = nl;
    return o;
}

void nat_eye_copy(uint32_t* dst, const uint32_t* src, unsigned nw, const NatMtxLog* log, unsigned nlog, unsigned eye) {
    memcpy(dst, src, nw * sizeof(uint32_t));
    for (unsigned k = 0; k < nlog; k++) memcpy(dst + log[k].at, nat_mtx[eye][log[k].slot], 16 * sizeof(uint32_t));
}
