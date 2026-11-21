/* nd_ublk.c: GD9A census of the status bar block (plan_gamedraw 1.6). Core 1 brackets draw_status_ui with a begin and an end record
 * (nd_ublk_begin, nd_ublk_end). The end record carries a key: FNV-1a of the recorder inputs inside the bracket (unit words; matrix and vertex
 * contents, not their pointers). Core 0 (nd_ublk_run) counts per site whether the key, the state hash and the emitted words repeat the last block
 * of that site. Prof only (pm_pf_on): nothing is hashed or timed otherwise. No draw change. */
#include <stdint.h>
#include <string.h>
#include <citro3d.h>
#include "pm_port.h"
#include "nd_api.h"
#include "nd_fs.h"

extern int pm_pf_on;                                     /* pm_phase.c */
extern unsigned pm_gpu_cmd_split_n(void);                /* pm_gpu.c */
extern unsigned pm_gpu_draws_frame(void);                /* pm_gpu.c, GD9A */
extern uint32_t pm_gpu_state_hash(void);                 /* pm_gpu.c, GD9A */
extern uint32_t pm_gpu_word_hash(unsigned from, unsigned to); /* pm_gpu.c, GD9A */
int is_host(unsigned int p);                             /* nd_walk.c: a pure range test */

/* ---- core 1: the key of the bracket ---- */
static uint32_t ub_key;
static unsigned ub_n, ub_site;
static void ub_w(uint32_t x) { for (int i = 0; i < 4; i++) { ub_key ^= (x >> (8 * i)) & 0xffu; ub_key *= 16777619u; } }
static void ub_bytes(const void* p, unsigned n) { const unsigned char* c = (const unsigned char*)p; for (unsigned i = 0; i < n; i++) { ub_key ^= c[i]; ub_key *= 16777619u; } }

/* one stream unit: the data behind a matrix or vertex pointer, not the pointer (it alternates per context) */
void nd_ublk_unit(uint32_t w0, uint32_t w1) {
    const unsigned id = w0 >> 24;
    ub_w(w0); ub_n++;
    if (id == ND_FS_MTX && is_host(w1)) ub_bytes((const void*)(uintptr_t)w1, 64u);
    else if (id == ND_FS_VTX && is_host(w1)) ub_bytes((const void*)(uintptr_t)w1, 16u * ((w0 >> 12) & 0xffu));
    else ub_w(w1);
}

/* a native record: its runner, then its units. One unit is an nd_xf record (a matrix pair, nd_rec_xf). */
void nd_ublk_napi(NdNapiRun run, const void* rec, unsigned units) {
    ub_w((uint32_t)(uintptr_t)run); ub_n++;
    if (units == 1u) { const uint32_t* r = (const uint32_t*)rec; nd_ublk_unit(r[0], r[1]); }
    else ub_bytes(rec, 8u * units);
}

void nd_ublk_begin(unsigned site) {
    uint32_t r[4] __attribute__((aligned(8))) = { 1u, site, 0u, 0u };
    nd_rec_napi(nd_ublk_run, r, 2u);
    ub_site = site; ub_key = 2166136261u; ub_n = 0u; nd_ub_on = pm_pf_on;
}

void nd_ublk_end(void) {
    uint32_t r[4] __attribute__((aligned(8))) = { 2u, ub_site, ub_key, ub_n };
    nd_ub_on = 0;
    nd_rec_napi(nd_ublk_run, r, 2u);
}

/* ---- core 0: the census ---- */
#define UB_NS 8u
static struct { uint32_t key, sh, wh; unsigned set; } ub_last[UB_NS];
static unsigned ub_c[8];                 /* n key st w bad spl words draws (sums over the window) */
static unsigned long long ub_tk;         /* block ticks summed over the window */
static int ub_b_on;
static unsigned ub_b_site, ub_b_coff, ub_b_split, ub_b_draws;
static uint32_t ub_b_sh;
static unsigned long long ub_b_t0;

void nd_ublk_run(const uint32_t* rec) {
    if (rec[0] == 1u) { /* begin */
        ub_b_on = pm_pf_on;
        if (!ub_b_on) return;
        u32 *cb; u32 csz, coff; GPUCMD_GetBuffer(&cb, &csz, &coff);
        ub_b_t0 = svcGetSystemTick();
        ub_b_site = rec[1]; ub_b_coff = coff; ub_b_split = pm_gpu_cmd_split_n();
        ub_b_draws = pm_gpu_draws_frame(); ub_b_sh = pm_gpu_state_hash();
        return;
    }
    if (rec[0] != 2u || !ub_b_on) return; /* end */
    ub_b_on = 0;
    const unsigned long long t1 = svcGetSystemTick();
    u32 *cb; u32 csz, coff; GPUCMD_GetBuffer(&cb, &csz, &coff);
    if (pm_gpu_cmd_split_n() != ub_b_split || coff < ub_b_coff) { ub_c[5]++; return; } /* a split inside the block: count it, no compare */
    const uint32_t wh = pm_gpu_word_hash(ub_b_coff, coff);
    const uint32_t key = rec[2];
    const unsigned s = ub_b_site & (UB_NS - 1u);
    ub_c[0]++;
    if (ub_last[s].set) {
        const int k = key == ub_last[s].key;
        const int st = k && ub_b_sh == ub_last[s].sh;
        const int w = st && wh == ub_last[s].wh;
        ub_c[1] += (unsigned)k; ub_c[2] += (unsigned)st; ub_c[3] += (unsigned)w; ub_c[4] += (unsigned)(st && !w);
    }
    ub_last[s].key = key; ub_last[s].sh = ub_b_sh; ub_last[s].wh = wh; ub_last[s].set = 1u;
    ub_c[6] += coff - ub_b_coff;
    ub_c[7] += pm_gpu_draws_frame() - ub_b_draws;
    ub_tk += t1 - ub_b_t0;
}

/* the prof line: copies the window counts and zeroes them (core 0) */
void nd_ublk_take(unsigned o[8], unsigned long long* tk) {
    for (unsigned i = 0; i < 8u; i++) { o[i] = ub_c[i]; ub_c[i] = 0u; }
    *tk = ub_tk; ub_tk = 0u;
}
