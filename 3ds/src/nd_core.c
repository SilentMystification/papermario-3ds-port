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

extern unsigned nd_native_draws, nd_native_refused;
extern int pm_debug_has(const char* key);
extern void pm_log_d(const char* fmt, ...);
extern unsigned pm_frame_count(void);
/* debug switch ndst: the native state (nd_ws) once per 100 frames */
extern unsigned nd_rect_tex_n, nd_rect_fill_n; /* nd_rect.c: handler calls (CORE-3 census) */
static __attribute__((cold, noinline)) void nd_st_log(void) {
    static int on = -1;
    if (on < 0) on = pm_debug_has("ndst");
    if (!on || pm_frame_count() % 100u) return;
    pm_log_d("ndst native %u refused %u f%u geom %08x omh %08x oml %08x kh %08x kl %08x comb %08x %08x prim %08x lod %04x rect tex %u fill %u", nd_native_draws, nd_native_refused, pm_frame_count(), nd_ws.geom, nd_ws.omode_h, nd_ws.omode_l,
             nd_ws.om_kh, nd_ws.om_kl, nd_ws.comb_w0, nd_ws.comb_w1, nd_ws.prim, nd_ws.prim_lod, nd_rect_tex_n, nd_rect_fill_n);
    nd_native_draws = nd_native_refused = 0;
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

/* S3G-4: the sort key field of an LSD pass: 0 tex1, 1 tex0, 2 tmpl. */
static uint8_t nd_field(const NdRec* r, int f) {
    return f == 0 ? r->tex1 : f == 1 ? r->tex0 : r->tmpl;
}
/* One LSD pass: stable counting sort of in[] by one key field (nd_field). Keys are u8. */
static void nd_pass(const NdRec* rec, const uint16_t* in, uint16_t* out, unsigned len, int f) {
    unsigned cnt[257];
    memset(cnt, 0, sizeof cnt);
    for (unsigned k = 0; k < len; k++) {
        const NdRec* r = &rec[in[k]];
        cnt[nd_field(r, f) + 1u]++;
    }
    for (unsigned b = 0; b < 256; b++) cnt[b + 1] += cnt[b];
    for (unsigned k = 0; k < len; k++) {
        const NdRec* r = &rec[in[k]];
        out[cnt[nd_field(r, f)]++] = in[k];
    }
}

unsigned nd_order(const NdRec* rec, unsigned n, uint16_t* ord) {
    uint16_t tmp[ND_REC_MAX];
    unsigned i = 0;
    while (i < n) {
        if (rec[i].seq & ND_SEQ_ORDERED) { ord[i] = (uint16_t)i; i++; continue; }
        unsigned j = i;
        while (j < n && !(rec[j].seq & ND_SEQ_ORDERED)) j++;
        /* run [i, j): index list in game order, then three stable passes (LSD): tex1, tex0, tmpl = the order by (tmpl, tex0, tex1) */
        for (unsigned k = i; k < j; k++) ord[k] = (uint16_t)k;
        nd_pass(rec, ord + i, tmp, j - i, 0);
        nd_pass(rec, tmp, ord + i, j - i, 1);
        nd_pass(rec, ord + i, tmp, j - i, 2);
        memcpy(ord + i, tmp, (j - i) * sizeof *ord);
        i = j;
    }
    return n;
}

void nd_eye_copy(uint32_t* dst, const uint32_t* src, unsigned nw, const NdMtxLog* log, unsigned nlog, unsigned eye) {
    memcpy(dst, src, nw * sizeof(uint32_t));
    for (unsigned k = 0; k < nlog; k++) memcpy(dst + log[k].at, nd_mtx_pool[eye][log[k].slot], 16 * sizeof(uint32_t));
}
