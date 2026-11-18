/* nd_msnap.c: the model snap, core 1 record and core 0 compare (NREC1A). Log lines: `msnap f<frame> cmp ...` (per 100 frames) and `msnap bad ...`. */
#include <stdint.h>
#include <string.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_ds.h"
#include "nd_api.h"
#include "nd_snap.h"
#include "nd_msnap.h"
#include "pm_port.h"

extern unsigned pm_frame_n;
extern unsigned fogc, blendc;
unsigned pm_gpu_env_now(void);

/* core 1: the snap follows the record in its chunk; flag bit 0 of the record header arg only when it does. */
void nd_msnap_rec(uint32_t* p, unsigned units) {
    NdMSnap* m = (NdMSnap*)nd_rec_data_st(sizeof(NdMSnap));
    nd_msnap_take(&nd_ds, m);
    if ((uint32_t*)m == p + 2u + 2u * units + 2u) p[0] |= 1u;
}

static unsigned ms_cmp, ms_nos, ms_bad, ms_wst, ms_genv, ms_nlog, ms_bucket;
static NdDs ms_d;

/* core 0: q is the record of rec_miss (nd_recs.c); its header is q[-2]. */
void nd_msnap_cmp(const uint32_t* q) {
    const uint32_t h = q[-2];
    const NdMSnap* s = (const NdMSnap*)(q + 2u * ((h >> 16) & 0xffu) + 2u);
    if (!(h & 1u) || s->magic != ND_MSNAP_MAGIC) { ms_nos++; return; }
    NdMSnap w; const char* nm[4] = { "", "", "", "" }; int nb;
    ms_d.w = nd_ws; ms_d.fogc = fogc; ms_d.blendc = blendc;
    nd_msnap_take(&ms_d, &w);
    ms_cmp++;
    if (nd_msnap_diff(s, &w, nm, 4, &nb)) ms_wst++;
    if (s->envc != pm_gpu_env_now()) ms_genv++;
    if (nb) {
        ms_bad++;
        if (ms_nlog++ < 8u) pm_log_evt("msnap bad f%u n %d %s %s %s %s", pm_frame_n, nb, nm[0], nm[1], nm[2], nm[3]);
    }
}

void nd_msnap_tick(void) {
    if (!PMDL("snapchk") || pm_frame_n / 100u == ms_bucket) return;
    ms_bucket = pm_frame_n / 100u;
    pm_log_evt("msnap f%u cmp %u nos %u bad %u wst %u genv %u", pm_frame_n, ms_cmp, ms_nos, ms_bad, ms_wst, ms_genv);
}
