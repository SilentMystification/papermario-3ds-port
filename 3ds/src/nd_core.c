/* nd_core.c: frame driver of the native draw path (plan_native_slices S1-2, PNS 6). The record array and the LEGACY bridge are deleted
 * (S10-2): pm_nusys.c walks the task's words with pm_gbi_run directly. */
#include <stdint.h>
#include "nd.h"
#include "nd_snap.h" /* NWS4B */

extern unsigned nd_native_draws, nd_native_refused;
extern int pm_debug_has(const char* key);
extern void pm_log_d(const char* fmt, ...);
extern unsigned pm_frame_count(void);
/* debug switch ndst: the native state (nd_c0) once per 100 frames */
unsigned nd_rect_tex_n, nd_rect_fill_n; /* handler calls (CORE-3 census); incremented by nd_rect.c, defined here so the host test links without nd_rect.c (ST) */
static __attribute__((cold, noinline)) void nd_st_log(void) { /* NWS4B */
    static int on = -1;
    if (on < 0) on = pm_debug_has("ndst");
    if (!on || pm_frame_count() % 100u) return;
    pm_log_d("ndst native %u refused %u f%u geom %08x omh %08x oml %08x kh %08x kl %08x comb %08x %08x prim %08x lod %04x rect tex %u fill %u", nd_native_draws, nd_native_refused, pm_frame_count(), nd_c0.geom, nd_ap->in.omode_h, nd_ap->oml, /* NWS4B */
             nd_c0.om_kh, nd_c0.om_kl, nd_ap->in.w0, nd_ap->in.w1, nd_ap->in.prim, nd_ap->plod, nd_rect_tex_n, nd_rect_fill_n); /* NWS4B */
    nd_native_draws = nd_native_refused = 0;
}
void nd_frame_begin(void) { nd_st_log(); }
