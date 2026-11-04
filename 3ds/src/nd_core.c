/* nd_core.c: native draw record array, frame driver and the LEGACY bridge (plan_native_slices S1-2, PNS 6). */
#include <stdint.h>
#include "nd.h"
#include "pm_gbi.h"

extern int pm_ph_on;                                                  /* pm_port.h */
void pm_pf_nd_add(unsigned long long ticks, unsigned legacy);         /* pm_phase.c: S0-6 window sums */
uint64_t svcGetSystemTick(void);

#define ND_REC_MAX 2048u
static NdRec nd_rec[ND_REC_MAX] __attribute__((aligned(32)));
static unsigned nd_n;

void nd_frame_begin(void) { nd_n = 0; }

/* cold: the walker runs one LEGACY span */
static __attribute__((cold, noinline)) void nd_legacy(void* list, unsigned nbytes) {
    pm_gbi_run(list, nbytes);
}

void nd_bridge_cut(void* list, unsigned nbytes) {
    /* ponytail: a full array runs the span at once, out of order. Add a split when producers fill it. */
    if (nd_n >= ND_REC_MAX) { nd_legacy(list, nbytes); return; }
    NdRec* r = &nd_rec[nd_n++];
    r->kind = ND_LEGACY;
    r->a = (uint32_t)(uintptr_t)list;
    r->b = nbytes;
}

void nd_emit(void) {
    unsigned legacy = 0;
    uint64_t t0 = pm_ph_on ? svcGetSystemTick() : 0;
    for (unsigned i = 0; i < nd_n; i++) {
        const NdRec* r = &nd_rec[i];
        if (r->kind == ND_LEGACY) { nd_legacy((void*)(uintptr_t)r->a, r->b); legacy++; }
    }
    if (pm_ph_on) pm_pf_nd_add(svcGetSystemTick() - t0, legacy);
}
