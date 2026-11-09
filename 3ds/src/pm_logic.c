/* CORE2C-3: the logic thread (row_CORE2C.md, plan_core2.md). The game thread (core 0) keeps the frame loop and the walker: aptMainLoop,
 * the pad scan, the menu, the pacing, the queue of walker entries (frame begin, messages, walks, frame end and present), the HUD. The
 * logic thread (core 1 on the O3DS, core 2 on the N3DS, priority game + 2: below the audio control and mixer threads) runs the
 * commands the frame loop hands it, one per pass (pm_port.h LgCmd): step_game_loop, the list build, the audio kick, the step tail.
 * Overlap: the frame loop hands step N+1 and then walks frame N, so the logic of N+1 runs while the walker draws N. The logic of N+2
 * starts only after the push of N+1, which waits until frame N is walked (nd_wq.c depth 1).
 * Serial (pm_lg_hand serial = 1): the command runs inline on the game thread after the logic thread is idle (det, phase and census
 * timers, fsplit, 60 fps interpolation, the prof sample frame): the serial order, byte-equal to the single-thread build.
 * The game thread never waits for the logic thread without running the queue for it (pm_wq_pump_blocked), so a drain or a frame push
 * inside a step always finishes. */
#include <3ds.h>
#include "pm_port.h"

void pm_wq_set_logic(Thread t); /* nd_wq.c */
int pm_wq_pump_one(void);
void pm_wq_pump_blocked(void);
void pm_wq_wait_walker(long long ns);
void pm_wq_kick_walker(void);
void pm_crash_install_here(void); /* pm_main.c */
void pm_fpscr_setup(const char* who);
void pm_stack_register(int id, const char* name, unsigned size);
unsigned pm_stack_used(int id, unsigned* size);
extern s16 D_80073E0A;

#define LG_STACK (64u * 1024u) /* heap (threadCreate): the game logic and list build; prof line stk shows the high-water mark */
#define LG_RING 4u
static LgCmd lg_ring[LG_RING];
static volatile unsigned lg_handed, lg_done, lg_dpub, lg_steps; /* commands handed and done (both modes), retrace flags published, steps done */
static volatile unsigned lg_qn, lg_qtaken; /* commands queued to the logic thread and taken by it (the serial mode does not touch them) */
static volatile int lg_d, lg_exit;
static unsigned lg_ends; /* game thread: step ends walked */
static Thread lg_th, lg_walker;
static LightEvent lg_ev; /* a command handed, or the exit */
static int lg_core = -1;
static volatile unsigned long long lg_pf_t; static volatile unsigned lg_pf_n; /* logic thread: ticks and count of the steps it ran */
/* PMU2: core 1 counter sums (ic, dc, cycles) over the logic steps of prof mode; lg_pmu_on = the core 1 PMU is set (logic thread only) */
static volatile unsigned long long lg_pf_pe[3]; static int lg_pmu_on;
extern int pm_pf_on; /* pm_phase.c */
void pm_pf_core_setup(void); /* pm_phase.c: the counter setup of the calling core */
void pm_pf_core_read(u64 out[3]);
static unsigned long long lg_pf_wait; static unsigned lg_pf_ser; /* game thread: ticks waiting for the logic, steps run inline */
#define LG_WAIT_NS 2000000ll

static void lg_exec(const LgCmd* c) {
    pm_step_run(c);
    __sync_synchronize();
    if (c->kind != LG_IDLE) lg_steps++;
    lg_done++;
}

static void lg_main(void* arg) {
    (void)arg;
    pm_crash_install_here();
    pm_fpscr_setup("logic"); /* the N64 FPU mode (flush to zero, nearest) is per thread */
    pm_stack_register(3, "logic", LG_STACK);
    lg_core = (int)svcGetProcessorID();
    for (;;) {
        while (lg_qtaken == lg_qn && !lg_exit) LightEvent_Wait(&lg_ev);
        if (lg_qtaken == lg_qn) break;
        __sync_synchronize();
        LgCmd c = lg_ring[lg_qtaken & (LG_RING - 1u)];
        __sync_synchronize();
        lg_qtaken++;
        /* PMU2: prof mode only. The core 1 PMU is set here, on this thread, at the first prof step (and again after prof was off). */
        int pf = pm_pf_on && c.kind != LG_IDLE;
        if (!pm_pf_on) lg_pmu_on = 0;
        u64 p0[3], p1[3];
        if (pf && !lg_pmu_on) { pm_pf_core_setup(); lg_pmu_on = 1; }
        if (pf) pm_pf_core_read(p0);
        u64 t0 = svcGetSystemTick();
        lg_exec(&c);
        if (c.kind != LG_IDLE) { lg_pf_t += svcGetSystemTick() - t0; lg_pf_n++; }
        if (pf) {
            pm_pf_core_read(p1);
            for (int k = 0; k < 3; k++) lg_pf_pe[k] += (unsigned)(p1[k] - p0[k]); /* u32 delta: one step is far below 2^32 events */
        }
        pm_wq_kick_walker();
    }
}

void pm_lg_start(void) {
    lg_walker = threadGetCurrent();
    lg_d = D_80073E0A != 0;
    LightEvent_Init(&lg_ev, RESET_ONESHOT);
    bool n3 = false;
    APT_CheckNew3DS(&n3);
    int core = n3 ? 2 : 1;
    s32 prio = 0x30;
    svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
    prio += 2; /* audio control = game priority, mixer + 1 */
    Thread t = threadCreate(lg_main, NULL, LG_STACK, prio, core, false);
    if (!t) { pm_log_w("logic: thread on core %d not created, the steps run on the game thread", core); return; }
    pm_wq_set_logic(t);
    lg_th = t;
    pm_log_w("logic: thread on core %d prio %02x stack %u KB", core, (unsigned)prio, LG_STACK >> 10);
}

void pm_lg_publish_d(int d) {
    lg_d = d;
    __sync_synchronize();
    lg_dpub++;
    if (lg_th && threadGetCurrent() == lg_th) pm_wq_kick_walker();
}

/* Game thread: wait until the logic is idle (every handed command done), running the queue for it, then run the whole queue. Other
 * threads (an APT hook on the APT thread) return at once: only the walker may run the queue. */
void pm_lg_quiesce(void) {
    if (!lg_th || threadGetCurrent() != lg_walker) return;
    while ((int)(lg_done - lg_handed) < 0) { pm_wq_pump_blocked(); pm_wq_wait_walker(LG_WAIT_NS); }
    while (pm_wq_pump_one()) {}
}

void pm_lg_hand(const LgCmd* c, int serial) {
    if (serial || !lg_th) { /* inline: lg_done passes lg_handed during the run, so a post inside it does not wait for itself */
        pm_lg_quiesce();
        lg_exec(c);
        lg_handed++;
        if (c->kind != LG_IDLE) lg_pf_ser++;
        return;
    }
    while (lg_qn - lg_qtaken >= LG_RING) { pm_wq_pump_blocked(); pm_wq_wait_walker(LG_WAIT_NS); }
    lg_ring[lg_qn & (LG_RING - 1u)] = *c;
    __sync_synchronize();
    lg_handed++;
    lg_qn++;
    LightEvent_Signal(&lg_ev);
}

int pm_lg_draw_flag(void) {
    if (lg_dpub != lg_handed) { /* the walk wait: the retrace of the last command starts when the step before it is done */
        u64 t0 = svcGetSystemTick();
        while (lg_dpub != lg_handed) { pm_wq_pump_blocked(); pm_wq_wait_walker(LG_WAIT_NS); }
        lg_pf_wait += svcGetSystemTick() - t0;
    }
    __sync_synchronize();
    return lg_d;
}

void pm_lg_end_walked(void) { lg_ends++; } /* the step end entry (pm_main.c), on the walker */
void pm_lg_show(void) {
    unsigned target = lg_steps;
    __sync_synchronize();
    while ((int)(lg_ends - target) < 0) { if (!pm_wq_pump_one()) pm_wq_wait_walker(LG_WAIT_NS); }
    pm_wq_pump_blocked();
}

void pm_lg_sleep_ns(long long ns) {
    if (!lg_th || lg_done == lg_handed) { svcSleepThread(ns); return; }
    u64 end = svcGetSystemTick() + (u64)ns * 268ull / 1000ull;
    for (;;) {
        pm_wq_pump_blocked();
        u64 now = svcGetSystemTick();
        if (now >= end) break;
        pm_wq_wait_walker((long long)((end - now) * 1000ull / 268ull));
    }
}

void pm_lg_stop(void) {
    if (!lg_th) return;
    pm_lg_quiesce();
    lg_exit = 1;
    __sync_synchronize();
    LightEvent_Signal(&lg_ev);
    threadJoin(lg_th, U64_MAX);
    threadFree(lg_th);
    lg_th = NULL;
    pm_wq_set_logic(NULL);
}

/* Prof (pm_phase.c): totals since boot; the window takes differences. */
void pm_lg_pf(unsigned long long* lg_t, unsigned* lg_n, unsigned long long* wait_t, unsigned* ser_n, int* core, unsigned* stk_kb, unsigned long long* pe) {
    *lg_t = lg_pf_t; *lg_n = lg_pf_n; *wait_t = lg_pf_wait; *ser_n = lg_pf_ser; *core = lg_core;
    for (int k = 0; k < 3; k++) pe[k] = lg_pf_pe[k];
    *stk_kb = pm_stack_used(3, NULL) >> 10;
}
