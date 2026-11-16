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
