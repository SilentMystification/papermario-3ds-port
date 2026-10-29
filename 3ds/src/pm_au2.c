#include <3ds.h>
#include <string.h>
#include "pm_port.h"
#include "pm_au2.h"

/* AU2: SPSC ring of command jobs. Producer = the audio control thread (or the game thread while that thread is joined: never both).
 * Consumer = the render thread. Design: docs/plans/AU2_design.md. A job is the commands between two commits (one slot).
 * sem_free counts empty slots, sem_full counts committed slots. committed and executed count jobs and decide the drain. */

typedef struct { uint32_t n; AuCmd c[AU2_SLOT_CMDS]; } Slot;
static Slot slots[AU2_SLOTS];
static uint32_t prod, cons;
static int open_;
static LightSemaphore sem_free, sem_full;
static LightEvent ev_idle;
static uint32_t committed, executed;
static int quit;
static Thread rthread;
static u8 r_crash_stack[0x4000] __attribute__((aligned(8)));
volatile int pm_au2_active;

/* Statistics (switch audiostat). The control side writes the first group, the render thread the second. */
static int st_on = -1;
static unsigned st_cmds, st_jobs, st_cmax, st_fill_n, st_fill_sum, st_fill_min = 0xFFFFFFFFu;
static unsigned long long st_emit_ticks;
static unsigned st_full_n, st_sync_n[AU2_SYNC_N];
static unsigned long long st_full_ticks, st_sync_ticks[AU2_SYNC_N];
static volatile unsigned r_jobs, r_busy_max;
static volatile unsigned long long r_busy_sum;

int pm_au2_on_render(void) { return rthread && threadGetCurrent() == rthread; }

static int stats(void) {
    if (st_on < 0) st_on = pm_debug_has("audiostat") ? 1 : 0;
    return st_on;
}

/* 1 when the render thread has nothing to do and nothing is waiting to be committed: a control call may then run its render
 * part at once on the control thread. Same order as the queue, but no thread hop (the first post of a step must not wait for a wake). */
int pm_au2_idle(void) {
    return !open_ && __atomic_load_n(&executed, __ATOMIC_ACQUIRE) == __atomic_load_n(&committed, __ATOMIC_ACQUIRE);
}

void pm_au2_commit(void) {
    if (!open_) return;
    Slot* s = &slots[prod];
    open_ = 0;
    prod = (prod + 1) % AU2_SLOTS;
    uint32_t c = __atomic_add_fetch(&committed, 1, __ATOMIC_RELEASE);
    if (stats()) {
        uint32_t fill = c - __atomic_load_n(&executed, __ATOMIC_ACQUIRE);
        st_jobs++;
        st_cmds += s->n;
        if (s->n > st_cmax) st_cmax = s->n;
        st_fill_n++;
        st_fill_sum += fill;
        if (fill < st_fill_min) st_fill_min = fill;
    }
    LightSemaphore_Release(&sem_full, 1);
}

void pm_au2_emit(const AuCmd* c) {
    int st = stats();
    unsigned t0 = st ? pm_ticks() : 0;
    if (!open_) {
        if (LightSemaphore_TryAcquire(&sem_free, 1) != 0) { /* ring full: the control thread waits for the render thread */
            unsigned w0 = pm_ticks();
            LightSemaphore_Acquire(&sem_free, 1);
            if (st) { st_full_n++; st_full_ticks += pm_ticks() - w0; }
        }
        slots[prod].n = 0;
        open_ = 1;
    }
    Slot* s = &slots[prod];
    s->c[s->n++] = *c;
    if (s->n == AU2_SLOT_CMDS) pm_au2_commit();
    if (st) st_emit_ticks += pm_ticks() - t0;
}

/* Wait until the render thread ran the first `target` jobs (target = a value of pm_au2_committed). Not on the render thread. */
static int wait_jobs(uint32_t target) {
    int waited = 0;
    while ((int32_t)(__atomic_load_n(&executed, __ATOMIC_ACQUIRE) - target) < 0) {
        LightEvent_Clear(&ev_idle);
        if ((int32_t)(__atomic_load_n(&executed, __ATOMIC_ACQUIRE) - target) >= 0) break;
        LightEvent_Wait(&ev_idle);
        waited = 1;
    }
    return waited;
}

uint32_t pm_au2_committed(void) { return __atomic_load_n(&committed, __ATOMIC_ACQUIRE); }

void pm_au2_wait_jobs(uint32_t target) {
    if (!pm_au2_active || pm_au2_on_render()) return;
    wait_jobs(target);
}

/* Sleep hook: like wait_jobs, but gives up at tick_end (system ticks). 0 = all ran, -1 = deadline. */
static int wait_jobs_until(uint32_t target, unsigned long long tick_end) {
    while ((int32_t)(__atomic_load_n(&executed, __ATOMIC_ACQUIRE) - target) < 0) {
        LightEvent_Clear(&ev_idle);
        if ((int32_t)(__atomic_load_n(&executed, __ATOMIC_ACQUIRE) - target) >= 0) break;
        unsigned long long now = svcGetSystemTick();
        if (now >= tick_end) return -1;
        if (LightEvent_WaitTimeout(&ev_idle, (s64)((tick_end - now) * 1000000000ull / SYSCLOCK_ARM11)) != 0) return -1;
    }
    return 0;
}

int pm_au2_sync_timed(int reason, unsigned long long ns) {
    if (!pm_au2_active || pm_au2_on_render()) return 0;
    unsigned t0 = pm_ticks();
    unsigned long long tick_end = svcGetSystemTick() + (ns / 1000ull) * SYSCLOCK_ARM11 / 1000000ull;
    pm_au2_commit();
    int r = wait_jobs_until(__atomic_load_n(&committed, __ATOMIC_ACQUIRE), tick_end);
    if (stats() && reason >= 0 && reason < AU2_SYNC_N) { st_sync_n[reason]++; st_sync_ticks[reason] += pm_ticks() - t0; }
    return r;
}

void pm_au2_sync(int reason) {
    if (!pm_au2_active || pm_au2_on_render()) return;
    unsigned t0 = pm_ticks();
    pm_au2_commit();
    int waited = wait_jobs(__atomic_load_n(&committed, __ATOMIC_ACQUIRE));
    if (stats() && reason >= 0 && reason < AU2_SYNC_N) { st_sync_n[reason]++; if (waited) st_sync_ticks[reason] += pm_ticks() - t0; }
}

static void render_main(void* unused) {
    (void)unused;
    pm_crash_install(r_crash_stack, sizeof(r_crash_stack));
    pm_stack_register(2, "render", 64 * 1024);
    pm_fpscr_setup("audio");
    pm_log_d("threads: audio render on core %d (svcGetProcessorID)", (int)svcGetProcessorID());
    for (;;) {
        LightSemaphore_Acquire(&sem_full, 1);
        if (__atomic_load_n(&executed, __ATOMIC_ACQUIRE) == __atomic_load_n(&committed, __ATOMIC_ACQUIRE)) {
            if (__atomic_load_n(&quit, __ATOMIC_ACQUIRE)) break; /* poison release from pm_au2_stop */
            continue;
        }
        Slot* s = &slots[cons];
        int st = stats();
        unsigned t0 = st ? pm_ticks() : 0;
        for (uint32_t i = 0; i < s->n; i++) au2_exec(&s->c[i]);
        if (st) {
            unsigned dt = pm_ticks() - t0;
            r_jobs++;
            r_busy_sum += dt;
            if (dt > r_busy_max) r_busy_max = dt;
        }
        cons = (cons + 1) % AU2_SLOTS;
        LightSemaphore_Release(&sem_free, 1);
        __atomic_add_fetch(&executed, 1, __ATOMIC_RELEASE);
        LightEvent_Signal(&ev_idle);
    }
}

int pm_au2_start(void) {
    if (rthread) return 1;
    if (pm_debug_has("au2off")) { pm_log("audio2: off (switch au2off), mixer runs inside the audio job"); return 0; }
    LightSemaphore_Init(&sem_free, AU2_SLOTS, AU2_SLOTS);
    LightSemaphore_Init(&sem_full, 0, AU2_SLOTS + 1);
    LightEvent_Init(&ev_idle, RESET_STICKY);
    prod = cons = 0;
    open_ = 0;
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    /* Lower priority than the control thread: a kicked control job preempts the mixer on the same core. */
    int core = pm_debug_has("audiocore0") ? -2 : 1;
    rthread = threadCreate(render_main, NULL, 64 * 1024, prio + 1, core, false);
    if (!rthread) {
        pm_log_w("audio2: render thread not created, mixer runs inside the audio job");
        return 0;
    }
    pm_au2_active = 1;
    pm_log("audio2: render thread started, ring %d jobs of %d commands (%u KB)", AU2_SLOTS, AU2_SLOT_CMDS, (unsigned)(sizeof(slots) / 1024));
    return 1;
}

void pm_au2_stop(void) {
    if (!rthread) return;
    pm_au2_sync(AU2_SYNC_SHUTDOWN);
    pm_au2_active = 0;
    __atomic_store_n(&quit, 1, __ATOMIC_RELEASE);
    LightSemaphore_Release(&sem_full, 1);
    threadJoin(rthread, U64_MAX);
    threadFree(rthread);
    rthread = NULL;
}

/* AUDDSP A1: voices in use per audio tick, window of one stats line */
static unsigned vs_n, vs_sum, vs_max, vs_hist[5], vs_bus[4], vs_adpcm, vs_loop, vs_hi, vs_full, vs_fx[4][12];
void pm_au2_vstat(int used, const int bus[4], int adpcm, int loop, int hi, int fx0, int fx1, int fx2, int fx3) {
    if (!pm_debug_has("audiostat")) return;
    vs_n++; vs_sum += used; if ((unsigned)used > vs_max) vs_max = used;
    vs_hist[used <= 8 ? 0 : used <= 12 ? 1 : used <= 16 ? 2 : used <= 23 ? 3 : 4]++;
    if (used >= 24) vs_full++;
    for (int i = 0; i < 4; i++) vs_bus[i] += bus[i];
    vs_adpcm += adpcm; vs_loop += loop; vs_hi += hi;
    int fx[4] = {fx0, fx1, fx2, fx3};
    for (int i = 0; i < 4; i++) vs_fx[i][fx[i] > 11 ? 11 : fx[i]]++;
}

void pm_au2_bankstat(int romOfs, int ninst, int nunique, unsigned bytes, unsigned maxlen, int nloop, int nraw) {
    static unsigned total;
    total += bytes;
    pm_log("auddsp bank: rom 0x%x slots %d unique samples %d adpcm bytes %u max sample %u looped %d non-adpcm %d, sum of banks loaded so far %u",
           (unsigned)romOfs, ninst, nunique, bytes, maxlen, nloop, nraw, total);
}

static unsigned vu_wav[1024], vu_len[1024], vu_n, vu_starts, vu_bytes;
void pm_au2_vstart(unsigned wav, unsigned len) {
    if (!pm_debug_has("audiostat")) return;
    vu_starts++;
    for (unsigned i = 0; i < vu_n; i++) if (vu_wav[i] == wav) return;
    if (vu_n < 1024) { vu_wav[vu_n] = wav; vu_len[vu_n++] = len; vu_bytes += len; }
}

void pm_au2_stats_log(void) {
    if (pm_debug_has("audiostat") && vu_starts) {
        unsigned mx = 0;
        for (unsigned i = 0; i < vu_n; i++) if (vu_len[i] > mx) mx = vu_len[i];
        pm_log_d("audio vsamples: voice starts %u unique samples %u adpcm bytes %u (pcm16 equal %u) largest %u", vu_starts, vu_n, vu_bytes, vu_bytes * 32 / 9, mx);
        vu_n = vu_starts = vu_bytes = 0;
    }
    if (pm_debug_has("audiostat") && vs_n) {
        pm_log_d("audio vstat: ticks %u voices avg x100 %u max %u, ticks with <=8 %u, 9-12 %u, 13-16 %u, 17-23 %u, 24 %u; avg x100 per bus b0 %u b1 %u b2 %u b3 %u; adpcm %u loop %u pitch>2 %u (x100 avg)",
                 vs_n, vs_sum * 100 / vs_n, vs_max, vs_hist[0], vs_hist[1], vs_hist[2], vs_hist[3], vs_hist[4],
                 vs_bus[0] * 100 / vs_n, vs_bus[1] * 100 / vs_n, vs_bus[2] * 100 / vs_n, vs_bus[3] * 100 / vs_n,
                 vs_adpcm * 100 / vs_n, vs_loop * 100 / vs_n, vs_hi * 100 / vs_n);
        pm_log_d("audio vfx: ticks per fx type (0 none 1 small 2 big 3 chorus 4 flange 5 echo 6-9 custom 10 otherbig) bus0 %u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u bus1 %u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u bus2 %u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u bus3 %u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u",
                 vs_fx[0][0], vs_fx[0][1], vs_fx[0][2], vs_fx[0][3], vs_fx[0][4], vs_fx[0][5], vs_fx[0][6], vs_fx[0][7], vs_fx[0][8], vs_fx[0][9], vs_fx[0][10],
                 vs_fx[1][0], vs_fx[1][1], vs_fx[1][2], vs_fx[1][3], vs_fx[1][4], vs_fx[1][5], vs_fx[1][6], vs_fx[1][7], vs_fx[1][8], vs_fx[1][9], vs_fx[1][10],
                 vs_fx[2][0], vs_fx[2][1], vs_fx[2][2], vs_fx[2][3], vs_fx[2][4], vs_fx[2][5], vs_fx[2][6], vs_fx[2][7], vs_fx[2][8], vs_fx[2][9], vs_fx[2][10],
                 vs_fx[3][0], vs_fx[3][1], vs_fx[3][2], vs_fx[3][3], vs_fx[3][4], vs_fx[3][5], vs_fx[3][6], vs_fx[3][7], vs_fx[3][8], vs_fx[3][9], vs_fx[3][10]);
        vs_n = vs_sum = vs_max = vs_adpcm = vs_loop = vs_hi = vs_full = 0;
        memset(vs_hist, 0, sizeof vs_hist); memset(vs_bus, 0, sizeof vs_bus); memset(vs_fx, 0, sizeof vs_fx);
    }
    if (!stats() || !pm_au2_active) return;
    unsigned j = st_jobs ? st_jobs : 1, rj = r_jobs ? r_jobs : 1;
    pm_log_d("audio2: jobs %u cmds/job avg %u max %u, emit cost %u us per job, ring fill min %u avg %u (jobs queued at commit), render busy avg %u us max %u us per job, ring full waits %u (%u us)",
             st_jobs, st_cmds / j, st_cmax, (unsigned)(st_emit_ticks / j / 268u), st_fill_n ? st_fill_min : 0, st_fill_n ? st_fill_sum / st_fill_n : 0,
             (unsigned)(r_busy_sum / rj / 268u), r_busy_max / 268u, st_full_n, (unsigned)(st_full_ticks / 268u));
    pm_log_d("audio2: render drains (count, total wait us): bank %u %u, map %u %u, save %u %u, shutdown %u %u",
             st_sync_n[0], (unsigned)(st_sync_ticks[0] / 268u), st_sync_n[1], (unsigned)(st_sync_ticks[1] / 268u),
             st_sync_n[2], (unsigned)(st_sync_ticks[2] / 268u), st_sync_n[3], (unsigned)(st_sync_ticks[3] / 268u));
    st_cmds = st_jobs = st_cmax = st_fill_n = st_fill_sum = 0;
    st_fill_min = 0xFFFFFFFFu;
    st_emit_ticks = 0;
    r_jobs = 0; r_busy_sum = 0; r_busy_max = 0;
}
