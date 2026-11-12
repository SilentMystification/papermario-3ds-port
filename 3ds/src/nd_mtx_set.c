/* nd_mtx_set.c: the N64 glue of the native matrix group (plan_s10_path GM-1, MV row (b)): G_MTX and G_MOVEMEM handlers, viewport, popmtx,
 * record lights, the model matrix cache and the gv matrix table. Target only: it calls walker, GPU and interp symbols. The host tests link nd_mtx.c, which holds the pure math. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"
#include "ultra64.h"
#include "gbi_custom.h"
#include "pm_gpu.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
#include "nd_int.h"
#include "nd_op.h"

/* GLOB: definitions moved here from the former pm_gbi.c (layout only). */
NdLight lt[9];
unsigned lt_n;
s8 lk[2][3]; /* T01: look-at X and Y vectors (G_MOVEMEM slots -2 and -1) */

/* ---- M: moved from the former pm_gbi.c (plan_s10_path group M, GM-1): matrix load, viewport, movemem, popmtx, record lights ---- */
float combined[4][4];

/* Viewport scale/translate after the 2-bit fraction. Full 320x240 until a display list sets one. */
float vp_sx = 160.f, vp_sy = 120.f, vp_sz = 127.75f;
float vp_tx = 160.f, vp_ty = 120.f, vp_tz = 127.75f;

unsigned gm_last = 0xffffu; /* GV1: the gm_tab entry of the last G_VTX */

unsigned gm_steps; /* W1: entries compared by gv_matrix_id, summed over the report window */

float (*gm_tab)[4][4]; /* matrices of the frame, PM_GM_N */

float (*gm_nrm)[3][4]; /* GV5: the rows 0 to 2 of the model view matrix of each gm_tab entry (the normal matrix), made when the entry is made */

unsigned gm_n;

/* INTERPB1: the key frame twins of the t = 0.5 pass (pm_interp_repl), same indices as gm_tab and gm_nrm. Allocated at the first L walk. */
extern int pm_interp_repl;
extern int pm_tw_on; /* pm_nusys.c: 1 for the whole walk of an L frame (its background and main tasks): the twins are made, K = L where nothing is replaced */
float combined_k[4][4];
float (*gm_tab_k)[4][4];
float (*gm_nrm_k)[3][4];
int nd_tw_chk; /* switch twinchk, read at each task start */
extern int pm_icmp_kind; /* pm_nusys.c: 0 plain, 1 L, 2 K frame (twinchk) */

void upload_mvp(void) {
    nd_mtx_combine(combined);
    if (pm_tw_on) nd_mtx_combine_k(combined_k); /* INTERPB1 */ /* S1-4: modelview x projection (STEREO: one matrix for both eyes, the shader shifts) */
    pm_gpu_note_mvp(combined, vp_sx, vp_sy, vp_tx, vp_ty); /* GV1c: the matrix is in the palette of the triangles */
}

/* H3: open-addressing index of gm_tab by a hash of the combined matrix. Slot holds entry + 1, 0 is empty. 2 * PM_GM_N slots for at most PM_GM_N entries. */
unsigned short gm_idx[2u * PM_GM_N];

static unsigned gm_hash(const float (*m)[4]) {
    const unsigned* w = (const unsigned*)m;
    unsigned h = 2166136261u;
    for (int i = 0; i < 16; i++) h = (h ^ w[i]) * 16777619u;
    return (h ^ (h >> 16)) & (2u * PM_GM_N - 1u);
}

unsigned gv_matrix_id(int need_nm) {
    if (!gm_tab) {
        gm_tab = (float(*)[4][4])malloc(PM_GM_N * sizeof(float[4][4]));
        gm_nrm = (float(*)[3][4])malloc(PM_GM_N * sizeof(float[3][4]));
        if (!gm_tab || !gm_nrm) { free(gm_tab); free(gm_nrm); gm_tab = NULL; gm_nrm = NULL; return GV_NONE; }
        pm_gpu_set_ntab((const float(*)[3][4])gm_nrm);
    }
    const int tw = pm_tw_on; /* INTERPB1: in an L frame an entry is the pair (L, K) */
    if (tw && !gm_tab_k) {
        gm_tab_k = (float(*)[4][4])malloc(PM_GM_N * sizeof(float[4][4]));
        gm_nrm_k = (float(*)[3][4])malloc(PM_GM_N * sizeof(float[3][4]));
        if (!gm_tab_k || !gm_nrm_k) { free(gm_tab_k); free(gm_nrm_k); gm_tab_k = NULL; gm_nrm_k = NULL; return GV_NONE; }
    }
    if (gm_last != GV_NONE && gm_last < gm_n && memcmp(gm_tab[gm_last], combined, sizeof(float[4][4])) == 0 && !need_nm && (!tw || memcmp(gm_tab_k[gm_last], combined_k, sizeof(float[4][4])) == 0)) return gm_last;
    float nm[3][4], nm_k[3][4];
    for (int i = 0; i < 3; i++) { nm[i][0] = nd_mv[nd_mv_sp][i][0]; nm[i][1] = nd_mv[nd_mv_sp][i][1]; nm[i][2] = nd_mv[nd_mv_sp][i][2]; nm[i][3] = 0.f; }
    if (tw) for (int i = 0; i < 3; i++) { nm_k[i][0] = nd_mv_k[nd_mv_sp][i][0]; nm_k[i][1] = nd_mv_k[nd_mv_sp][i][1]; nm_k[i][2] = nd_mv_k[nd_mv_sp][i][2]; nm_k[i][3] = 0.f; }
    if (gm_last != GV_NONE && gm_last < gm_n && memcmp(gm_tab[gm_last], combined, sizeof(float[4][4])) == 0 && memcmp(gm_nrm[gm_last], nm, sizeof nm) == 0 &&
        (!tw || (memcmp(gm_tab_k[gm_last], combined_k, sizeof(float[4][4])) == 0 && memcmp(gm_nrm_k[gm_last], nm_k, sizeof nm_k) == 0))) return gm_last;
    unsigned p, best = GV_NONE, gmi;
    /* H3: the newest entry with the same combined matrix (and normal when needed), found by probing its hash chain */
    for (p = gm_hash(combined); gm_idx[p]; p = (p + 1) & (2u * PM_GM_N - 1u)) {
        unsigned e = gm_idx[p] - 1u;
        gm_steps++;
        if ((best == GV_NONE || e > best) && memcmp(gm_tab[e], combined, sizeof(float[4][4])) == 0 && (!need_nm || memcmp(gm_nrm[e], nm, sizeof nm) == 0) &&
            (!tw || (memcmp(gm_tab_k[e], combined_k, sizeof(float[4][4])) == 0 && (!need_nm || memcmp(gm_nrm_k[e], nm_k, sizeof nm_k) == 0)))) best = e;
    }
    if (best != GV_NONE) gmi = best;
    else if (gm_n < PM_GM_N) {
        memcpy(gm_tab[gm_n], combined, sizeof(float[4][4])); memcpy(gm_nrm[gm_n], nm, sizeof nm);
        if (tw) { memcpy(gm_tab_k[gm_n], combined_k, sizeof(float[4][4])); memcpy(gm_nrm_k[gm_n], nm_k, sizeof nm_k); }
        gm_idx[p] = (unsigned short)(gm_n + 1); gmi = gm_n++;
    }
    else gmi = GV_NONE;
    gm_last = gmi;
    return gmi;
}

/* SR2: unpacked model matrices by slot (header w1 bits 0 to 7). A tag is 0x10000 | pmMtxVer when valid, 0 when not. */
float mtx_cache[256][16];
u32 mtx_tag[256];

/* hdr: the header w1 of a producer record (model.c R1), 0 for a plain G_MTX. */
_Static_assert(ND_MTX_PUSH == G_MTX_PUSH && ND_MTX_LOAD == G_MTX_LOAD && ND_MTX_PROJ == G_MTX_PROJECTION, "nd.h matrix flags equal the gbi values");
/* M4-2 diag, switch ndref=<frame>: one line per G_MTX load of that frame (source pointer, first words, interp hit, slot) */
int pm_gbi_ndref_frame(void);
extern const float (*pm_interp_find(const void* m))[4];
static __attribute__((cold, noinline)) void mtx_dbg(const Mtx* src, u32 hdr) {
    static unsigned n;
    const u32* s = (const u32*)src->m;
    const float (*lm)[4] = pm_interp_repl ? pm_interp_find(src) : 0;
    pm_log("gmtx f%u n %u src %p w %08x %08x %08x %08x slot %u tag %u repl %d lm %d", pm_frame_count(), n++, (const void*)src,
           s[0], s[1], s[2], s[3], hdr & 0xffu, (unsigned)(hdr >> 16), pm_interp_repl, lm != 0);
}
int nd_mtx_static; /* SEAM2: the last model matrix load was a static matrix (R1 bit 8) */
/* LOADMTX: a matrix source outside host memory (a stale or wrong record word). The load is skipped: the old matrix stays. One warning per run,
 * as the walker's G_MTX check (nd_walk.c gbi_cold_bad_mtx). pm_log_evt passes the logging gate in every mode (console log, prof mode). */
static __attribute__((cold, noinline)) void mtx_bad(u32 w0, u32 w1, u32 hdr, const void* src, const void* caller) {
    static int once;
    if (once) return;
    once = 1;
    pm_log_evt("load_mtx: source %08x outside host memory, w0 %08x w1 %08x hdr %08x, caller %p, frame %u, skipped",
               (unsigned)(uintptr_t)src, (unsigned)w0, (unsigned)w1, (unsigned)hdr, caller, pm_frame_count());
}
void load_mtx(u32 w0, u32 w1, u32 hdr) {
    nd_mtx_static = (hdr & 0x100u) != 0;
    unsigned flags = (w0 & 0xff) ^ G_MTX_PUSH;
    Mtx* src = (Mtx*)ptr_of(w1);
    if (!src) return;
    if (__builtin_expect(!is_host((u32)(uintptr_t)src), 0)) { mtx_bad(w0, w1, hdr, src, __builtin_return_address(0)); return; } /* LOADMTX */
    float in[4][4];
    if (pm_frame_count() == 200 && pm_gbi_ndref_frame() == 200) mtx_dbg(src, hdr); /* M4-2 diag */
    int lm = nd_mtx_replace(src, in); /* F06-2, S1-5: the interpolated matrix of the t = 0.5 pass */
    unsigned slot = hdr & 0xffu;
    int cached = slot != 0 && (hdr & 0x100u) != 0 && !lm; /* a static model matrix: the header says src is that model's savedMtx */
    u32 tag = 0x10000u | (hdr >> 16);
    if (lm) { /* the interpolated matrix is in `in` already */ }
    else if (cached && mtx_tag[slot] == tag) memcpy(in, mtx_cache[slot], sizeof(in));
    else {
        nd_mtx_unpack((const uint32_t*)src->m, in);
        if (cached) {
            memcpy(mtx_cache[slot], in, sizeof(in));
            mtx_tag[slot] = tag;
        }
    }
    float ink[4][4];
    if (pm_tw_on) { /* INTERPB1: the key frame matrix K of this G_MTX: the cache rule of the main pass */
        if (!lm) memcpy(ink, in, sizeof ink);
        else {
            const int ck = slot != 0 && (hdr & 0x100u) != 0;
            if (ck && mtx_tag[slot] == tag) memcpy(ink, mtx_cache[slot], sizeof ink);
            else {
                nd_mtx_unpack((const uint32_t*)src->m, ink);
                if (ck) { memcpy(mtx_cache[slot], ink, sizeof ink); mtx_tag[slot] = tag; }
            }
        }
    }
    nd_mtx_load_tw(in, pm_tw_on ? ink : 0, flags); /* S1-4: push, load or premultiply */
    upload_mvp();
}

static __attribute__((noinline)) void gbi_viewport(u32 w1) {
    Vp* vp = (Vp*)ptr_of(w1);
    if (vp) {
        vp_sx = (float)vp->vp.vscale[0] / 4.f;
        vp_sy = (float)vp->vp.vscale[1] / 4.f;
        vp_sz = (float)vp->vp.vscale[2] / 4.f;
        vp_tx = (float)vp->vp.vtrans[0] / 4.f;
        vp_ty = (float)vp->vp.vtrans[1] / 4.f;
        vp_tz = (float)vp->vp.vtrans[2] / 4.f;
        upload_mvp();
    }
}

void gbi_movemem(u32 w0, u32 w1) {
    if ((w0 & 0xff) == G_MV_LIGHT) lit_freeze(); /* GV5 */
    if ((w0 & 0xff) == G_MV_LIGHT) { /* T01: look-at vectors */
        int ls = (int)(((w0 >> 8) & 0xff) * 8 / 24) - 2;
        const u8* l = (const u8*)ptr_of(w1);
        if (l && (ls == -2 || ls == -1)) { lk[ls + 2][0] = (s8)l[8]; lk[ls + 2][1] = (s8)l[9]; lk[ls + 2][2] = (s8)l[10]; }
    }
    if ((w0 & 0xff) == G_MV_LIGHT) { /* L01: offset 0 and 24 are the look-at vectors, light n is at n * 24 + 24 */
        int slot = (int)(((w0 >> 8) & 0xff) * 8 / 24) - 2;
        const u8* l = (const u8*)ptr_of(w1);
        if (l && slot >= 0 && slot < 9) { lt[slot].col[0] = l[0]; lt[slot].col[1] = l[1]; lt[slot].col[2] = l[2]; lt[slot].dir[0] = (s8)l[8]; lt[slot].dir[1] = (s8)l[9]; lt[slot].dir[2] = (s8)l[10]; }
    }
    if ((w0 & 0xff) == G_MV_VIEWPORT) gbi_viewport(w1);
}

void gbi_popmtx(u32 w1) {
    nd_mtx_pop(w1 / 64); /* S1-4: count 0 pops one */
    upload_mvp();
}

/* HC4d: the lights of a model record (gSPSetLights0 to 7, F3DEX2) by their typed fields: the light count (G_MW_NUMLIGHT, NUML(n), n at
 * least 1), then G_MOVEMEM light k (k = 1 to n: l[k - 1], n + 1: the ambient) at offset k * 24 + 24, each with the walker's skip test. */
void rec_lights(const Lightsn* L, unsigned k) {
    if (k > 7u) return;
    const unsigned n = k ? k : 1u; /* NUMLIGHTS_0 is 1 */
    const uint32_t t[2] = { NDO_W0(NDO_MW, NDO_MW_NUMLIGHT << 16), NUML(n) };
    if (!nd_op_same(t)) nd_op_apply(t);
    for (unsigned i = 1; i <= n + 1u; i++) {
        const u32 a = (((u32)sizeof(Light) - 1u) / 8u) << 19 | ((i * 24u + 24u) / 8u) << 8 | G_MV_LIGHT;
        const u32 b = (u32)(uintptr_t)(i <= n ? (const void*)&L->l[i - 1u] : (const void*)&L->a);
        if (!lt_mem_same(a, b)) gbi_movemem(a, b);
    }
}

void mtx_elem(Mtx* dst, int r, int c, float v) {
    s32 fixed = (s32)(v * 65536.f);
    u32* m = (u32*)dst->m;
    int idx = r * 2 + (c >> 1);
    u32 sh = (c & 1) ? 0u : 16u;
    u32 mask = 0xffffu << sh;
    m[idx] = (m[idx] & ~mask) | ((((u32)(fixed >> 16)) & 0xffffu) << sh);
    m[idx + 8] = (m[idx + 8] & ~mask) | (((u32)fixed & 0xffffu) << sh);
}

/* ---- MV-10: light count, cut from the former pm_gbi.c (no change; target only, calls lit_freeze) ---- */
extern unsigned lt_n;
void nd_light_count(u32 w1) { lit_freeze(); lt_n = w1 / 24; if (lt_n > 7) lt_n = 7; } /* ST-1: G_MW_NUMLIGHT; group M owns lt_n */

/* pm_gpu native draw: the combined matrix of the walker's current draw (modelview x projection). */
const float (*pm_gbi_combined_ptr(void))[4] { return combined; }

/* ---- INTERPB1 debug (switch twinchk): the twins of the L walk (frame kind 1) against the values the key walk (kind 2) makes in the same
 * order: gm_tab_k[i] against gm_tab[i] (and the nrm rows, and pcomp at the key walk's pcomp_calc), the palette 0 and the pm5 normal rows of each
 * draw site. The frame end (pm_gbi_end_frame) keeps the L entry count, or compares the K frame with it. One line per 10 compared steps
 * (the scenarios end soon after the first icmp lines). Cold: runs only under the switch. ---- */
unsigned nd_tw_m, nd_tw_p, nd_tw_q; /* entry and pcomp mismatches, pal0 mismatches, nrm mismatches */
#define TW_N 4096u
static u32 tw_hp[TW_N], tw_hq[TW_N];
static unsigned tw_e, tw_win, tw_ln, tw_have_l, tw_np_l, tw_nq_l, tw_np_k, tw_nq_k;
static u32 tw_hash(const void* v, unsigned nw) {
    const u32* w = (const u32*)v;
    u32 h = 2166136261u;
    for (unsigned i = 0; i < nw; i++) h = (h ^ w[i]) * 16777619u;
    return h;
}
void nd_twinchk_pal(const float* o) { /* palette 0 of a draw (the K twin in the L walk, the main value in the key walk) */
    if (pm_icmp_kind == 1) { if (tw_np_l < TW_N) tw_hp[tw_np_l] = tw_hash(o, 16); tw_np_l++; }
    else if (pm_icmp_kind == 2) { if (tw_np_k >= tw_np_l || tw_np_k >= TW_N || tw_hp[tw_np_k] != tw_hash(o, 16)) nd_tw_p++; tw_np_k++; }
}
void nd_twinchk_nrm(const float n[3][4]) { /* the pm5 normal rows of a range */
    if (pm_icmp_kind == 1) { if (tw_nq_l < TW_N) tw_hq[tw_nq_l] = tw_hash(n, 12); tw_nq_l++; }
    else if (pm_icmp_kind == 2) { if (tw_nq_k >= tw_nq_l || tw_nq_k >= TW_N || tw_hq[tw_nq_k] != tw_hash(n, 12)) nd_tw_q++; tw_nq_k++; }
}
static __attribute__((cold, noinline)) void tw_frame_end(void) {
    if (pm_icmp_kind == 1) { tw_ln = gm_n; tw_have_l = 1; }
    else if (pm_icmp_kind == 2 && tw_have_l) {
        unsigned n = gm_n < tw_ln ? gm_n : tw_ln;
        for (unsigned i = 0; i < n; i++) {
            if (memcmp(gm_tab[i], gm_tab_k[i], sizeof(float[4][4])) != 0) nd_tw_m++;
            if (memcmp(gm_nrm[i], gm_nrm_k[i], sizeof(float[3][4])) != 0) nd_tw_q++;
        }
        nd_tw_m += gm_n > tw_ln ? gm_n - tw_ln : tw_ln - gm_n;
        nd_tw_p += tw_np_k > tw_np_l ? tw_np_k - tw_np_l : tw_np_l - tw_np_k;
        nd_tw_q += tw_nq_k > tw_nq_l ? tw_nq_k - tw_nq_l : tw_nq_l - tw_nq_k;
        tw_e += n;
        tw_have_l = 0;
        if (++tw_win >= 10u) {
            pm_log("twinchk f%u entries %u mismatch %u pal0 %u nrm %u", pm_frame_count(), tw_e, nd_tw_m, nd_tw_p, nd_tw_q);
            tw_e = tw_win = 0; nd_tw_m = nd_tw_p = nd_tw_q = 0;
        }
    }
}
void pm_gpu_twin_begin(void); /* pm_gpu.c */
/* The start of a walked frame (pm_nusys.c): kind 1 an L frame, 2 a key frame, 0 plain. */
void nd_twin_frame_begin(int kind) {
    nd_tw_chk = PMDL("twinchk");
    if (kind == 1) { nd_mtx_twin_sync(); pm_gpu_twin_begin(); tw_np_l = tw_nq_l = 0; } /* the twin stack starts from the main one */
    else if (kind == 2) tw_np_k = tw_nq_k = 0;
}
/* The end of a walked frame (nd_cen.c pm_gbi_end_frame, before the matrix table is reset). */
void nd_twin_frame_end(void) { if (__builtin_expect(nd_tw_chk, 0)) tw_frame_end(); }
