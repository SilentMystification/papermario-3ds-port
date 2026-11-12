/* HC0 census (PM_FNCOV lane only, tools). The draw TUs are built with -finstrument-functions, so every function entry calls
 * __cyg_profile_func_enter. While the census is armed, the calls are counted per function address in a hash table.
 * Switch fncov=<frame>: armed at the first walker entry of frame >= <frame>; reported at the first walker entry of a later
 * frame as "fncov f<n> <addr>:<calls> ..." (4 per line, nonzero only), then "fncov f<n> done". hotset.py --fncov joins the
 * addresses with the map. The hooks are no_instrument_function, so they never count themselves. */
#include <string.h>
#include <stdlib.h>
#include "pm_port.h"

#define FN_N 4096u /* slots, a power of two: 32 KB of bss in the census lane only */
typedef struct { unsigned addr, n; } FnEnt;
static FnEnt fn_tab[FN_N];
static int fn_init, fn_on, fn_active, fn_done;
static unsigned fn_want, fn_f0;

static FnEnt* fn_slot(unsigned a) __attribute__((no_instrument_function));
static FnEnt* fn_slot(unsigned a) {
    unsigned h = (a * 2654435761u) >> 20; /* the top 12 bits of the multiplicative hash */
    for (unsigned k = 0; k < FN_N; k++) {
        FnEnt* e = &fn_tab[(h + k) & (FN_N - 1u)];
        if (e->addr == a) return e;
        if (!e->addr) { e->addr = a; return e; }
    }
    return 0;
}

void __cyg_profile_func_enter(void* self, void* call_site) __attribute__((no_instrument_function));
void __cyg_profile_func_enter(void* self, void* call_site) {
    FnEnt* e;
    (void)call_site;
    if (!fn_active) return;
    e = fn_slot((unsigned)self);
    if (e) e->n++;
}
void __cyg_profile_func_exit(void* self, void* call_site) __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void* self, void* call_site) { (void)self; (void)call_site; }

static void fn_dump(unsigned f) __attribute__((no_instrument_function));
static void fn_dump(unsigned f) {
    unsigned a[4], c[4], k = 0;
    for (unsigned i = 0; i < FN_N; i++) {
        if (!fn_tab[i].addr || !fn_tab[i].n) continue;
        a[k] = fn_tab[i].addr; c[k] = fn_tab[i].n; k++;
        if (k == 4) { pm_log_hw("fncov f%u %08x:%u %08x:%u %08x:%u %08x:%u", f, a[0], c[0], a[1], c[1], a[2], c[2], a[3], c[3]); k = 0; }
    }
    for (unsigned j = 0; j < k; j++) pm_log_hw("fncov f%u %08x:%u", f, a[j], c[j]);
    pm_log_hw("fncov f%u done", f);
}

/* Switch fncovrot=<min frame>: no fixed frame. The census arms at the walker entry of the first frame >= min in which
 * pm_fncov_event is called (draw_box.c: a ROTSCALE box whose contents go to an off-screen pass). */
static unsigned fn_min;
static void fn_init_once(void) __attribute__((no_instrument_function));
static void fn_init_once(void) {
    char v[16];
    if (fn_init) return;
    fn_init = 1;
    if (pm_debug_value("fncovrot", v, sizeof v) != 0) {
        fn_on = 1;
        fn_min = (unsigned)atoi(v);
        fn_want = 0xFFFFFFFFu;
    } else {
        fn_on = pm_debug_value("fncov", v, sizeof v) != 0;
        fn_want = fn_on ? (unsigned)atoi(v) : 0u;
    }
}

/* Called by draw_box.c at the off-screen contents branch, with the frame of the step being built. */
void pm_fncov_event(unsigned f) __attribute__((no_instrument_function));
void pm_fncov_event(unsigned f) {
    fn_init_once();
    if (fn_on && !fn_active && !fn_done && fn_want == 0xFFFFFFFFu && f >= fn_min) fn_want = f;
}

/* Called by nd_wq.c before each walker entry runs (game thread), with the frame of the entry. */
void pm_fncov_walk(unsigned f) __attribute__((no_instrument_function));
void pm_fncov_walk(unsigned f) {
    fn_init_once();
    if (!fn_on || fn_done) return;
    if (!fn_active && f >= fn_want) { memset(fn_tab, 0, sizeof fn_tab); fn_f0 = f; fn_active = 1; }
    else if (fn_active && f > fn_f0) { fn_active = 0; fn_done = 1; fn_dump(fn_f0); }
}
