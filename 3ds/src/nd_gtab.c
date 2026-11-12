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
