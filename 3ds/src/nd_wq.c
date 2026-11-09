/* CORE2-3a: the walker side. The walker owns nd_ws, pm_gpu (C3D context, command buffers, GX queue, frame begin and end, present,
 * shots, grabs, captures, depth reads) and pm_tex (texture cache). The game side reaches it only through the calls below
 * (plan_core2.md, CORE2-2 design; row_CORE2C.md):
 *   pm_wq_post(fn, arg, size): a message. The arg bytes are copied into the queue; the walker runs fn in queue order. The caller
 *     does not wait.
 *   pm_wq_frame(frame): one frame (background and main task, colour buffer, flags) into the same queue. Depth 1: while a frame is
 *     queued and not walked, the push waits. So a display context is built again only after the walk of the frame that used it.
 *   pm_wq_call(fn, arg, size): the caller needs a result or the walker output (depth read, exit): wait until the queue is empty,
 *     then run fn.
 *   pm_walk_drain(why) (nd_walk.c): the game side is about to write memory a queued or running walk reads: wait until the queue is
 *     empty.
 * CORE2C-2: one thread. The poster is also the walker: a post or a frame push runs the queue at once (wq_pump), so the order of walks,
 * messages and game code is the serial order (det keeps this order when the walker gets its own thread).
 * pm_walk_side counts the walker-side code running now (a walk, or fn of a message); a post from there runs at once (the walker's
 * own nested calls). Switch c2own: the owned entry points (PM_WALK_OWN in pm_gpu.c and pm_tex.c) log once each when they run
 * outside the walker side, so a direct game-side call is found. */
#include <string.h>
#include "pm_port.h"

int pm_walk_side;
int pm_c2own = -1;

/* The queue: a ring of copied messages, power-of-two size, free-running head (next to run) and tail (next free). 40 bytes an entry. */
#define WQ_N 32u
#define WQ_ARG 32u
typedef struct { void (*fn)(void*); unsigned size; unsigned long long arg[WQ_ARG / 8]; } WqEnt;
static WqEnt wq_ring[WQ_N];
static unsigned wq_head, wq_tail;
static unsigned wq_frames; /* frames queued and not walked: 0 or 1 */
_Static_assert(sizeof(PmFrame) <= WQ_ARG, "a frame fits one queue entry");

static void wq_frame_run(void* a) { pm_frame_walk((const PmFrame*)a); wq_frames--; }
static void wq_pump(void) { /* the walker: run every queued entry in order */
    while (wq_head != wq_tail) {
        WqEnt* e = &wq_ring[wq_head & (WQ_N - 1u)];
        pm_walk_side++;
        e->fn(e->arg);
        pm_walk_side--;
        wq_head++;
    }
}
static __attribute__((cold, noinline)) void wq_too_big(unsigned size) {
    pm_log_e("wq: message of %u bytes, the queue holds %u", size, WQ_ARG);
    pm_fatal_stop("wq: message too big");
}
static void wq_put(void (*fn)(void*), const void* arg, unsigned size) {
    if (size > WQ_ARG) wq_too_big(size);
    if (wq_tail - wq_head == WQ_N) wq_pump(); /* full: the walker frees it */
    WqEnt* e = &wq_ring[wq_tail & (WQ_N - 1u)];
    e->fn = fn;
    e->size = size;
    if (size) memcpy(e->arg, arg, size);
    wq_tail++;
}

void pm_wq_post(void (*fn)(void*), void* arg, unsigned size) {
    if (pm_walk_side) { pm_walk_side++; fn(arg); pm_walk_side--; return; } /* the walker's own call: no queue */
    wq_put(fn, arg, size);
    wq_pump();
}

void pm_wq_frame(const PmFrame* f) {
    while (wq_frames) wq_pump(); /* depth 1: the frame before is walked first */
    wq_frames++;
    wq_put(wq_frame_run, f, sizeof *f);
    wq_pump();
}

/* pm_walk_drain (nd_walk.c) calls this first: return when the queue is empty and no walk runs. */
void pm_wq_idle(void) { wq_pump(); }

void pm_wq_call(void (*fn)(void*), void* arg, unsigned size) {
    (void)size;
    if (!pm_walk_side) pm_walk_drain("call");
    pm_walk_side++;
    fn(arg);
    pm_walk_side--;
}

#define OWN_N 32
__attribute__((cold, noinline)) void pm_walk_own(const char* fn) {
    static const char* seen[OWN_N];
    static unsigned n;
    for (unsigned i = 0; i < n; i++) if (seen[i] == fn) return;
    if (n < OWN_N) seen[n++] = fn;
    pm_log_w("c2own: %s ran outside the walker side (frame %u)", fn, pm_frame_count());
}
