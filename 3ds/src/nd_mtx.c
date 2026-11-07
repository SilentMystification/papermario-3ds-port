/* nd_mtx.c: native matrix stack, N64 matrix unpack, eye shift and the palette product (plan_native_slices S1-4).
 * Moved from pm_gbi.c (elem, unpack, mul, ident, load_mtx stack part, gbi_popmtx, upload_mvp) and pm_gpu.c (pal_mul).
 * Pure C on floats, no citro3d: the host tests build it. Build flags keep one rounding per operation (no x87 extended precision on the host). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"
#ifdef TARGET_3DS /* GM-1: the N64 glue of the matrix group (host tests build the pure part) */
#include "ultra64.h"
#include "gbi_custom.h"
#include "pm_gbi.h"
#include "pm_gpu.h"
#include "pm_tex.h"
#include "pm_port.h"
#include "variables.h"
#include "nd_int.h"
#endif

float nd_mv[ND_MV_DEPTH][4][4];
ND_HOTD int nd_mv_sp;
float nd_proj[4][4];
float nd_mtx_pool[2][ND_MTX_MAX][16] __attribute__((aligned(32)));

void nd_mtx_ident(float m[4][4]) {
    memset(m, 0, sizeof(float) * 16);
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;
}

/* the walker product: c = a x b, row vectors, accumulation order k = 0 to 3 */
void nd_mtx_mul(float c[4][4], const float a[4][4], const float b[4][4]) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) c[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
}

/* N64 fixed point Mtx: 16 words, integer halves in words 0 to 7, fraction halves in words 8 to 15 (s15.16 per element) */
static float elem(const uint32_t* m, int row, int col) {
    int idx = row * 2 + (col >> 1);
    uint32_t sh = (col & 1) ? 0 : 16;
    int16_t ip = (int16_t)((m[idx] >> sh) & 0xffff);
    int16_t fp = (int16_t)((m[idx + 8] >> sh) & 0xffff);
    int32_t fixed = (int32_t)((uint32_t)(int32_t)ip << 16 | (uint16_t)fp);
    return (float)fixed * (1.f / 65536.f); /* exact: a power of two scale */
}
void nd_mtx_unpack(const uint32_t* w, float d[4][4]) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) d[r][c] = elem(w, r, c);
}

extern int pm_interp_repl;                          /* pm_port.h: 1 during the t = 0.5 pass */
const float (*pm_interp_find(const void* m))[4];    /* pm_interp.c */
int nd_mtx_replace(const void* src, float in[4][4]) {
    const float (*lm)[4] = pm_interp_repl ? pm_interp_find(src) : 0;
    if (!lm) return 0;
    memcpy(in, lm, sizeof(float) * 16);
    return 1;
}

static float f127[256]; /* byte (a signed char) to value / 127: the same quotient as the division, without the division in the draw path */
void nd_lights(PmFx* fx, int texgen, const NdLight* lt, unsigned n, const int8_t lk[2][3]) {
    memset(fx, 0, sizeof *fx);
    fx->lflag[0] = 1.f;
    if (texgen) fx->lflag[1] = 1.f;
    for (int q = 0; q < 3; q++) fx->amb[q] = (float)lt[n].col[q];
    for (unsigned i = 0; i < n && i < 7; i++)
        for (int q = 0; q < 3; q++) { fx->col[i][q] = (float)lt[i].col[q]; fx->dir[i][q] = f127[(uint8_t)lt[i].dir[q]]; }
    for (int q = 0; q < 3; q++) { fx->lk0[q] = f127[(uint8_t)lk[0][q]]; fx->lk1[q] = f127[(uint8_t)lk[1][q]]; }
}

void nd_mtx_reset(void) {
    for (int i = 0; i < 256; i++) f127[i] = (float)(int8_t)i / 127.f;
    nd_mtx_ident(nd_proj);
    nd_mtx_ident(nd_mv[0]);
    nd_mv_sp = 0;
}

/* G_MTX: flags already hold G_MTX_PUSH set for a push (the F3DEX2 flag sense is xor-ed by the caller). A push beyond the depth is dropped like the RSP does. */
void nd_mtx_load(const float in[4][4], unsigned flags) {
    int is_proj = (flags & ND_MTX_PROJ) != 0;
    if (!is_proj && (flags & ND_MTX_PUSH) && nd_mv_sp + 1 < ND_MV_DEPTH) {
        memcpy(nd_mv[nd_mv_sp + 1], nd_mv[nd_mv_sp], sizeof(nd_mv[0]));
        nd_mv_sp++;
    }
    float (*dst)[4] = is_proj ? nd_proj : nd_mv[nd_mv_sp];
    if (flags & ND_MTX_LOAD) memcpy(dst, in, sizeof(float) * 16);
    else {
        /* Row vector v * incoming * current. The RSP and PaperBoat's interpreter both premultiply; the other order leaves a child under the wrong parent. */
        float cur[4][4];
        memcpy(cur, dst, sizeof cur);
        nd_mtx_mul(dst, in, cur);
    }
}

void nd_mtx_pop(unsigned count) {
    if (count == 0) count = 1;
    while (count-- && nd_mv_sp > 0) nd_mv_sp--;
}

/* combined = modelview x projection. STEREO: a perspective projection gets the eye shift x' = x + e * (w - conv). Row 3 is the translation row, the vertex has 1 in that place.
 * An orthographic matrix (UI) stays at the screen plane. e is the signed eye offset, conv the convergence factor. */
void nd_mtx_combine(float c[4][4], float e, float conv) {
    const float (*p)[4] = nd_proj;
    if (e != 0.f && (p[0][3] != 0.f || p[1][3] != 0.f || p[2][3] != 0.f)) {
        float s[4][4], cw = conv * (p[2][3] < 0.f ? -p[2][3] : p[2][3]);
        memcpy(s, p, sizeof s);
        for (int r = 0; r < 4; r++) s[r][0] += e * s[r][3];
        s[3][0] -= e * cw;
        nd_mtx_mul(c, nd_mv[nd_mv_sp], s);
    } else nd_mtx_mul(c, nd_mv[nd_mv_sp], p);
}

/* citro3d Mtx_Multiply (libcitro3d.a, one vmul and three vmla per element): out = a x b on matrices in the C3D_Mtx memory order, accumulation from the last column to the first */
void nd_mtx_cmul(float o[16], const float a[16], const float b[16]) {
    float t[16];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            float s = b[j] * a[4 * i + 3];
            s += a[4 * i + 2] * b[4 + j];
            s += a[4 * i + 1] * b[8 + j];
            s += a[4 * i + 0] * b[12 + j];
            t[4 * i + j] = s;
        }
    memcpy(o, t, sizeof t);
}

/* the palette product pcomp = post x game (SEAM rule: one product, this order). post: C3D_Mtx memory order. game: the walker matrix m[col][row], placed in C3D_Mtx order (x last) */
void nd_mtx_pcomp(float o[16], const float post[16], const float m[4][4]) {
    float g[16];
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) g[4 * r + (3 - c)] = m[c][r];
    nd_mtx_cmul(o, post, g);
}


#ifdef TARGET_3DS

/* ---- M: moved from pm_gbi.c (plan_s10_path group M, GM-1): matrix load, viewport, movemem, popmtx, record lights ---- */
float combined[4][4];

/* Viewport scale/translate after the 2-bit fraction. Full 320x240 until a display list sets one. */
float vp_sx = 160.f, vp_sy = 120.f, vp_sz = 127.75f;
float vp_tx = 160.f, vp_ty = 120.f, vp_tz = 127.75f;

unsigned gm_last = 0xffffu; /* GV1: the gm_tab entry of the last G_VTX */

unsigned gm_steps; /* W1: entries compared by gv_matrix_id, summed over the report window */

float (*gm_tab)[4][4]; /* matrices of the frame, 256 */

float (*gm_nrm)[3][4]; /* GV5: the rows 0 to 2 of the model view matrix of each gm_tab entry (the normal matrix), made when the entry is made */

unsigned gm_n;

extern float pm_eye_e, pm_eye_conv;

void upload_mvp(void) {
    nd_mtx_combine(combined, pm_eye_e, pm_eye_conv); /* S1-4: modelview x projection, STEREO eye shift on a perspective matrix */
    pm_gpu_note_mvp(combined, vp_sx, vp_sy, vp_tx, vp_ty); /* GV1c: the matrix is in the palette of the triangles */
}

void pm_gbi_eye_changed(void) { upload_mvp(); }

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
    if (lm) {}
    else if (cached && mtx_tag[slot] == tag) memcpy(in, mtx_cache[slot], sizeof(in));
    else {
        nd_mtx_unpack((const u32*)src->m, in);
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
#endif /* TARGET_3DS */
