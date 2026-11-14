/* nd_gtab.c: the lookup of the native list tables (S7CONV, nd_gtab.h). nd_gtab_idx[] is generated (3ds/src/nd_gfx_tab.c) in symbol order;
 * link addresses are not known when it is made, so the first call sorts it by list address (insertion sort, cold, once). Core 1 only: the
 * records carry the table pointer to core 0. */
#include <stdint.h>
#include <string.h>
#include "nd_gtab.h"
#include "nd_op.h"

static int gt_sorted;
static __attribute__((cold, noinline)) void gt_sort(void) {
    for (unsigned i = 1; i < nd_gtab_n; i++) {
        const NdGIdx e = nd_gtab_idx[i];
        unsigned j = i;
        for (; j > 0u && (uintptr_t)nd_gtab_idx[j - 1u].list > (uintptr_t)e.list; j--) nd_gtab_idx[j] = nd_gtab_idx[j - 1u];
        nd_gtab_idx[j] = e;
    }
    gt_sorted = 1;
}
const NdGTab* nd_gtab_find(const void* list) {
    if (__builtin_expect(!gt_sorted, 0)) gt_sort();
    unsigned lo = 0, hi = nd_gtab_n;
    while (lo < hi) {
        const unsigned mid = (lo + hi) >> 1;
        const uintptr_t a = (uintptr_t)nd_gtab_idx[mid].list;
        if (a == (uintptr_t)list) return nd_gtab_idx[mid].tab;
        if (a < (uintptr_t)list) lo = mid + 1u; else hi = mid;
    }
    return 0;
}
/* EMIT4: the flat words of a v2 list from its unit copy (e->u, e->nu): a call unit is the callee's flat words through call, the others copy */
int nd_ntab_flat(const NdNDev* e, void* out, unsigned* n, unsigned max, int depth, int (*call)(uint32_t w1, unsigned* n, int depth)) {
    for (unsigned i = 0; i < e->nu; i++) {
        const uint32_t* u = e->u + 2u * i;
        if ((u[0] >> 24) == NDO_CALL) { if (!call(u[1], n, depth)) return 0; if (u[0] & 1u) return 1; continue; }
        if (*n >= max) return 0;
        memcpy((char*)out + 8u * *n, u, 8u);
        (*n)++;
    }
    return e->nu < max;
}

static unsigned gt_kind(uint32_t w0) { /* as st_kind of nd_rec.c: 0 not a state op, 1 a state op, 2 a sync, 3 the list end */
    const unsigned op = w0 >> 24;
    if (!nd_op_typed(op)) return 0u;
    if (op == NDO_END) return 3u;
    if (op == NDO_MISC) return (w0 & 0xffffffu) == (NDO_MISC_NOP << 16) ? 2u : 0u;
    return (op <= NDO_COMB || nd_op_tex(op) || (op >= NDO_MTX && op <= NDO_MOVEMEM)) ? 1u : 0u;
}
unsigned nd_gt_state_only(const uint32_t* l) {
    for (unsigned n = 0; n < 64u; n++) {
        const unsigned k = gt_kind(l[2u * n]);
        if (k == 3u) return 1u;
        if (!k) return 0u;
    }
    return 0u;
}
/* CONV2D2: nd_ntab_find (nd_gtab.h), core 0 only (nd_gtab_run). The same lookup as nd_gtab_find over the device index ndn_dev_idx. */
static int nt_sorted;
static __attribute__((cold, noinline)) void nt_sort(void) {
    for (unsigned i = 1; i < ndn_dev_n; i++) {
        const NdNDev* e = ndn_dev_idx[i];
        unsigned j = i;
        for (; j > 0u && (uintptr_t)ndn_dev_idx[j - 1u]->list > (uintptr_t)e->list; j--) ndn_dev_idx[j] = ndn_dev_idx[j - 1u];
        ndn_dev_idx[j] = e;
    }
    nt_sorted = 1;
}
const NdNDev* nd_ntab_find(const void* list) {
    if (__builtin_expect(!nt_sorted, 0)) nt_sort();
    unsigned lo = 0, hi = ndn_dev_n;
    while (lo < hi) {
        const unsigned mid = (lo + hi) >> 1;
        const uintptr_t a = (uintptr_t)ndn_dev_idx[mid]->list;
        if (a == (uintptr_t)list) return ndn_dev_idx[mid];
        if (a < (uintptr_t)list) lo = mid + 1u; else hi = mid;
    }
    return 0;
}
void nd_tab_init(void) { if (!gt_sorted) gt_sort(); if (!nt_sorted) nt_sort(); } /* GKEY2 calls it before the logic thread starts */
#ifndef ND_SCOMP_HOST
/* S7RUN: nd_rec_dl (nd_gtab.h). Core 1. A list that rec_inline (nd_rec.c) records in line (to its G_ENDDL fewer than 64 units, each a state op) is
 * flagged state-only: the runner then does what G_PM_LST and the state run did, with no ent_gdl or dlc_try call. Memo of the last 8 lookups. */
#include "ultra64.h"
#include "variables.h"
#include "pm_port.h"
#include "nd.h"
#include "nd_cmd.h"
#include "nd_op.h"
#include "gbi_custom.h"
#include "nd_api.h"


extern unsigned nd_gtr_ent, nd_gtr_dlc, nd_gtr_ops; /* nd_walk.c, core 0: the arms nd_gtab_run took */
extern unsigned nd_gtr_ntab; /* nd_walk.c, core 0: the runs of nd_ntab_run */
extern unsigned nd_gtr_rec; /* GKEY2: the runs of nd_ntab_rec */
static unsigned gtr_tab, gtr_dl, gtr_pt, gtr_pd, gtr_pe, gtr_pc, gtr_po, gtr_pn, gtr_pr, gtr_next = 300u, gtr_seen;
static __attribute__((cold, noinline)) void gtr_cen(unsigned f) {
    const unsigned e = nd_gtr_ent, c = nd_gtr_dlc, o = nd_gtr_ops;
    if (gtr_tab != gtr_pt || gtr_dl != gtr_pd) pm_log_evt("gtabrun f%u tab %u dl %u arm %u/%u/%u", f, gtr_tab - gtr_pt, gtr_dl - gtr_pd, e - gtr_pe, c - gtr_pc, o - gtr_po);
    if (gtr_tab != gtr_pt || gtr_dl != gtr_pd) pm_log_evt("ntabrun f%u runs %u rec %u", f, nd_gtr_ntab - gtr_pn, nd_gtr_rec - gtr_pr);
    gtr_pt = gtr_tab; gtr_pd = gtr_dl; gtr_pe = e; gtr_pc = c; gtr_po = o; gtr_pn = nd_gtr_ntab; gtr_pr = nd_gtr_rec;
    gtr_next = (f / 300u + 1u) * 300u;
}
static __attribute__((cold, noinline)) void entcap_nt(const void* l, uint32_t site) { static uint32_t k[256]; static unsigned n;
    const uint32_t a = (uint32_t)(uintptr_t)l; for (unsigned i = 0; i < n; i++) if (k[i] == (a ^ site * 2654435761u)) return; if (n == 256u) return;
    k[n++] = a ^ site * 2654435761u; pm_log_evt("entcap nt l %08x s %08x", a, site); }
static const NdPsoDesc* dl_ent; /* ENTRY1: the entry of the nd_rec_dl_e call in progress (null: none) */
static uint32_t dl_site; /* FXENT3: site | (k + 1) << 28 of nd_rec_dl_key, 0: none */
void nd_rec_dl(const void* list) {
    static const void* memo_l[8];
    static const void* memo_t[8]; /* GKEY2: NdNDev* when memo_f bit 1, else NdGTab* */
    static unsigned memo_f[8], mi;
    const unsigned f = pm_frame_count();
    if (__builtin_expect(!gtr_seen, 0)) { gtr_seen = 1; gtr_next = f + 3u; } /* the first caller: one early line (a scene shorter than 300 frames) */
    if (__builtin_expect(f >= gtr_next, 0)) gtr_cen(f);
    const void* t = NULL;
    unsigned fl = 0, k;
    for (k = 0; k < 8u; k++) if (memo_l[k] == list) { t = memo_t[k]; fl = memo_f[k]; break; }
    if (k == 8u) {
        const NdNDev* nt = nd_ntab_find(list); /* GKEY2: a list with a device entry runs from the entry */
        if (nt) { t = nt; fl = nt->fl | 2u; }
        else { const NdGTab* g = nd_gtab_find(list); if (g) { t = g; fl = nd_gt_state_only((const uint32_t*)g->list); } }
        if (t) { memo_l[mi] = list; memo_t[mi] = t; memo_f[mi] = fl; mi = (mi + 1u) & 7u; }
    }
    if (!t) {
        if (__builtin_expect(PMDL("entcap"), 0)) entcap_nt(list, (uint32_t)(uintptr_t)__builtin_return_address(0));
        gtr_dl++;
        gSPDisplayList(gMainGfxPos++, list);
        return;
    }
    gtr_tab++;
    const int v2 = (fl & 2u) != 0; fl &= 1u;
    if (dl_ent || __builtin_expect(PMDL("entcap"), 0)) { const uint32_t r4[4] = { (uint32_t)(uintptr_t)t, fl, (dl_site ? dl_site : (uint32_t)(uintptr_t)__builtin_return_address(0)), (uint32_t)(uintptr_t)dl_ent }; nd_rec_napi(v2 ? (dl_ent ? nd_ntab_rec_e : nd_ntab_rec) : (dl_ent ? nd_gtab_run_e : nd_gtab_run), r4, 2u); return; }
    const uint32_t rec[2] = { (uint32_t)(uintptr_t)t, fl };
    nd_rec_napi(v2 ? nd_ntab_rec : nd_gtab_run, rec, 1u);
}
void nd_rec_dl_e(const void* list, const NdPsoDesc* e) { dl_ent = e; nd_rec_dl(list); dl_ent = NULL; }
void nd_rec_dl_key(const void* list, unsigned k) { dl_site = (uint32_t)(uintptr_t)__builtin_return_address(0) | ((k & 15u) + 1u) << 28; nd_rec_dl(list); dl_site = 0; }
#endif
