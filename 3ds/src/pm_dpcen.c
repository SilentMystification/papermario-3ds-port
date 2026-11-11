/* DPCEN2: resident bytes per pool and the load-time transform counts (switch dpcen, after the DPCEN1 byte line).
 * dpres line: the heap in use, the free linear and VRAM space, the model cache entries and pooled draws (nd_mc_fill), and the counts and
 * microseconds of the sprite archive decodes (spr_load_sprite) and the static list compiles (nd_dlc_compile) since the last line.
 * Debug only: reads pool state, changes no draw. */
#include <3ds.h>
#include <malloc.h>
#include "pm_port.h"

void nd_mc_fill(unsigned* ent, unsigned* pool); /* nd_model.c, DOORHITCH */

unsigned pm_dp_spr_n, pm_dp_spr_us, pm_dp_cmp_n, pm_dp_cmp_us;

void pm_dp_res_line(unsigned f) {
    unsigned ent = 0, pool = 0;
    nd_mc_fill(&ent, &pool);
    pm_log_evt("dpres f%u heapkb %u linfreekb %u vramfreekb %u mcent %u mcpool %u sprload %u sprus %u compiles %u compus %u",
               f, (unsigned)(mallinfo().uordblks >> 10), (unsigned)(linearSpaceFree() >> 10), (unsigned)(vramSpaceFree() >> 10),
               ent, pool, pm_dp_spr_n, pm_dp_spr_us, pm_dp_cmp_n, pm_dp_cmp_us);
    pm_dp_spr_n = 0; pm_dp_spr_us = 0; pm_dp_cmp_n = 0; pm_dp_cmp_us = 0;
}
