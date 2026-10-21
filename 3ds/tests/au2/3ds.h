/* Host shim of the libctru calls that pm_au2.c uses (pthreads). Only for 3ds/tests/au2. */
#ifndef AU2_SHIM_3DS_H
#define AU2_SHIM_3DS_H
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
typedef uint8_t u8; typedef int32_t s32; typedef uint32_t u32; typedef uint64_t u64; typedef int Result;
#define U64_MAX UINT64_MAX
#define CUR_THREAD_HANDLE 0
typedef struct { pthread_mutex_t m; pthread_cond_t c; int n; int max; } LightSemaphore;
typedef struct { pthread_mutex_t m; pthread_cond_t c; int set; int sticky; } LightEvent;
typedef pthread_mutex_t LightLock;
enum { RESET_ONESHOT, RESET_STICKY };
typedef struct Thread_s { pthread_t t; void (*fn)(void*); void* arg; } *Thread;
void LightSemaphore_Init(LightSemaphore* s, int initial, int max);
void LightSemaphore_Acquire(LightSemaphore* s, int n);
int LightSemaphore_TryAcquire(LightSemaphore* s, int n);
void LightSemaphore_Release(LightSemaphore* s, int n);
void LightEvent_Init(LightEvent* e, int type);
void LightEvent_Wait(LightEvent* e);
void LightEvent_Clear(LightEvent* e);
void LightEvent_Signal(LightEvent* e);
void LightLock_Init(LightLock* l);
void LightLock_Lock(LightLock* l);
void LightLock_Unlock(LightLock* l);
Thread threadCreate(void (*fn)(void*), void* arg, size_t stack, int prio, int core, int detached);
Thread threadGetCurrent(void);
int threadJoin(Thread t, u64 timeout);
void threadFree(Thread t);
int svcGetThreadPriority(s32* out, int h);
u32 svcGetProcessorID(void);
#endif
