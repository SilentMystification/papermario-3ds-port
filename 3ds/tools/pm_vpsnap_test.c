/* HWFIX: host test of pm_vp_snap (3ds/src/pm_vpsnap.c). Prints "vpsnap ok 5/5" or "vpsnap FAIL <name>" and returns 1. */
#include <stdio.h>
#include <string.h>
#include "ultra64.h"

Vp* pm_vp_snap(const Vp* v);
s32 gCurrentDisplayContextIndex;
static unsigned frame, warns;
unsigned pm_frame_count(void) { return frame; }
void pm_log_w(const char* fmt, ...) { (void)fmt; warns++; }

static Vp mk(int k) {
    Vp v;
    memset(&v, 0, sizeof v);
    v.vp.vscale[0] = (short)k;
    v.vp.vtrans[1] = (short)(3 * k);
    return v;
}
#define CHECK(c, name) do { if (!(c)) { printf("vpsnap FAIL %s\n", name); return 1; } } while (0)

int main(void) {
    Vp a = mk(1), b = mk(2), x = mk(129);
    Vp* pa;
    Vp* pb;
    Vp* p1;
    /* copy */
    pa = pm_vp_snap(&a);
    CHECK(pa != &a && memcmp(pa, &a, sizeof a) == 0, "copy");
    /* dedupe: a repeat of the last value gets the same slot */
    pb = pm_vp_snap(&b);
    CHECK(pm_vp_snap(&a) == pa && pm_vp_snap(&b) == pb, "dedupe");
    /* full: 126 more values fill the pool; the next one gets the original pointer and one warning */
    for (int k = 3; k <= 128; k++) { Vp v = mk(k); pm_vp_snap(&v); }
    warns = 0;
    CHECK(pm_vp_snap(&x) == &x && warns == 1, "full");
    /* frame: a new frame starts the pool again */
    frame++;
    CHECK(pm_vp_snap(&x) != &x, "frame");
    /* context: the other display context has its own pool */
    gCurrentDisplayContextIndex = 1;
    frame++;
    p1 = pm_vp_snap(&a);
    CHECK(p1 != pa && memcmp(p1, &a, sizeof a) == 0, "context");
    puts("vpsnap ok 5/5");
    return 0;
}
