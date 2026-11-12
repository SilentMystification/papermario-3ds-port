/* nd_gtab.c: the lookup of the native list tables (S7CONV, nd_gtab.h). nd_gtab_idx[] is generated (3ds/src/nd_gfx_tab.c) in symbol order;
 * link addresses are not known when it is made, so the first call sorts it by list address (insertion sort, cold, once). Core 1 only: the
 * records carry the table pointer to core 0. */
#include <stdint.h>
#include "nd_gtab.h"

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
static unsigned gtr_tab, gtr_dl, gtr_pt, gtr_pd, gtr_pe, gtr_pc, gtr_po, gtr_next = 300u, gtr_seen;
static __attribute__((cold, noinline)) void gtr_cen(unsigned f) {
    const unsigned e = nd_gtr_ent, c = nd_gtr_dlc, o = nd_gtr_ops;
    if (gtr_tab != gtr_pt || gtr_dl != gtr_pd) pm_log_evt("gtabrun f%u tab %u dl %u arm %u/%u/%u", f, gtr_tab - gtr_pt, gtr_dl - gtr_pd, e - gtr_pe, c - gtr_pc, o - gtr_po);
    gtr_pt = gtr_tab; gtr_pd = gtr_dl; gtr_pe = e; gtr_pc = c; gtr_po = o;
    gtr_next = (f / 300u + 1u) * 300u;
}
static unsigned gt_kind(uint32_t w0) { /* as st_kind of nd_rec.c: 0 not a state op, 1 a state op, 2 a sync, 3 the list end */
    const unsigned op = w0 >> 24;
    if (!nd_op_typed(op)) return 0u;
    if (op == NDO_END) return 3u;
    if (op == NDO_MISC) return (w0 & 0xffffffu) == (NDO_MISC_NOP << 16) ? 2u : 0u;
    return (op <= NDO_COMB || nd_op_tex(op) || (op >= NDO_MTX && op <= NDO_MOVEMEM)) ? 1u : 0u;
}
static unsigned gt_state_only(const uint32_t* l) {
    for (unsigned n = 0; n < 64u; n++) {
        const unsigned k = gt_kind(l[2u * n]);
        if (k == 3u) return 1u;
        if (!k) return 0u;
    }
    return 0u;
}
void nd_rec_dl(const void* list) {
    static const void* memo_l[8];
    static const NdGTab* memo_t[8];
    static unsigned memo_f[8], mi;
    const unsigned f = pm_frame_count();
    if (__builtin_expect(!gtr_seen, 0)) { gtr_seen = 1; gtr_next = f + 3u; } /* the first caller: one early line (a scene shorter than 300 frames) */
    if (__builtin_expect(f >= gtr_next, 0)) gtr_cen(f);
    const NdGTab* t = NULL;
    unsigned fl = 0, k;
    for (k = 0; k < 8u; k++) if (memo_l[k] == list) { t = memo_t[k]; fl = memo_f[k]; break; }
    if (k == 8u) {
        t = nd_gtab_find(list);
        if (t) { fl = gt_state_only((const uint32_t*)t->list); memo_l[mi] = list; memo_t[mi] = t; memo_f[mi] = fl; mi = (mi + 1u) & 7u; }
    }
    if (!t) {
        gtr_dl++;
        gSPDisplayList(gMainGfxPos++, list);
        return;
    }
    gtr_tab++;
    const uint32_t rec[2] = { (uint32_t)(uintptr_t)t, fl };
    nd_rec_napi(nd_gtab_run, rec, 1u);
}
#endif
