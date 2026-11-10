/* CRASHALL: thread creation with the crash handler. One pool slot per live thread: the slot holds the thread's name, its
 * handler stack (PM_TH_HSTACK bytes) and the Thread handle the handler looks up. A thread that returns frees its slot. */
#include "pm_port.h"
#include "pm_thread.h"
#include <string.h>

/* 17 port threads that can live at once (game, main, logic, audio, au_render, prefetch, rs_loader, flash, savecrash,
 * log_writer, log_step x2, log_sink, c2, c1, wdog) + 2 spare. The handler's deepest static chain is 3792 B
 * (crash_handler -> pm_exit_process -> pm_flash_crash_flush -> pm_crash_line -> drain_locked -> out_batch -> pm_log_w ->
 * vlog -> vsnprintf -> _svfprintf_r -> _dtoa_r), measured on the ELF: 4 KB keeps 300 B of margin. */
#define PM_TH_MAX 19
#define PM_TH_HSTACK 4096

typedef struct {
    ThreadFunc fn;
    void* arg;
    const char* name;
    Thread th; /* set by the thread itself (threadGetCurrent) before its handler is installed */
    int used;
    u8 hstack[PM_TH_HSTACK] __attribute__((aligned(8)));
} PmTh;

static PmTh pool[PM_TH_MAX];

static PmTh* slot_take(void) {
    for (int i = 0; i < PM_TH_MAX; i++)
        if (__atomic_exchange_n(&pool[i].used, 1, __ATOMIC_ACQ_REL) == 0) return &pool[i];
    return NULL;
}

static void slot_free(PmTh* s) {
    s->th = NULL;
    __atomic_store_n(&s->used, 0, __ATOMIC_RELEASE);
}

static void th_entry(void* p) {
    PmTh* s = (PmTh*)p;
    s->th = threadGetCurrent();
    pm_crash_install(s->hstack, sizeof s->hstack);
    /* crashtest=<thread name>: this thread does one null write, to check the crash path of that thread (CRASHALL test). */
    char v[32];
    if (pm_debug_value("crashtest", v, sizeof v) && strcmp(v, s->name) == 0) *(volatile u32*)0 = 0;
    s->fn(s->arg);
    slot_free(s);
}

Thread pm_thread_create(ThreadFunc fn, void* arg, size_t stack, int prio, int core, const char* name) {
    PmTh* s = slot_take();
    if (!s) {
        pm_log_w("thread %s: no crash slot (%d), not created", name, PM_TH_MAX);
        return NULL;
    }
    s->fn = fn;
    s->arg = arg;
    s->name = name;
    s->th = NULL;
    Thread t = threadCreate(th_entry, s, stack, prio, core, false);
    if (!t) slot_free(s);
    return t;
}

void pm_thread_adopt(const char* name) {
    PmTh* s = slot_take();
    if (!s) return;
    s->name = name;
    s->th = threadGetCurrent();
    pm_crash_install(s->hstack, sizeof s->hstack);
}

const char* pm_thread_name_current(void) {
    Thread me = threadGetCurrent();
    for (int i = 0; i < PM_TH_MAX; i++)
        if (__atomic_load_n(&pool[i].used, __ATOMIC_ACQUIRE) && pool[i].th == me) return pool[i].name;
    return NULL;
}
