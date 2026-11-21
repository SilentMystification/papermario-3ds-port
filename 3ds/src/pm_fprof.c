/* PROF4 function profile (PM_FPROF lane only, tools). The TUs of the census list are built with -finstrument-functions. The hooks
 * give exclusive cycles per function per thread (core 0 = game thread, core 1 = logic thread): a tick (svcGetSystemTick, the CPU
 * clock) at each entry and exit of a call at depth < fp_dmax; the time between two ticks goes to the function on top of the stack.
 * Deeper calls and other threads are not timed. Switch fprofd=<1..8> sets fp_dmax (default 4).
 * Each thread owns 2 table halves. A window change (fp_gen) swaps a thread to the other half at its next depth 0 entry and marks the
 * half it left as done. pm_fprof_window (pm_phase.c, window end) prints each done half, then clears it. All functions here are
 * no_instrument_function. */
#include <string.h>
#include <stdlib.h>
#include <3ds.h>
#include "pm_port.h"

void pm_log_prof_line(const char* fmt, ...); /* pm_log.c */

#define NF __attribute__((no_instrument_function))
#define FP_N 2048u /* slots of a half, a power of two (PROF4B: 2048, a full half drops counts) */
#define FP_TPM 268112ull /* ticks per ms */
#define FP_TOP 30u
typedef struct { unsigned addr, n; unsigned long long cyc; } FpEnt;
typedef struct { FpEnt e[FP_N]; unsigned calls, pairs, drop, used; unsigned dn[9]; unsigned f0, f1; } FpHalf; /* dn: calls per depth, 8 = 8 and up */
typedef struct {
    void* tls;
    unsigned d, sp, last, gen, cur;
    unsigned stk[8];
    unsigned char done[2];
    FpHalf h[2];
} FpSt;
static FpSt fp_st[2];
static unsigned fp_gen, fp_dmax = 4u, fp_hook_t, fp_pair_t, fp_cal_pend;
static unsigned fp_cnt, fp_win, fp_live, fp_arm, fp_end, fp_frame; /* PROF4B: fprofn=<arm>:<end>, counts only while fp_live (arm..end) */
static unsigned fp_last_total, fp_last_steps;
static FpEnt fp_sink;

static FpSt* fp_me(void) NF;
static FpSt* fp_me(void) {
    void* t = getThreadLocalStorage();
    if (t == fp_st[0].tls) return &fp_st[0];
    if (t == fp_st[1].tls) return &fp_st[1];
    return 0;
}

static FpEnt* fp_slot(FpSt* s, unsigned a) NF;
static FpEnt* fp_slot(FpSt* s, unsigned a) {
    FpHalf* h = &s->h[s->cur];
    unsigned i = (a * 2654435761u) >> 21; /* the top 11 bits of the multiplicative hash */
    for (unsigned k = 0; k < FP_N; k++) {
        FpEnt* e = &h->e[(i + k) & (FP_N - 1u)];
        if (e->addr == a) return e;
        if (!e->addr) { e->addr = a; h->used++; return e; }
    }
    h->drop++;
    return &fp_sink;
}

static void fp_swap(FpSt* s) NF;
static void fp_swap(FpSt* s) {
    s->done[s->cur] = 1;
    s->h[s->cur].f1 = fp_frame;
    s->cur ^= 1u;
    s->h[s->cur].f0 = fp_frame;
    s->gen = fp_gen;
}

void __cyg_profile_func_enter(void* fn, void* cs) NF;
void __cyg_profile_func_enter(void* fn, void* cs) {
    FpSt* s = fp_me();
    (void)cs;
    if (!s) return; /* another thread: no count */
    if (s->d == 0u && s->gen != fp_gen) fp_swap(s); /* window change at depth 0: the other half */
    if (fp_cnt) { /* PROF4B: counts only, between the arm and end frames */
        if (fp_live) { FpEnt* e = fp_slot(s, (unsigned)fn); s->h[s->cur].calls++; e->n++; e->cyc += s->d; s->h[s->cur].dn[s->d < 8u ? s->d : 8u]++; }
        s->d++;
        return;
    }
    if (fp_win && !fp_live) { s->d++; return; } /* PROF4B: fprofw, timed only between the arm and end frames */
    s->h[s->cur].calls++;
    if (s->d++ >= fp_dmax) return; /* deeper calls: their time stays with the timed caller */
    const unsigned t = (unsigned)svcGetSystemTick();
    s->h[s->cur].pairs++;
    if (s->sp) fp_slot(s, s->stk[s->sp - 1u])->cyc += t - s->last;
    s->stk[s->sp++] = (unsigned)fn;
    s->last = t;
}
void __cyg_profile_func_exit(void* fn, void* cs) NF;
void __cyg_profile_func_exit(void* fn, void* cs) {
    FpSt* s = fp_me();
    (void)fn; (void)cs;
    if (!s || !s->d) return;
    if (fp_cnt) { s->d--; return; } /* PROF4B: counts only */
    if (fp_win && !fp_live) { s->d--; return; }
    if (--s->d >= fp_dmax) return;
    const unsigned t = (unsigned)svcGetSystemTick();
    if (!s->sp) return;
    FpEnt* e = fp_slot(s, s->stk[--s->sp]);
    e->cyc += t - s->last;
    e->n++;
    s->last = t;
}

static void fp_clear(FpSt* s, unsigned k) NF;
static void fp_clear(FpSt* s, unsigned k) { memset(&s->h[k], 0, sizeof s->h[k]); s->done[k] = 0; }

/* Called by the game thread (pm_main.c game_thread, after the switches) and the logic thread (pm_logic.c lg_main start). */
void pm_fprof_thread(int k) NF;
void pm_fprof_thread(int k) {
    char v[16];
    fp_st[k].tls = getThreadLocalStorage();
    if (k != 0) return;
    if (pm_debug_value("fprofd", v, sizeof v)) {
        unsigned d = (unsigned)atoi(v);
        fp_dmax = d < 1u ? 1u : d > 8u ? 8u : d;
    }
    /* calibration: 1000 untimed pairs (depth at the limit), then 1000 timed pairs, on this thread's own state */
    FpSt* s = &fp_st[0];
    unsigned t0 = (unsigned)svcGetSystemTick();
    s->d = fp_dmax;
    for (unsigned i = 0; i < 1000u; i++) { __cyg_profile_func_enter((void*)0x100, 0); __cyg_profile_func_exit((void*)0x100, 0); }
    unsigned t1 = (unsigned)svcGetSystemTick();
    s->d = 0;
    for (unsigned i = 0; i < 1000u; i++) { __cyg_profile_func_enter((void*)0x100, 0); __cyg_profile_func_exit((void*)0x100, 0); }
    unsigned t2 = (unsigned)svcGetSystemTick();
    fp_hook_t = t1 - t0;
    fp_pair_t = t2 - t1;
    s->d = s->sp = 0;
    fp_clear(s, 0);
    fp_clear(s, 1);
    fp_cnt = pm_debug_value("fprofn", v, sizeof v) ? 1u : 0u; /* PROF4B: fprofn=<arm>:<end>, after the calibration (it runs the timed path) */
    if (pm_debug_value("fprofw", v, sizeof v)) { /* PROF4B: timed frame window fprofw=<arm>:<end> */
        const char* c = strchr(v, ':');
        fp_win = 1u;
        fp_arm = (unsigned)atoi(v);
        fp_end = c ? (unsigned)atoi(c + 1) : fp_arm + 80u;
    }
    if (fp_cnt) {
        const char* c = strchr(v, ':');
        fp_arm = (unsigned)atoi(v);
        fp_end = c ? (unsigned)atoi(c + 1) : fp_arm + 80u;
    }
    s->cur = 0;
    s->gen = fp_gen;
    fp_cal_pend = 1;
}

typedef struct { unsigned a; unsigned long long c; unsigned n; } FpTop;

static void fp_print(unsigned f, unsigned core, unsigned steps) NF;
static void fp_print(unsigned f, unsigned core, unsigned steps) {
    FpSt* s = &fp_st[core];
    for (unsigned k = 0; k < 2u; k++) {
        if (!s->done[k] || k == s->cur) continue;
        FpHalf* h = &s->h[k];
        if (!h->calls) { fp_clear(s, k); continue; } /* PROF4B: a half before the arm has no calls */
        if (fp_cnt) { /* PROF4B: the span of the half is its frames (f1 - f0); a half with no calls (before the arm) is skipped */
            if (!h->calls) { fp_clear(s, k); continue; }
            unsigned span = h->f1 > h->f0 ? h->f1 - h->f0 : 1u;
            for (unsigned i = 0; i < FP_N; i++) {
                const FpEnt* e = &h->e[i];
                if (e->addr && e->n >= 1u) pm_log_prof_line("fprofn f%u c%u %08x n %u dsum %u", f, core, e->addr, e->n, (unsigned)e->cyc);
            }
            pm_log_prof_line("fprofn f%u c%u steps %u calls %u d %u %u %u %u %u %u %u %u %u", f, core, span, h->calls,
                h->dn[0], h->dn[1], h->dn[2], h->dn[3], h->dn[4], h->dn[5], h->dn[6], h->dn[7], h->dn[8]);
            fp_clear(s, k);
            continue;
        }
        FpTop top[FP_TOP];
        unsigned nt = 0;
        for (unsigned i = 0; i < FP_N; i++) { /* insertion into the top 30 by cyc */
            const FpEnt* e = &h->e[i];
            if (!e->addr || !e->cyc) continue;
            unsigned p;
            if (nt < FP_TOP) p = nt++;
            else if (e->cyc > top[FP_TOP - 1u].c) p = FP_TOP - 1u;
            else continue;
            while (p > 0u && top[p - 1u].c < e->cyc) { top[p] = top[p - 1u]; p--; }
            top[p].a = e->addr; top[p].c = e->cyc; top[p].n = e->n;
        }
        unsigned st = h->f1 > h->f0 ? h->f1 - h->f0 : (steps ? steps : 1u); /* PROF4B: the frames of the half when set */
        for (unsigned r = 0; r < nt; r++) {
            unsigned ms100 = (unsigned)(top[r].c * 100ull / ((unsigned long long)st * FP_TPM));
            unsigned n10 = (unsigned)((unsigned long long)top[r].n * 10ull / st);
            pm_log_prof_line("fprof f%u c%u r%u %08x ex %u n %u", f, core, r, top[r].a, ms100, n10);
        }
        /* ovh: the untimed calls cost one hook pair each, the timed ones one timed pair (calibration: 1000 of each) */
        unsigned long long ov = ((unsigned long long)(h->calls - h->pairs) * fp_hook_t + (unsigned long long)h->pairs * fp_pair_t) / 1000u;
        pm_log_prof_line("fprof f%u c%u calls %u pairs %u drop %u ovh %u", f, core, h->calls / st, h->pairs / st, h->drop,
            (unsigned)(ov * 100ull / ((unsigned long long)st * FP_TPM)));
        fp_clear(s, k);
    }
}

/* Called at the window print (pm_phase.c pf_report, game thread). The game thread swaps its own half here (a swap at any depth is
 * safe: the stack holds addresses, the open calls close in the new half) and prints it at once: core 0 lines are of this window.
 * The logic thread swaps at its next depth 0 entry: its half is printed at the next window (the frame and steps of the previous call). */
void pm_fprof_window(unsigned total, unsigned steps) NF;
/* PROF4B: called by pm_phase.c each frame (game thread): arm swaps both halves on, end prints core 0, end + 10 prints core 1. */
void pm_fprof_frame(unsigned f) NF;
void pm_fprof_frame(unsigned f) {
    if (!fp_cnt && !fp_win) return;
    fp_frame = f;
    if (f == fp_arm) {
        if (fp_cal_pend) { fp_cal_pend = 0; pm_log_prof_line("fprof cal hook %u pair %u", fp_hook_t, fp_pair_t); }
        fp_live = 1; fp_gen++; fp_swap(&fp_st[0]);
    }
    else if (f == fp_end) { fp_gen++; fp_swap(&fp_st[0]); fp_print(f, 0u, 0u); }
    else if (f == fp_end + 10u) { fp_print(fp_end, 1u, 0u); fp_live = 0; }
}
void pm_fprof_window(unsigned total, unsigned steps) {
    if (fp_cal_pend) {
        fp_cal_pend = 0;
        pm_log_prof_line("fprof cal hook %u pair %u", fp_hook_t, fp_pair_t);
    }
    if (fp_cnt) return; /* PROF4B: counts mode prints at the frame window (pm_fprof_frame) only */
    fp_print(fp_last_total, 1u, fp_last_steps);
    fp_gen++;
    fp_swap(&fp_st[0]);
    fp_print(total, 0u, steps);
    fp_last_total = total;
    fp_last_steps = steps;
}
