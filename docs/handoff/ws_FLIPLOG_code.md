# ws_FLIPLOG_code: the exact edits of ws_FLIPLOG (verified on main 71ea9565ca)
pm_gpu.c and pm_log.c have CRLF line ends. Keep CRLF. The new files can use LF.
## 1. New file 3ds/include/pm_flipfps.h
```c
/* FLIPLOG: switch flipfps (off by default). One live log row per drawn frame (pm_flipfps.c). */
#ifndef PM_FLIPFPS_H
#define PM_FLIPFPS_H
extern unsigned pm_dbg_gen; /* pm_log.c */
int pm_debug_has(const char* word); /* pm_log.c */
void pm_log_live_line(const char* fmt, ...); /* pm_log.c: a line for the live log only, never the console */
/* the switch, cached until the switch string changes (the PMDL form of pm_port.h:68) */
static inline int pm_ff_on(void) {
    static int v; static unsigned g;
    if (g != pm_dbg_gen) { v = pm_debug_has("flipfps"); g = pm_dbg_gen; }
    return v;
}
unsigned pm_ff_us(unsigned ticks);
void pm_ff_begin(unsigned t);
void pm_ff_wait(unsigned t0, unsigned t1, unsigned gpu_us);
void pm_ff_end(unsigned f, unsigned t, int off, int soff, int spl, int offn);
#endif
```
## 2. New file 3ds/src/pm_flipfps.c
```c
/* FLIPLOG: switch flipfps. One live log row per drawn frame, printed at the begin of the next drawn frame:
 * flipfps f<frame> cpu <us> wait <us> gpu <us> off <n> soff <n> spl <n> reuse <n>
 * cpu: walker time from pm_gpu_begin_w to after C3D_FrameEnd, less the wait at its own frame begin.
 * wait: the wait at the next frame begin (sync60 pace and C3D_FrameBegin): there the CPU waits for the GPU of this frame.
 * gpu: C3D_GetDrawingTime() after the next C3D_FrameBegin (the value of the prof gpu field, pm_phase.c:78).
 * off, soff: full and 64 x 64 off-screen images. spl: frame splits. reuse: full images that took a used slot again (V05 demand). */
#include "pm_flipfps.h"

typedef struct { unsigned f, cpu, off, soff, spl, reuse; } FfRow;
static FfRow ff_row;
static int ff_have;
static unsigned ff_tb, ff_wait; /* this frame: the begin tick, the wait ticks at its own begin */

unsigned pm_ff_us(unsigned ticks) { return (unsigned)(((unsigned long long)ticks * 15644u) >> 22); } /* 268112 ticks = 1000 us, no division */
void pm_ff_begin(unsigned t) { ff_tb = t; }
void pm_ff_wait(unsigned t0, unsigned t1, unsigned gpu_us) {
    const unsigned w = t1 - t0;
    if (ff_have) {
        pm_log_live_line("flipfps f%u cpu %u wait %u gpu %u off %u soff %u spl %u reuse %u", ff_row.f, ff_row.cpu, pm_ff_us(w), gpu_us,
                         ff_row.off, ff_row.soff, ff_row.spl, ff_row.reuse);
        ff_have = 0;
    }
    ff_wait = w;
}
void pm_ff_end(unsigned f, unsigned t, int off, int soff, int spl, int offn) {
    ff_row.f = f;
    ff_row.cpu = pm_ff_us(t - ff_tb - ff_wait);
    ff_row.off = (unsigned)off; ff_row.soff = (unsigned)soff; ff_row.spl = (unsigned)spl;
    ff_row.reuse = off > offn ? (unsigned)(off - offn) : 0u;
    ff_have = 1;
}
```
## 3. 3ds/src/pm_gpu.c (4 edits; the comments must not name the pm_ff_ functions)
3a. Line 24. Before: `#include "pm_gpu_int.h"`. After: that line, then the new line `#include "pm_flipfps.h"`.
3b. pm_gpu_begin_w, line 2970. Before:
`    ndref_begin(); if (__builtin_expect(drec_win != -1, 0)) drec_begin();`
After: that line, then the new line
`    if (__builtin_expect(pm_ff_on(), 0)) pm_ff_begin(pm_ticks()); /* FLIPLOG: the frame begin tick */`
3c. pm_gpu_begin_wait, lines 2926 and 2929. Before:
`    unsigned fb_flags = 0; /* SYNC60: ...` (keep the line) and `    raw1_reset(); sh_clear(); C3D_FrameBegin(fb_flags);` (keep the line).
After: put this new line directly above the `unsigned fb_flags` line:
`    const int ff_on = pm_ff_on(); const unsigned ff_t0 = ff_on ? pm_ticks() : 0u; /* FLIPLOG: the wait for the GPU of the last frame starts here */`
Put this new line directly below the `C3D_FrameBegin(fb_flags);` line:
`    if (__builtin_expect(ff_on, 0)) pm_ff_wait(ff_t0, pm_ticks(), (unsigned)(C3D_GetDrawingTime() * 1000.f)); /* FLIPLOG: the wait and the GPU time of the last frame */`
3d. pm_gpu_end_w, line 3223. Before:
`    ndunit_scan(); rd_sync(); C3D_FrameEnd(GX_CMDLIST_FLUSH);`
After: that line, then the new line
`    if (__builtin_expect(pm_ff_on(), 0)) pm_ff_end(pm_frame_count(), pm_ticks(), off_used, off_sused, grab_split + kick_done, OFF_N); /* FLIPLOG: the counts of this frame */`
FACT: off_used, off_sused (pm_gpu.c:261), grab_split (:277, split_frame :301) and kick_done (:770, kick_split :781) are cleared only in pm_gpu_begin_wait (:2901, :2908, :2914).
## 4. 3ds/src/pm_log.c (4 edits)
4a. Line 505. Before: `... bit 1 fallback (str holds the formatted text) */`. After: `... bit 1 fallback (str holds the formatted text), bit 3 live only (FLIPLOG) */`.
4b. Line 585. Before: `    sl->flags = (u8)((prof_force ? 1 : 0) | (evt ? 4 : 0));`
After: `    sl->flags = (u8)((prof_force & 9) | (evt ? 4 : 0)); /* FLIPLOG: prof_force 1 = prof line, 9 = prof line for the live log only */`
4c. After the closing `}` of pm_log_prof_line (line 630), add:
```c
/* FLIPLOG: a profiling line for the live log only: never the console; log.txt only with prof_disk, as a prof line. */
void pm_log_live_line(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    prof_force = 9; vlog(1, fmt, ap); prof_force = 0;
    va_end(ap);
}
```
4d. Line 1002. Before: `            int tl = (evt || pad || lvl <= ll), tc = (pad || lvl <= cl); /* U2: ...`
After: replace only `tc = (pad || lvl <= cl);` with `tc = !(sl->flags & 8) && (pad || lvl <= cl);`. Keep the comment and add ` FLIPLOG: bit 3 skips the console.`
FACT: sl->flags readers are only pm_log.c:950 (bit 1), :998 (bit 0), :1000 (bit 2). prof_force is set only at :629.
## 5. New file 3ds/tools/pm_flipfps_test.c (the expected values are computed: never change one)
```c
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
```
## 6. New file 3ds/tests/host.d/fliplog.sh
```sh
#!/bin/sh
# FLIPLOG: the flipfps row (pm_flipfps.c) and the switch gate of its 3 hooks in pm_gpu.c, with a seeded fault.
gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/fliplog 3ds/tools/pm_flipfps_test.c 3ds/src/pm_flipfps.c || { echo "fliplog FAIL: compile"; exit 1; }
/tmp/fliplog || exit 1
chk() { n=$(grep -cE 'pm_ff_(begin|wait|end)\(' "$1"); g=$(grep -E 'pm_ff_(begin|wait|end)\(' "$1" | grep -cE '__builtin_expect\((pm_ff_on\(\)|ff_on), 0\)'); [ "$n" = 3 ] && [ "$g" = 3 ] && echo ok || echo bad; }
mkdir -p /tmp/fliplog_seed
sed 's/if (__builtin_expect(pm_ff_on(), 0)) pm_ff_begin/pm_ff_begin/' 3ds/src/pm_gpu.c > /tmp/fliplog_seed/pm_gpu.c
[ "$(chk /tmp/fliplog_seed/pm_gpu.c)" = bad ] || { echo "fliplog seed FAIL: the gate check misses a hook with no switch test"; exit 1; }
[ "$(chk 3ds/src/pm_gpu.c)" = ok ] || { echo "fliplog FAIL: pm_gpu.c needs 3 pm_ff hooks, each behind the flipfps switch"; exit 1; }
echo "fliplog gate ok"
```
## 7. New files 3ds/tools/azt.d/FLIPLOG.json and 3ds/tools/bench.d/FLIPLOG.json
Copy 3ds/tools/azt.d/MESH1.json and 3ds/tools/bench.d/MESH1.json. Change these values (both files where the key exists):
name and scenario `fliplog_fsel`. debug `fileselect prof=30 flipfps`. cap 25. deadline 20. first_marker `prof=1 log=off`. expects `none`.
until `flipfps f\\d+ cpu \\d+ wait \\d+ gpu \\d+ off \\d+ soff \\d+ spl \\d+ reuse \\d+`.
expected_events: `{"name": "prof on", "regex": "prof=1 log=off"}` and `{"name": "flipfps row", "regex": "<the until regex>"}`.
subject, proves, purpose, reason: `FLIPLOG: the capture launch words (prof=30 flipfps) put flipfps rows in log.txt (run_azahar.ps1:193 adds profdisk=1)`.
tags: `["azahar", "cold"]`.
