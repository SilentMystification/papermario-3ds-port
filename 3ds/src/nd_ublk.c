/* nd_ublk.c: GD9A census of the status bar block (plan_gamedraw 1.6), GD9A2 census 2. Core 1 brackets draw_status_ui with a begin and an end
 * record (nd_ublk_begin, nd_ublk_end). The end record carries a key: FNV-1a of the recorder inputs inside the bracket (unit words; matrix and
 * vertex contents, not their pointers). The begin record carries the FNV of nd_ds before the bracket. Core 0 (nd_ublk_run) counts per site whether
 * the key, the ds hash, the state hash and the emitted words repeat the last counted block of that site. ublkf=<f0>:<f1> limits the count and the
 * log to the frames f0 to f1. Core 1 logs the first differing unit (kdiff, kn) and the first differing nd_ds byte (dsdiff) inside the span.
 * Prof only (pm_pf_on): nothing is hashed, copied or timed otherwise. No draw change. */
#include <stdint.h>
#include <stdlib.h>
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
extern unsigned pm_walk_frame;                           /* nd_wq.c: the frame of the entry the walker runs */
void pm_log_prof_line(const char* fmt, ...); /* pm_log.c: prof=4 level; plain pm_log is dropped in prof mode (GD9A2) */
int is_host(unsigned int p);                             /* nd_walk.c: a pure range test */
unsigned nd_ds_size(void); const void* nd_ds_addr(void); /* nd_rec.c, GD9A2: nd_ds without nd_ds.h */

#define UB_FNV0 2166136261u
#define UB_FNVP 16777619u

/* ublkf=<f0>:<f1> (GD9A2): the frame span. Returns 1 when the key is set; f1 = f0 without the colon. */
static int ub_span_get(unsigned* f0, unsigned* f1) {
    char v[24];
    char* e;
    if (!pm_debug_value("ublkf", v, sizeof v)) return 0;
    *f0 = (unsigned)strtoul(v, &e, 10);
    *f1 = (*e == ':') ? (unsigned)strtoul(e + 1, NULL, 10) : *f0;
    return 1;
}

/* ---- core 1: the key of the bracket and its entries ---- */
#define UB_MAXU 512u
typedef struct { uint32_t h, a, b; } UbE;   /* entry: FNV of the contribution, its words */
static uint32_t ub_key, ub_eh;              /* bracket key; hash of the entry being built */
static unsigned ub_n, ub_site;
static UbE *ub_cur, *ub_prev;               /* entries of this bracket, of the last counted bracket */
static unsigned ub_pn;
static uint32_t ub_pkey, ub_dh_cur, ub_dh_prev;
static uint8_t *ub_ds_cur, *ub_ds_prev;     /* nd_ds at the begin record of this bracket and of the last counted one */
static unsigned ub_dsz;                    /* sizeof nd_ds */
static int ub_alloc, ub_pset;
static int ub_c1_rd, ub_c1_sp;
static unsigned ub_c1_f0, ub_c1_f1;
static unsigned ub_nlog[3];                 /* kdiff, kn, dsdiff: lines logged (8 each per run) */

static void ub_mix(unsigned char b) { ub_key ^= b; ub_key *= UB_FNVP; ub_eh ^= b; ub_eh *= UB_FNVP; }
static void ub_w(uint32_t x) { for (int i = 0; i < 4; i++) ub_mix((unsigned char)((x >> (8 * i)) & 0xffu)); }
static void ub_bytes(const void* p, unsigned n) { const unsigned char* c = (const unsigned char*)p; for (unsigned i = 0; i < n; i++) ub_mix(c[i]); }
static void ub_ent(uint32_t a, uint32_t b) {
    if (ub_cur && ub_n < UB_MAXU) { ub_cur[ub_n].h = ub_eh; ub_cur[ub_n].a = a; ub_cur[ub_n].b = b; }
    ub_n++;
}
static uint32_t ub_fnv(const void* p, unsigned n) {
    const unsigned char* c = (const unsigned char*)p; uint32_t h = UB_FNV0;
    for (unsigned i = 0; i < n; i++) { h ^= c[i]; h *= UB_FNVP; }
    return h;
}

/* one stream unit: the data behind a matrix or vertex pointer, not the pointer (it alternates per context) */
void nd_ublk_unit(uint32_t w0, uint32_t w1) {
    const unsigned id = w0 >> 24;
    ub_eh = UB_FNV0;
    ub_w(w0);
    if (id == ND_FS_MTX && is_host(w1)) ub_bytes((const void*)(uintptr_t)w1, 64u);
    else if (id == ND_FS_VTX && is_host(w1)) ub_bytes((const void*)(uintptr_t)w1, 16u * ((w0 >> 12) & 0xffu));
    else ub_w(w1);
    ub_ent(w0, w1);
}

/* a native record: its runner, then its units. One unit is an nd_xf record (a matrix pair, nd_rec_xf). */
void nd_ublk_napi(NdNapiRun run, const void* rec, unsigned units) {
    ub_eh = UB_FNV0;
    ub_w((uint32_t)(uintptr_t)run);
    ub_ent((uint32_t)(uintptr_t)run, 0u);
    if (units == 1u) { const uint32_t* r = (const uint32_t*)rec; nd_ublk_unit(r[0], r[1]); }
    else { ub_eh = UB_FNV0; ub_bytes(rec, 8u * units); ub_ent(units, (uint32_t)(uintptr_t)rec); }
}

/* the ds snap of the begin record (prof only): a copy of nd_ds and its hash; 0 when the copy is not allocated */
static uint32_t ub_ds_snap(void) {
    if (!ub_alloc) {
        ub_alloc = 1;
        ub_cur = (UbE*)malloc(UB_MAXU * sizeof(UbE)); ub_prev = (UbE*)malloc(UB_MAXU * sizeof(UbE));
        ub_dsz = nd_ds_size(); ub_ds_cur = (uint8_t*)malloc(ub_dsz); ub_ds_prev = (uint8_t*)malloc(ub_dsz);
    }
    if (!ub_ds_cur) return 0u;
    memcpy(ub_ds_cur, nd_ds_addr(), ub_dsz);
    return ub_fnv(ub_ds_cur, ub_dsz);
}

void nd_ublk_begin(unsigned site) {
    uint32_t r[4] __attribute__((aligned(8))) = { 1u, site, 0u, 0u };
    ub_dh_cur = pm_pf_on ? ub_ds_snap() : 0u;
    r[2] = ub_dh_cur;
    nd_rec_napi(nd_ublk_run, r, 2u);
    ub_site = site; ub_key = UB_FNV0; ub_eh = UB_FNV0; ub_n = 0u; nd_ub_on = pm_pf_on;
}

/* core 1 at the end record (prof only): the first differing unit or nd_ds byte inside the span, then the bracket becomes the last one */
static void ub_cmp(void) {
    if (!ub_c1_rd) { ub_c1_rd = 1; ub_c1_sp = ub_span_get(&ub_c1_f0, &ub_c1_f1); }
    const unsigned f = pm_frame_count();
    const unsigned nmax = ub_n < UB_MAXU ? ub_n : UB_MAXU;
    if (ub_pset && ub_c1_sp && f >= ub_c1_f0 && f <= ub_c1_f1) {
        if (ub_key != ub_pkey) {
            if (ub_n != ub_pn) {
                if (ub_nlog[1] < 8u) { ub_nlog[1]++; pm_log_prof_line("ublk f%u kn %u was %u", f, ub_n, ub_pn); }
            } else {
                for (unsigned i = 0; i < nmax; i++) {
                    if (ub_cur[i].h == ub_prev[i].h) continue;
                    if (ub_nlog[0] < 8u) { ub_nlog[0]++; pm_log_prof_line("ublk f%u kdiff u%u w0 %08x w1 %08x was %08x %08x", f, i, ub_cur[i].a, ub_cur[i].b, ub_prev[i].a, ub_prev[i].b); }
                    break;
                }
            }
        } else if (ub_dh_cur != ub_dh_prev && ub_ds_cur && ub_ds_prev) {
            const uint8_t* a = ub_ds_cur; const uint8_t* b = ub_ds_prev;
            unsigned off = 0u;
            while (off < ub_dsz && a[off] == b[off]) off++;
            if (ub_nlog[2] < 8u && off < ub_dsz) { ub_nlog[2]++; pm_log_prof_line("ublk f%u dsdiff off %u", f, off); }
        }
    }
    { UbE* t = ub_prev; ub_prev = ub_cur; ub_cur = t; }
    { uint8_t* t = ub_ds_prev; ub_ds_prev = ub_ds_cur; ub_ds_cur = t; }
    ub_pn = ub_n; ub_pkey = ub_key; ub_dh_prev = ub_dh_cur; ub_pset = 1;
}

void nd_ublk_end(void) {
    const int on = nd_ub_on;
    uint32_t r[4] __attribute__((aligned(8))) = { 2u, ub_site, ub_key, ub_n };
    nd_ub_on = 0;
    if (on && ub_cur && ub_prev && ub_ds_cur && ub_ds_prev) ub_cmp();
    nd_rec_napi(nd_ublk_run, r, 2u);
}

/* ---- core 0: the census ---- */
#define UB_NS 8u
static struct { uint32_t key, sh, wh, ds; unsigned fr; unsigned set; } ub_last[UB_NS];
static unsigned ub_c[11];                /* n key st w bad spl words draws ds bad2 dup (sums over the window) */
static unsigned long long ub_tk;         /* block ticks summed over the window */
static int ub_b_on;
static unsigned ub_b_site, ub_b_coff, ub_b_split, ub_b_draws, ub_b_ds;
static uint32_t ub_b_sh;
static unsigned long long ub_b_t0;
static int c0_rd, c0_sp;
static unsigned c0_f0, c0_f1;

void nd_ublk_run(const uint32_t* rec) {
    if (rec[0] == 1u) { /* begin */
        ub_b_on = pm_pf_on;
        if (!ub_b_on) return;
        if (!c0_rd) { c0_rd = 1; c0_sp = ub_span_get(&c0_f0, &c0_f1); }
        u32 *cb; u32 csz, coff; GPUCMD_GetBuffer(&cb, &csz, &coff);
        ub_b_t0 = svcGetSystemTick();
        ub_b_site = rec[1]; ub_b_coff = coff; ub_b_split = pm_gpu_cmd_split_n(); ub_b_ds = rec[2];
        ub_b_draws = pm_gpu_draws_frame(); ub_b_sh = pm_gpu_state_hash();
        return;
    }
    if (rec[0] != 2u || !ub_b_on) return; /* end */
    ub_b_on = 0;
    const unsigned fr = pm_walk_frame;
    if (c0_sp && (fr < c0_f0 || fr > c0_f1)) return; /* outside the span: not counted */
    const unsigned long long t1 = svcGetSystemTick();
    u32 *cb; u32 csz, coff; GPUCMD_GetBuffer(&cb, &csz, &coff);
    if (pm_gpu_cmd_split_n() != ub_b_split || coff < ub_b_coff) { ub_c[5]++; return; } /* a split inside the block: count it, no compare */
    const uint32_t key = rec[2];
    const unsigned s = ub_b_site & (UB_NS - 1u);
    ub_c[0]++;
    ub_c[6] += coff - ub_b_coff;
    ub_c[7] += pm_gpu_draws_frame() - ub_b_draws;
    ub_tk += t1 - ub_b_t0;
    if (ub_last[s].set && ub_last[s].fr == fr) { ub_c[10]++; return; } /* a second walk of this site in the frame: counted, not compared */
    const uint32_t wh = pm_gpu_word_hash(ub_b_coff, coff);
    if (ub_last[s].set) {
        const int k = key == ub_last[s].key;
        const int st = k && ub_b_sh == ub_last[s].sh;
        const int w = st && wh == ub_last[s].wh;
        const int ds = k && ub_b_ds == ub_last[s].ds;
        ub_c[1] += (unsigned)k; ub_c[2] += (unsigned)st; ub_c[3] += (unsigned)w; ub_c[4] += (unsigned)(st && !w);
        ub_c[8] += (unsigned)ds; ub_c[9] += (unsigned)(ds && st && !w);
    }
    ub_last[s].key = key; ub_last[s].sh = ub_b_sh; ub_last[s].wh = wh; ub_last[s].ds = ub_b_ds; ub_last[s].fr = fr; ub_last[s].set = 1u;
}

/* the prof line: copies the window counts and zeroes them (core 0) */
void nd_ublk_take(unsigned o[11], unsigned long long* tk) {
    for (unsigned i = 0; i < 11u; i++) { o[i] = ub_c[i]; ub_c[i] = 0u; }
    *tk = ub_tk; ub_tk = 0u;
}
