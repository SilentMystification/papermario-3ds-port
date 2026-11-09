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

float (*gm_tab)[4][4]; /* matrices of the frame, 256 */

float (*gm_nrm)[3][4]; /* GV5: the rows 0 to 2 of the model view matrix of each gm_tab entry (the normal matrix), made when the entry is made */

unsigned gm_n;

void upload_mvp(void) {
    nd_mtx_combine(combined); /* S1-4: modelview x projection (STEREO: one matrix for both eyes, the shader shifts) */
    pm_gpu_note_mvp(combined, vp_sx, vp_sy, vp_tx, vp_ty); /* GV1c: the matrix is in the palette of the triangles */
}

/* H3: open-addressing index of gm_tab by a hash of the combined matrix. Slot holds entry + 1, 0 is empty. 512 slots for at most 256 entries. */
unsigned short gm_idx[512];

static unsigned gm_hash(const float (*m)[4]) {
    const unsigned* w = (const unsigned*)m;
    unsigned h = 2166136261u;
    for (int i = 0; i < 16; i++) h = (h ^ w[i]) * 16777619u;
    return (h ^ (h >> 16)) & 511u;
}

unsigned gv_matrix_id(int need_nm) {
    if (!gm_tab) {
        gm_tab = (float(*)[4][4])malloc(256 * sizeof(float[4][4]));
        gm_nrm = (float(*)[3][4])malloc(256 * sizeof(float[3][4]));
        if (!gm_tab || !gm_nrm) { free(gm_tab); free(gm_nrm); gm_tab = NULL; gm_nrm = NULL; return GV_NONE; }
        pm_gpu_set_ntab((const float(*)[3][4])gm_nrm);
    }
    if (gm_last != GV_NONE && gm_last < gm_n && memcmp(gm_tab[gm_last], combined, sizeof(float[4][4])) == 0 && !need_nm) return gm_last;
    float nm[3][4];
    for (int i = 0; i < 3; i++) { nm[i][0] = nd_mv[nd_mv_sp][i][0]; nm[i][1] = nd_mv[nd_mv_sp][i][1]; nm[i][2] = nd_mv[nd_mv_sp][i][2]; nm[i][3] = 0.f; }
    if (gm_last != GV_NONE && gm_last < gm_n && memcmp(gm_tab[gm_last], combined, sizeof(float[4][4])) == 0 && memcmp(gm_nrm[gm_last], nm, sizeof nm) == 0) return gm_last;
    unsigned p, best = GV_NONE, gmi;
    /* H3: the newest entry with the same combined matrix (and normal when needed), found by probing its hash chain */
    for (p = gm_hash(combined); gm_idx[p]; p = (p + 1) & 511u) {
        unsigned e = gm_idx[p] - 1u;
        gm_steps++;
        if ((best == GV_NONE || e > best) && memcmp(gm_tab[e], combined, sizeof(float[4][4])) == 0 && (!need_nm || memcmp(gm_nrm[e], nm, sizeof nm) == 0)) best = e;
    }
    if (best != GV_NONE) gmi = best;
    else if (gm_n < 256) { memcpy(gm_tab[gm_n], combined, sizeof(float[4][4])); memcpy(gm_nrm[gm_n], nm, sizeof nm); gm_idx[p] = (unsigned short)(gm_n + 1); gmi = gm_n++; }
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
extern int pm_interp_repl;
int pm_gbi_ndref_frame(void);
extern const float (*pm_interp_find(const void* m))[4];
static __attribute__((cold, noinline)) void mtx_dbg(const Mtx* src, u32 hdr) {
    static unsigned n;
    const u32* s = (const u32*)src->m;
    const float (*lm)[4] = pm_interp_repl ? pm_interp_find(src) : 0;
    pm_log("gmtx f%u n %u src %p w %08x %08x %08x %08x slot %u tag %u repl %d lm %d", pm_frame_count(), n++, (const void*)src,
           s[0], s[1], s[2], s[3], hdr & 0xffu, (unsigned)(hdr >> 16), pm_interp_repl, lm != 0);
}
void load_mtx(u32 w0, u32 w1, u32 hdr) {
    unsigned flags = (w0 & 0xff) ^ G_MTX_PUSH;
    Mtx* src = (Mtx*)ptr_of(w1);
    if (!src) return;
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
    nd_mtx_load(in, flags); /* S1-4: push, load or premultiply */
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

void rec_lights(const Lightsn* L, unsigned k) {
    Gfx tg[12];
    Gfx* p = tg;
    switch (k) {
    case 0: gSPSetLights0(p++, L[0]); break;
    case 1: gSPSetLights1(p++, L[0]); break;
    case 2: gSPSetLights2(p++, L[0]); break;
    case 3: gSPSetLights3(p++, L[0]); break;
    case 4: gSPSetLights4(p++, L[0]); break;
    case 5: gSPSetLights5(p++, L[0]); break;
    case 6: gSPSetLights6(p++, L[0]); break;
    case 7: gSPSetLights7(p++, L[0]); break;
    default: return;
    }
    for (const Gfx* h = tg; h < p; h++) {
        u32 w0 = h->words.w0, w1 = h->words.w1;
        unsigned op = w0 >> 24;
        if (g2_same(op, w0, w1)) { g2_note(op, w0); continue; }
        if (op == G_MOVEWORD) gbi_moveword(w0, w1);
        else if (op == G_MOVEMEM) gbi_movemem(w0, w1);
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
