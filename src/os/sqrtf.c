#include "ultra64.h"

/* SIZE-3: with -Os, GCC compiles __builtin_sqrtf in this file as a call to sqrtf, so the function calls itself (the
 * -Os lane build starts sqrtf with "b ." at 0x0018c580) and every caller hangs. On 3DS the VFP instruction is used. */
f32 sqrtf(f32 f) {
    f32 r;

    __asm__("vsqrt.f32 %0, %1" : "=t"(r) : "t"(f));
    return r;
}
