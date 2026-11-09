/* nd_mtx.c: native matrix stack, N64 matrix unpack, eye shift and the palette product (plan_native_slices S1-4).
 * Pure C on floats, no citro3d, no game or GPU symbols except the interp hook (pm_interp.c): the host tests build it.
 * The G_MTX / G_MOVEMEM glue is nd_mtx_set.c (GM-1, target only). Build flags keep one rounding per operation (no x87 extended precision on the host). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nd.h"

float nd_mv[ND_MV_DEPTH][4][4];
ND_HOTD int nd_mv_sp;
float nd_proj[4][4];
float nd_proj_wn; /* EYECW: length of the xyz part of the w column of nd_proj (0: orthographic). nd_proj is view x perspective, so |p23| alone follows the camera yaw */

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
    nd_proj_wn = 0.f;
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
    if (is_proj) nd_proj_wn = sqrtf(nd_proj[0][3] * nd_proj[0][3] + nd_proj[1][3] * nd_proj[1][3] + nd_proj[2][3] * nd_proj[2][3]); /* EYECW: one sqrtf per projection load */
}

void nd_mtx_pop(unsigned count) {
    if (count == 0) count = 1;
    while (count-- && nd_mv_sp > 0) nd_mv_sp--;
}

/* combined = modelview x projection. STEREO: the eye shift is not in this matrix: the vertex shader adds it (pm4/pm5 eye, eyd), so both eyes share one matrix. */
void nd_mtx_combine(float c[4][4]) { nd_mtx_mul(c, nd_mv[nd_mv_sp], nd_proj); }

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


