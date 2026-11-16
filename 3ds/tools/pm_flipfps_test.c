/* FLIPLOG host test: the flipfps row of pm_flipfps.c and its switch gate. Prints "fliplog ok 6/6". */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "pm_flipfps.h"
unsigned pm_dbg_gen = 1;
static int sw;
int pm_debug_has(const char* word) { return sw && !strcmp(word, "flipfps"); }
static char last[256];
static int nline;
void pm_log_live_line(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vsnprintf(last, sizeof last, fmt, ap); va_end(ap); nline++; }
static int pass, fail;
static void check(int c, const char* what) { if (c) pass++; else { fail++; printf("fliplog FAIL: %s (last \"%s\", lines %d)\n", what, last, nline); } }
/* one drawn frame in the hook form of pm_gpu.c: frame begin, wait at C3D_FrameBegin, frame end */
static void frame(unsigned tb, unsigned t0, unsigned t1, unsigned gpu, unsigned f, unsigned te, int off, int soff, int spl) {
    if (__builtin_expect(pm_ff_on(), 0)) pm_ff_begin(tb);
    const int ff_on = pm_ff_on(); const unsigned ff_t0 = ff_on ? t0 : 0u;
    if (__builtin_expect(ff_on, 0)) pm_ff_wait(ff_t0, t1, gpu);
    if (__builtin_expect(pm_ff_on(), 0)) pm_ff_end(f, te, off, soff, spl, 3);
}
int main(void) {
    check(pm_ff_us(0u) == 0u && pm_ff_us(134056u) == 500u && pm_ff_us(268112u) == 1000u && pm_ff_us(26811186u) == 100000u, "tick to us");
    for (unsigned k = 0; k < 4; k++) frame(1000u * k, 2000u * k, 2000u * k + 268112u, 5u, k, 1000u * k + 804336u, 4, 1, 2);
    check(nline == 0, "switch off prints nothing");
    sw = 1; pm_dbg_gen++;
    frame(1000u, 2000u, 270112u, 5u, 7u, 805336u, 4, 1, 2);
    check(nline == 0, "no row before the next frame begin");
    frame(900000u, 900000u, 1034056u, 12345u, 8u, 1436224u, 2, 0, 0);
    check(nline == 1 && !strcmp(last, "flipfps f7 cpu 2000 wait 500 gpu 12345 off 4 soff 1 spl 2 reuse 1"), "row f7");
    frame(4294967000u, 4294967000u, 4294967000u, 33000u, 9u, 1072152u, 5, 4, 3);
    check(nline == 2 && !strcmp(last, "flipfps f8 cpu 1500 wait 0 gpu 33000 off 2 soff 0 spl 0 reuse 0"), "row f8");
    frame(2000000u, 2000000u, 4681120u, 1u, 10u, 2100000u, 0, 0, 0);
    check(nline == 3 && !strcmp(last, "flipfps f9 cpu 4000 wait 10000 gpu 1 off 5 soff 4 spl 3 reuse 2"), "row f9 (tick wrap)");
    printf("fliplog %s %d/%d\n", fail ? "FAIL" : "ok", pass, pass + fail);
    return fail ? 1 : 0;
}
