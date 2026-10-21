/* Host test of the AU2 ring (3ds/src/pm_au2.c): random stress, no loss, no reorder, forced full ring, drain.
 * Build and run: sh 3ds/tests/au2/run.sh */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include "pm_au2.h"

/* ---- shim ---- */
void LightSemaphore_Init(LightSemaphore* s, int initial, int max) { pthread_mutex_init(&s->m, 0); pthread_cond_init(&s->c, 0); s->n = initial; s->max = max; }
void LightSemaphore_Acquire(LightSemaphore* s, int n) { pthread_mutex_lock(&s->m); while (s->n < n) pthread_cond_wait(&s->c, &s->m); s->n -= n; pthread_mutex_unlock(&s->m); }
int LightSemaphore_TryAcquire(LightSemaphore* s, int n) { int r = -1; pthread_mutex_lock(&s->m); if (s->n >= n) { s->n -= n; r = 0; } pthread_mutex_unlock(&s->m); return r; }
void LightSemaphore_Release(LightSemaphore* s, int n) { pthread_mutex_lock(&s->m); s->n += n; if (s->n > s->max) { fprintf(stderr, "FAIL semaphore over max\n"); exit(1); } pthread_cond_broadcast(&s->c); pthread_mutex_unlock(&s->m); }
void LightEvent_Init(LightEvent* e, int type) { pthread_mutex_init(&e->m, 0); pthread_cond_init(&e->c, 0); e->set = 0; e->sticky = type == RESET_STICKY; }
void LightEvent_Wait(LightEvent* e) { pthread_mutex_lock(&e->m); while (!e->set) pthread_cond_wait(&e->c, &e->m); if (!e->sticky) e->set = 0; pthread_mutex_unlock(&e->m); }
void LightEvent_Clear(LightEvent* e) { pthread_mutex_lock(&e->m); e->set = 0; pthread_mutex_unlock(&e->m); }
void LightEvent_Signal(LightEvent* e) { pthread_mutex_lock(&e->m); e->set = 1; pthread_cond_broadcast(&e->c); pthread_mutex_unlock(&e->m); }
void LightLock_Init(LightLock* l) { pthread_mutex_init(l, 0); }
void LightLock_Lock(LightLock* l) { pthread_mutex_lock(l); }
void LightLock_Unlock(LightLock* l) { pthread_mutex_unlock(l); }
static __thread Thread cur;
static void* tramp(void* p) { Thread t = p; cur = t; t->fn(t->arg); return 0; }
Thread threadCreate(void (*fn)(void*), void* arg, size_t stack, int prio, int core, int detached) {
    (void)stack; (void)prio; (void)core; (void)detached;
    Thread t = calloc(1, sizeof *t); t->fn = fn; t->arg = arg;
    pthread_create(&t->t, 0, tramp, t);
    return t;
}
Thread threadGetCurrent(void) { return cur; }
int threadJoin(Thread t, u64 timeout) { (void)timeout; pthread_join(t->t, 0); return 0; }
void threadFree(Thread t) { free(t); }
int svcGetThreadPriority(s32* out, int h) { (void)h; *out = 0x30; return 0; }
u32 svcGetProcessorID(void) { return 1; }
unsigned pm_ticks(void) { return 0; }
int pm_debug_has(const char* s) { return s[0] == 'a' && s[1] == 'u' && s[2] == 'd'; }
void pm_log(const char* f, ...) { (void)f; }
void pm_log_d(const char* f, ...) { (void)f; }
void pm_log_w(const char* f, ...) { (void)f; }
void pm_crash_install(void* s, unsigned n) { (void)s; (void)n; }
void pm_stack_register(int id, const char* n, unsigned s) { (void)id; (void)n; (void)s; }
void pm_fpscr_setup(const char* w) { (void)w; }

/* ---- test ---- */
static unsigned long long rng = 88172645463325252ull;
static unsigned rnd(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return (unsigned)(rng >> 32); }
static uint32_t next_expect;
static int slow;
static unsigned crng = 12345;
static int fails; /* only the render thread writes it during the run */
void au2_exec(const AuCmd* c) {
    uint32_t ne = __atomic_load_n(&next_expect, __ATOMIC_RELAXED);
    if ((uint32_t)c->v0 != ne) { if (fails++ < 5) fprintf(stderr, "FAIL order: got %u expected %u\n", (unsigned)c->v0, ne); }
    if (c->v1 != (int32_t)(c->v0 * 2654435761u) || c->idx != (uint8_t)c->v0 || c->s1 != (int16_t)(c->v0 >> 3)) { if (fails++ < 5) fprintf(stderr, "FAIL payload at %u\n", (unsigned)c->v0); }
    __atomic_add_fetch(&next_expect, 1, __ATOMIC_RELEASE);
    crng = crng * 1664525u + 1013904223u;
    if (__atomic_load_n(&slow, __ATOMIC_RELAXED) && ((crng >> 16) & 1023) == 0) usleep(200);
}
void au2_post_exec(void* p, unsigned b) { (void)p; (void)b; }
void au2_clean_dma_exec(void) {}

int main(void) {
    if (!pm_au2_start() || !pm_au2_active) { fprintf(stderr, "FAIL start\n"); return 1; }
    uint32_t seq = 0;
    for (int round = 0; round < 3000; round++) {
        __atomic_store_n(&slow, (round % 3) == 0, __ATOMIC_RELAXED); /* a slow consumer fills the ring */
        unsigned burst = rnd() % 700; /* more than one slot (256) and more than the ring sometimes */
        for (unsigned i = 0; i < burst; i++) {
            AuCmd c = { AUC_PITCH, (uint8_t)seq };
            c.v0 = (int32_t)seq; c.v1 = (int32_t)(seq * 2654435761u); c.s1 = (int16_t)(seq >> 3);
            pm_au2_emit(&c);
            seq++;
            if ((rnd() & 63) == 0) pm_au2_commit();
        }
        unsigned r = rnd() % 8;
        if (r == 0) {
            pm_au2_sync(AU2_SYNC_BANK);
            if (__atomic_load_n(&next_expect, __ATOMIC_ACQUIRE) != seq) { fprintf(stderr, "FAIL drain: executed %u of %u\n", next_expect, seq); fails++; }
        } else if (r < 3) {
            pm_au2_commit();
        }
    }
    pm_au2_stop();
    if (next_expect != seq) { fprintf(stderr, "FAIL end: executed %u of %u\n", next_expect, seq); fails++; }
    printf("au2 queue test: %u commands, %s\n", seq, fails ? "FAIL" : "ok (no loss, no reorder, drain exact)");
    return fails ? 1 : 0;
}
