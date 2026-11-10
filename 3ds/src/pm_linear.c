/* LINRACE: see pm_linear.h. One LightLock around each libctru linear call. No call of this file nests inside another, so the lock is not
 * recursive. The __wrap_ functions are the link-time wrap (CMakeLists --wrap=linearAlloc ...): a call from libctru or citro3d goes to the
 * same locked path, and the __real_ calls reach libctru directly. */
#include <3ds.h>
#include "pm_linear.h"

void* __real_linearAlloc(size_t size);
void* __real_linearMemAlign(size_t size, size_t align);
void __real_linearFree(void* p);
void* __real_linearRealloc(void* p, size_t size);

static LightLock lin_lock; /* zero is unlocked: _LOCK_T is int32_t and LightLock_Init sets 0 */

void* pm_linear_alloc(size_t size) {
    LightLock_Lock(&lin_lock);
    void* p = __real_linearAlloc(size);
    LightLock_Unlock(&lin_lock);
    return p;
}
void* pm_linear_memalign(size_t size, size_t align) {
    LightLock_Lock(&lin_lock);
    void* p = __real_linearMemAlign(size, align);
    LightLock_Unlock(&lin_lock);
    return p;
}
void pm_linear_free(void* p) {
    if (!p) return;
    LightLock_Lock(&lin_lock);
    __real_linearFree(p);
    LightLock_Unlock(&lin_lock);
}
void* pm_linear_realloc(void* p, size_t size) {
    LightLock_Lock(&lin_lock);
    void* q = __real_linearRealloc(p, size);
    LightLock_Unlock(&lin_lock);
    return q;
}

void* __wrap_linearAlloc(size_t size) { return pm_linear_alloc(size); }
void* __wrap_linearMemAlign(size_t size, size_t align) { return pm_linear_memalign(size, align); }
void __wrap_linearFree(void* p) { pm_linear_free(p); }
void* __wrap_linearRealloc(void* p, size_t size) { return pm_linear_realloc(p, size); }
