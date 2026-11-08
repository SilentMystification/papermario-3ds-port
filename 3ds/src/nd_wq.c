/* CORE2-3a: the walker side. The walker owns nd_ws, pm_gpu (C3D context, command buffers, GX queue, frame begin and end, present,
 * shots, grabs, captures, depth reads) and pm_tex (texture cache). The game thread reaches it only through the calls below
 * (plan_core2.md, CORE2-2 design):
 *   pm_wq_post(fn, arg): a message. The walker runs fn before its next task, in post order. The caller does not wait.
 *   pm_wq_call(fn, arg): the caller needs a result or the walker output (depth read, exit): drain the walker, then run fn.
 *   pm_walk_drain(why) (nd_walk.c): the game thread is about to write memory the last walk read.
 * Serial walker (today): every task is walked inside nuGfxTaskStart, so the walker is idle between tasks and both calls run fn at
 * once on the game thread, in program order: the same order and the same pictures as a direct call. CORE2-3c changes only this file
 * and pm_walk_drain: pm_wq_post queues, pm_wq_call and pm_walk_drain wait for the walker queue.
 * pm_walk_side counts the walker-side code running now (a walk, or fn of a message). Switch c2own: the owned entry points (PM_WALK_OWN
 * in pm_gpu.c and pm_tex.c) log once each when they run outside the walker side, so a direct game-thread call is found. */
#include "pm_port.h"

int pm_walk_side;
int pm_c2own = -1;
void pm_walk_drain(const char* why); /* nd_walk.c */

/* size: the bytes of arg (CORE2-3c copies a posted message into the queue). Code already on the walker side runs fn at once. */
void pm_wq_post(void (*fn)(void*), void* arg, unsigned size) {
    (void)size;
    pm_walk_side++;
    fn(arg);
    pm_walk_side--;
}

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
