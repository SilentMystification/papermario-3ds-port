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
