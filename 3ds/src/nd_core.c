/* nd_core.c: frame driver of the native draw path (plan_native_slices S1-2, PNS 6). The record array and the LEGACY bridge are deleted
 * (S10-2): pm_nusys.c walks the task's words with pm_gbi_run directly. */
#include <stdint.h>
#include "nd.h"

extern unsigned nd_native_draws, nd_native_refused;
extern int pm_debug_has(const char* key);
extern void pm_log_d(const char* fmt, ...);
extern unsigned pm_frame_count(void);
/* debug switch ndst: the native state (nd_ws) once per 100 frames */
unsigned nd_rect_tex_n, nd_rect_fill_n; /* handler calls (CORE-3 census); incremented by nd_rect.c, defined here so the host test links without nd_rect.c (ST) */
static __attribute__((cold, noinline)) void nd_st_log(void) {
    static int on = -1;
    if (on < 0) on = pm_debug_has("ndst");
    if (!on || pm_frame_count() % 100u) return;
    pm_log_d("ndst native %u refused %u f%u geom %08x omh %08x oml %08x kh %08x kl %08x comb %08x %08x prim %08x lod %04x rect tex %u fill %u", nd_native_draws, nd_native_refused, pm_frame_count(), nd_ws.geom, nd_ws.omode_h, nd_ws.omode_l,
             nd_ws.om_kh, nd_ws.om_kl, nd_ws.comb_w0, nd_ws.comb_w1, nd_ws.prim, nd_ws.prim_lod, nd_rect_tex_n, nd_rect_fill_n);
    nd_native_draws = nd_native_refused = 0;
}
void nd_frame_begin(void) { nd_st_log(); }
