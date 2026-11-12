/* CORE2-3a: the walker side. The walker owns nd_ws, pm_gpu (C3D context, command buffers, GX queue, frame begin and end, present,
 * shots, grabs, captures, depth reads) and pm_tex (texture cache). It runs on the game thread (core 0, the frame loop). The game logic
 * (pm_logic.c: step_game_loop, the list build, the step tail) runs on the logic thread (core 1 on the O3DS, core 2 on the N3DS) or, in
 * the serial mode (det, phase or census timers, fsplit), inline on the game thread. The logic side reaches the
 * walker only through the calls below (plan_core2.md; row_CORE2C.md):
 *   pm_wq_post(fn, arg, size): a message. The arg bytes are copied into the queue; the walker runs fn in queue order.
 *   pm_wq_frame(frame): one frame (pm_port.h PmFrame). Depth 1: the push waits until the frame before is walked, so a display context
 *     is built again only after the walk of the frame that used it (the N64 rule).
 *   pm_wq_call(fn, arg, size): the caller needs a result or the walker output: fn runs on the walker after every queued entry, the
 *     caller waits for it.
 *   pm_walk_drain(why) (nd_walk.c): the logic is about to write memory a queued or running walk reads: wait until the queue is empty.
 * The queue is one ring of copied entries: the logic thread writes the tail, the game thread runs and frees the head. Each entry
 * carries the frame number of its poster (pm_walk_frame while it runs, so pm_frame_count() on the walker side is the frame of the
 * entry, as in the serial order). A logic thread that waits for the walker sets wq_blocked; every wait of the game thread runs the
 * queue then (pm_wq_pump_blocked), so a drain inside a step never waits for the next pass.
 * Serial: the poster is the walker. A post or a push runs the queue at once (the serial order; det is byte-equal to it).
 * On the game thread, a post or call outside a walk first stops the logic thread at a step boundary (pm_lg_quiesce): menu present,
 * shots, key frames.
 * pm_walk_side counts the walker-side code running on the game thread now (a walk, or fn of an entry). Switch c2own: the owned entry
 * points (PM_WALK_OWN in pm_gpu.c and pm_tex.c) log once each when they run outside the walker side or on the logic thread. */
#include <3ds.h>
#include <string.h>
#include "pm_port.h"

int pm_walk_side;
int pm_c2own = -1;
unsigned pm_walk_frame; /* the frame number of the entry the walker runs */
extern unsigned pm_frame_n; /* pm_main.c: the logic step counter */
extern int nd_c2_on; /* nd_c2haz.c */
void nd_c2_push(void);

/* The queue: power-of-two ring, free-running head (next to run) and tail (next free). 40 bytes an entry, 2.5 KB. */
#define WQ_N 64u
#define WQ_ARG 32u
typedef struct { void (*fn)(void*); unsigned frame; unsigned long long arg[WQ_ARG / 8]; } WqEnt;
static WqEnt wq_ring[WQ_N];
static volatile unsigned wq_head, wq_tail;
static volatile unsigned wq_fr_push, wq_fr_walked; /* frames pushed and walked */
static volatile int wq_blocked; /* the logic thread waits for the walker: 0 no, WQB_ALL the queue is run whole, WQB_WALKED it is run up to the last frame */
enum { WQB_ALL = 1, WQB_WALKED = 2 };
static Thread wq_lg; /* the logic thread (NULL: serial only) */
static LightEvent wq_ev_walker; /* logic to walker: blocked, a command done, a retrace started */
static LightEvent wq_ev_logic; /* walker to logic: an entry ran */
_Static_assert(sizeof(PmFrame) <= WQ_ARG, "a frame fits one queue entry");
_Static_assert(sizeof(PmKey) <= WQ_ARG, "a key frame fits one queue entry");

void pm_wq_set_logic(Thread t) {
    LightEvent_Init(&wq_ev_walker, RESET_ONESHOT);
    LightEvent_Init(&wq_ev_logic, RESET_ONESHOT);
    __sync_synchronize();
    wq_lg = t;
}
int pm_wq_on_logic(void) { return wq_lg && threadGetCurrent() == wq_lg; }
void pm_wq_kick_walker(void) { if (wq_lg) LightEvent_Signal(&wq_ev_walker); }
/* The game thread waits at most ns for the logic thread (a block, a done command, a retrace start). */
void pm_wq_wait_walker(long long ns) { LightEvent_WaitTimeout(&wq_ev_walker, ns); }

/* Game thread: run the head entry. 0 when the queue is empty. */
int pm_wq_pump_one(void) {
    unsigned h = wq_head;
    if (h == wq_tail) return 0;
    __sync_synchronize(); /* the entry bytes were written before the tail */
    WqEnt* e = &wq_ring[h & (WQ_N - 1u)];
    pm_walk_frame = e->frame;
#ifdef PM_FNCOV
    { extern void pm_fncov_walk(unsigned f); pm_fncov_walk(e->frame); } /* HC0 census lane only */
#endif
    pm_walk_side++;
    e->fn(e->arg);
    pm_walk_side--;
    __sync_synchronize();
    wq_head = h + 1u;
    __sync_synchronize();
    if (wq_blocked) LightEvent_Signal(&wq_ev_logic);
    return 1;
}
static void wq_pump(void) { while (pm_wq_pump_one()) {} }
/* Game thread, at every wait: a logic thread that waits for the walker gets the whole queue run. */
void pm_wq_pump_blocked(void) {
    int b = wq_blocked;
    if (b == WQB_ALL) wq_pump();
    else if (b == WQB_WALKED) { while (wq_fr_walked != wq_fr_push && pm_wq_pump_one()) {} } /* a whole pump would run the begin entry of the next step: one more vblank */
}

/* C1WAIT: the logic thread's wait for the walker, in ticks of the prof window (pm_phase.c reads it per logic step). [0] every
 * wq_block wait, [1] the depth 1 frame wait (c_walked). Modular 32 bit: the window reads take differences. Prof only (pm_pf_on). */
extern int pm_pf_on; /* pm_phase.c */
static unsigned wq_bw[2];
/* PAUSEDR: counters for the core2 wait line (pm_phase.c). pm_wq_dr: logic thread drains (pm_wq_idle), pm_wq_drw: those that found the
 * queue non-empty (they wait), pm_wq_wqf: logic thread waits for a full queue (c_space). Modular 32 bit, a window reads differences. */
unsigned pm_wq_dr, pm_wq_drw, pm_wq_wqf;
static int c_walked(void* c);
/* Logic thread: wait until cond(). The walker runs the queue when it sees wq_blocked (flag, barrier, test on both sides). */
static void wq_block(int (*cond)(void*), void* c, int kind) {
    int pf = pm_pf_on;
    unsigned t0 = pf ? (unsigned)svcGetSystemTick() : 0u;
    wq_blocked = kind;
    __sync_synchronize();
    LightEvent_Signal(&wq_ev_walker);
    while (!cond(c)) LightEvent_Wait(&wq_ev_logic);
    wq_blocked = 0;
    if (pf) {
        unsigned d = (unsigned)svcGetSystemTick() - t0;
        wq_bw[0] += d;
        if (cond == c_walked) wq_bw[1] += d;
    }
}
void pm_wq_pf(unsigned out[2]) { out[0] = wq_bw[0]; out[1] = wq_bw[1]; }
static int c_space(void* c) { (void)c; return wq_tail - wq_head < WQ_N; }
static int c_empty(void* c) { (void)c; return wq_head == wq_tail; }
static int c_walked(void* c) { (void)c; return wq_fr_walked == wq_fr_push; }
static int c_done(void* c) { return *(volatile int*)c != 0; }

static __attribute__((cold, noinline)) void wq_too_big(unsigned size) {
    pm_log_e("wq: message of %u bytes, the queue holds %u", size, WQ_ARG);
    pm_fatal_stop("wq: message too big");
}
static void wq_put(void (*fn)(void*), const void* arg, unsigned size) {
    if (size > WQ_ARG) wq_too_big(size);
    if (wq_tail - wq_head == WQ_N) { if (pm_wq_on_logic()) pm_wq_wqf++; wq_block(c_space, NULL, WQB_ALL); }
    unsigned t = wq_tail;
    WqEnt* e = &wq_ring[t & (WQ_N - 1u)];
    e->fn = fn;
    e->frame = pm_frame_n;
    if (size) memcpy(e->arg, arg, size);
    __sync_synchronize();
    wq_tail = t + 1u;
}
void pm_lg_quiesce(void); /* pm_logic.c */
/* Game thread outside a walk: the logic thread stops at a step boundary, the queue is run, then fn runs on the walker side. */
static void wq_main_direct(void (*fn)(void*), void* arg) {
    pm_lg_quiesce();
    wq_pump();
    pm_walk_frame = pm_frame_n;
#ifdef PM_FNCOV
    { extern void pm_fncov_walk(unsigned f); pm_fncov_walk(pm_frame_n); } /* HC0 census lane only */
#endif
    pm_walk_side++;
    fn(arg);
    pm_walk_side--;
}

void pm_wq_post(void (*fn)(void*), void* arg, unsigned size) {
    if (pm_wq_on_logic()) { wq_put(fn, arg, size); return; }
    if (pm_walk_side) { pm_walk_side++; fn(arg); pm_walk_side--; return; } /* the walker's own call: no queue */
    wq_main_direct(fn, arg);
}

static void wq_frame_run(void* a) { pm_frame_walk((const PmFrame*)a); __sync_synchronize(); wq_fr_walked++; }
void pm_wq_frame(const PmFrame* f) {
    if (pm_wq_on_logic()) {
        if (wq_fr_walked != wq_fr_push) wq_block(c_walked, NULL, WQB_ALL); /* depth 1: the frame before is walked first */
        if (nd_c2_on > 0) nd_c2_push(); /* CORE2C-3: the census check point of the threaded walk */
        wq_fr_push++;
        wq_put(wq_frame_run, f, sizeof *f);
        return;
    }
    wq_fr_push++;
    wq_main_direct(wq_frame_run, (void*)f);
}

/* 60 fps: the logic of step N+1 runs while the walker draws frame N and key frame N. Before it writes the replacement table
 * (pm_interp_lerp: rtab and repl, read by the walker through pm_interp_find) it waits until both are walked; the wait runs the queue
 * up to them only (WQB_WALKED), not the begin entry of step N+1 behind them. A key frame is a frame in the count (wq_fr_push). */
void pm_wq_frame_slot(void) {
    if (pm_wq_on_logic() && wq_fr_walked != wq_fr_push) wq_block(c_walked, NULL, WQB_WALKED);
}
static void wq_key_run(void* a) { pm_key_show((const PmKey*)a); __sync_synchronize(); wq_fr_walked++; }
void pm_wq_key(const PmKey* k) {
    wq_fr_push++;
    if (pm_wq_on_logic()) { wq_put(wq_key_run, k, sizeof *k); pm_wq_kick_walker(); return; }
    wq_main_direct(wq_key_run, (void*)k);
}

/* pm_walk_drain (nd_walk.c) calls this first: return when the queue is empty and no walk runs. */
void pm_wq_idle(void) {
    if (pm_wq_on_logic()) {
        pm_wq_dr++;
        if (wq_head != wq_tail) { pm_wq_drw++; wq_block(c_empty, NULL, WQB_ALL); }
        return;
    }
    if (!pm_walk_side) { pm_lg_quiesce(); wq_pump(); }
}

typedef struct { void (*fn)(void*); void* arg; volatile int* done; } WqCall;
static void wq_call_run(void* a) { WqCall* c = (WqCall*)a; c->fn(c->arg); __sync_synchronize(); *c->done = 1; }
void pm_wq_call(void (*fn)(void*), void* arg, unsigned size) {
    (void)size;
    if (pm_wq_on_logic()) { /* fn runs on the walker after the queued entries; arg stays the caller's (results) */
        volatile int done = 0;
        WqCall c = { fn, arg, &done };
        wq_put(wq_call_run, &c, sizeof c);
        wq_block(c_done, (void*)&done, WQB_ALL);
        return;
    }
    if (pm_walk_side) { pm_walk_side++; fn(arg); pm_walk_side--; return; }
    wq_main_direct(fn, arg);
}

#define OWN_N 32
__attribute__((cold, noinline)) void pm_walk_own(const char* fn) {
    static const char* seen[OWN_N];
    static unsigned n;
    for (unsigned i = 0; i < n; i++) if (seen[i] == fn) return;
    if (n < OWN_N) seen[n++] = fn;
    pm_log_w("c2own: %s ran outside the walker side (frame %u)", fn, pm_frame_count());
}
