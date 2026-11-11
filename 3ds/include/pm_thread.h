#ifndef PM_THREAD_H
#define PM_THREAD_H

#include <3ds.h>

/* CRASHALL: every port thread is created here. The new thread installs the CPU exception handler (pm_main.c crash_handler)
 * on its own handler stack from one static pool, so a fault on any port thread writes its [CRASH] trace and ends the process.
 * Returns NULL when the thread is not created (no pool slot, or threadCreate failed); the callers already handle NULL. */
Thread pm_thread_create(ThreadFunc fn, void* arg, size_t stack, int prio, int core, const char* name);

/* The calling thread (the 3DS main thread) gets a pool slot and the handler. */
void pm_thread_adopt(const char* name);

/* Name of the calling port thread for the [CRASH] line, NULL when the thread is not a port thread. */
const char* pm_thread_name_current(void);

/* STACKANY: bounds of the calling port thread's stack (lo, hi). Returns 1 when known, 0 when not (adopted thread). */
int pm_thread_stack_current(u32* lo, u32* hi);

#endif
