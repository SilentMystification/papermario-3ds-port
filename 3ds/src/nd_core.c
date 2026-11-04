/* nd_core.c: native draw record array, frame driver and the LEGACY bridge (plan_native_slices S1-2, PNS 6). */
#include <stdint.h>
#include <string.h>
#include "nd.h"
#include "pm_gbi.h"

extern int pm_ph_on;                                                  /* pm_port.h */
void pm_pf_nd_add(unsigned long long ticks, unsigned legacy);         /* pm_phase.c: S0-6 window sums */
uint64_t svcGetSystemTick(void);

static NdRec nd_rec[ND_REC_MAX] __attribute__((aligned(32)));
static unsigned nd_n;

extern int pm_debug_has(const char* key);
extern void pm_log_d(const char* fmt, ...);
extern unsigned pm_frame_count(void);
/* debug switch ndst: the native state of the bridge (nd_st, S2) once per 100 frames, then the counts restart */
static __attribute__((cold, noinline)) void nd_st_log(void) {
    static int on = -1;
    if (on < 0) on = pm_debug_has("ndst");
    if (!on || pm_frame_count() % 100u) return;
    pm_log_d("ndst f%u geom %08x omh %08x oml %08x comb %08x %08x prim %08x scis %u %u %u %u mtx %u pop %u fill %u rect %u dl %u end %u load %u unk %u", pm_frame_count(), nd_st.geom, nd_st.om_h, nd_st.om_l,
             nd_st.comb_w0, nd_st.comb_w1, nd_st.prim, nd_st.sc_ulx, nd_st.sc_uly, nd_st.sc_lrx, nd_st.sc_lry, nd_st.n_mtx, nd_st.n_pop, nd_st.n_fill, nd_st.n_rect, nd_st.n_dl, nd_st.n_enddl, nd_st.n_load, nd_st.n_unknown);
    nd_st.n_mtx = nd_st.n_pop = nd_st.n_fill = nd_st.n_rect = nd_st.n_dl = nd_st.n_enddl = nd_st.n_load = nd_st.n_unknown = 0;
}
void nd_frame_begin(void) { nd_n = 0; nd_st_log(); }

/* cold: the walker runs one LEGACY span */
static __attribute__((cold, noinline)) void nd_legacy(void* list, unsigned nbytes) {
    pm_gbi_run(list, nbytes);
}

void nd_bridge_cut(void* list, unsigned nbytes) {
    /* ponytail: a full array runs the span at once, out of order. Add a split when producers fill it. */
    if (nd_n >= ND_REC_MAX) { nd_legacy(list, nbytes); return; }
    NdRec* r = &nd_rec[nd_n++];
    memset(r, 0, sizeof *r);
    r->kind = ND_LEGACY;
    r->aux = (uint32_t)(uintptr_t)list;
    r->aux2 = nbytes;
}

void nd_emit(void) {
    unsigned legacy = 0;
    uint64_t t0 = pm_ph_on ? svcGetSystemTick() : 0;
    for (unsigned i = 0; i < nd_n; i++) {
        const NdRec* r = &nd_rec[i];
        if (r->kind == ND_LEGACY) { nd_legacy((void*)(uintptr_t)r->aux, r->aux2); legacy++; }
    }
    if (pm_ph_on) pm_pf_nd_add(svcGetSystemTick() - t0, legacy);
}

/* One LSD pass: stable counting sort of in[] by tex0 (by_tex) or by tmpl. Keys are u8. */
static void nd_pass(const NdRec* rec, const uint16_t* in, uint16_t* out, unsigned len, int by_tex) {
    unsigned cnt[257];
    memset(cnt, 0, sizeof cnt);
    for (unsigned k = 0; k < len; k++) {
        const NdRec* r = &rec[in[k]];
        cnt[(by_tex ? r->tex0 : r->tmpl) + 1u]++;
    }
    for (unsigned b = 0; b < 256; b++) cnt[b + 1] += cnt[b];
    for (unsigned k = 0; k < len; k++) {
        const NdRec* r = &rec[in[k]];
        out[cnt[by_tex ? r->tex0 : r->tmpl]++] = in[k];
    }
}

unsigned nd_order(const NdRec* rec, unsigned n, uint16_t* ord) {
    uint16_t tmp[ND_REC_MAX];
    unsigned i = 0;
    while (i < n) {
        if (rec[i].seq & ND_SEQ_ORDERED) { ord[i] = (uint16_t)i; i++; continue; }
        unsigned j = i;
        while (j < n && !(rec[j].seq & ND_SEQ_ORDERED)) j++;
        /* run [i, j): index list in game order, then the tex0 pass, then the tmpl pass (LSD, stable) */
        for (unsigned k = i; k < j; k++) ord[k] = (uint16_t)k;
        nd_pass(rec, ord + i, tmp, j - i, 1);
        nd_pass(rec, tmp, ord + i, j - i, 0);
        i = j;
    }
    return n;
}

void nd_eye_copy(uint32_t* dst, const uint32_t* src, unsigned nw, const NdMtxLog* log, unsigned nlog, unsigned eye) {
    memcpy(dst, src, nw * sizeof(uint32_t));
    for (unsigned k = 0; k < nlog; k++) memcpy(dst + log[k].at, nd_mtx_pool[eye][log[k].slot], 16 * sizeof(uint32_t));
}
