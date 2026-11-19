/* nd_msnap.c: the model snap, core 1 record and core 0 compare (NREC1A). Log lines: `msnap f<frame> cmp ...` (per 100 frames) and `msnap bad ...`. */
#include <stdint.h>
#include <string.h>
#include "ultra64.h"
#include "nd.h"
#include "nd_ds.h"
#include "nd_api.h"
#include "nd_snap.h"
#include "nd_msnap.h"
#include "nd_msnapw.h"
#include "pm_port.h"

extern unsigned pm_frame_n;
extern unsigned fogc, blendc;
static uint32_t ms_pre[5]; void nd_msnap_pre(const NdDs* s) { ms_pre[0] = s->w.omode_h; ms_pre[1] = s->w.omode_l; ms_pre[2] = s->w.om_kh; ms_pre[3] = s->w.om_kl; ms_pre[4] = s->w.geom; } /* NAT2C */
unsigned pm_gpu_env_now(void);

/* core 1: the snap follows the record in its chunk; flag bit 0 of the record header arg only when it does. */
void nd_msnap_rec(uint32_t* p, unsigned units) {
    NdMSnap* m = (NdMSnap*)(p + 2u + 2u * units + 2u);
    nd_msnap_take(&nd_ds, m);
    m->k_omh = ms_pre[0]; m->k_oml = ms_pre[1]; m->k_kh = ms_pre[2]; m->k_kl = ms_pre[3]; m->k_geom = ms_pre[4]; /* NAT2C */
    p[0] |= 1u; /* NREC1B: nd_rec_model reserved the room */
}

static unsigned ms_nos, ms_nlog;
static NdDs ms_d;
static NdMSnap ms_wk; static const NdMSnap* ms_cur = &ms_wk; /* NREC1B: the snap of the record that runs now */
static const NdMSnap* ms_of(const uint32_t* q) { const uint32_t h = q[-2]; const NdMSnap* s = (const NdMSnap*)(q + 2u * ((h >> 16) & 0xffu) + 2u); return ((h & 1u) && s->magic == ND_MSNAP_MAGIC) ? s : NULL; }
const uint32_t* nd_msnap_key(const uint32_t* q) { const NdMSnap* s = ms_of(q); return s ? &s->k_omh : NULL; } /* NAT2C */
void nd_msnap_use(const uint32_t* q) { /* NREC1B: no snap: the walker inputs (fixed fallback, counted in nos) */
    const NdMSnap* s = ms_of(q);
    if (!s) { ms_nos++; ms_d.w = nd_ws; ms_d.fogc = fogc; ms_d.blendc = blendc; nd_msnap_take(&ms_d, &ms_wk); ms_wk.envc = pm_gpu_env_now(); s = &ms_wk; if (ms_nlog++ < 8u) pm_log_evt("msnap nos f%u", pm_frame_n); }
    ms_cur = s;
}
const uint32_t* nd_msnap_w(void) { return &ms_cur->magic; }
const NdTd* nd_msnap_td0(void) { return &ms_cur->b[0].td; }

