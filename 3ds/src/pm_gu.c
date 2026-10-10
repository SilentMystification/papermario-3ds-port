#include <math.h>
#include "ultra64.h"

/* libultra matrix ops the decomp calls. The fixed layout matches pm_gbi's unpack:
 * one u32 holds the integer halves of a column pair, the word 8 later holds the fractions. */

void guMtxF2L(float mf[4][4], Mtx* m);

void guMtxIdentF(float mf[4][4]) {
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) mf[i][j] = (i == j) ? 1.f : 0.f;
    }
}

void guMtxIdent(Mtx* m) {
    float f[4][4];
    guMtxIdentF(f);
    guMtxF2L(f, m);
}

void guMtxCatF(float mf[4][4], float nf[4][4], float res[4][4]) {
    float t[4][4];
    int i, j, k;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            float s = 0.f;
            for (k = 0; k < 4; k++) s += mf[i][k] * nf[k][j];
            t[i][j] = s;
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) res[i][j] = t[i][j];
    }
}

void guMtxF2L(float mf[4][4], Mtx* m) {
    u32* ai = (u32*)m->m;
    u32* af = ai + 8;
    int i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 2; j++) {
            s32 e1 = (s32)(mf[i][j * 2] * 65536.f);
            s32 e2 = (s32)(mf[i][j * 2 + 1] * 65536.f);
            *ai++ = ((u32)e1 & 0xffff0000u) | (((u32)e2 >> 16) & 0xffffu);
            *af++ = (((u32)e1 << 16) & 0xffff0000u) | ((u32)e2 & 0xffffu);
        }
    }
}

void guMtxL2F(float mf[4][4], Mtx* m) {
    const u32* w = (const u32*)m->m;
    int r, c;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            int idx = r * 2 + (c >> 1);
            u32 sh = (c & 1) ? 0u : 16u;
            s16 ip = (s16)((w[idx] >> sh) & 0xffffu);
            s16 fp = (s16)((w[idx + 8] >> sh) & 0xffffu);
            s32 fixed = ((s32)ip << 16) | (u16)fp;
            mf[r][c] = (float)fixed / 65536.f;
        }
    }
}

void guMtxXFMF(float mf[4][4], float x, float y, float z, float* ox, float* oy, float* oz) {
    *ox = mf[0][0] * x + mf[1][0] * y + mf[2][0] * z + mf[3][0];
    *oy = mf[0][1] * x + mf[1][1] * y + mf[2][1] * z + mf[3][1];
    *oz = mf[0][2] * x + mf[1][2] * y + mf[2][2] * z + mf[3][2];
}

void guScaleF(float mf[4][4], float x, float y, float z) {
    guMtxIdentF(mf);
    mf[0][0] = x;
    mf[1][1] = y;
    mf[2][2] = z;
}

void guScale(Mtx* m, float x, float y, float z) {
    float f[4][4];
    guScaleF(f, x, y, z);
    guMtxF2L(f, m);
}

void guRotateRPYF(float mf[4][4], float r, float p, float h) {
    float dtor = 3.1415926f / 180.f;
    float sinr, sinp, sinh, cosr, cosp, cosh;
    r *= dtor;
    p *= dtor;
    h *= dtor;
    sinr = sinf(r);
    cosr = cosf(r);
    sinp = sinf(p);
    cosp = cosf(p);
    sinh = sinf(h);
    cosh = cosf(h);
    guMtxIdentF(mf);
    mf[0][0] = cosp * cosh;
    mf[0][1] = cosp * sinh;
    mf[0][2] = -sinp;
    mf[1][0] = sinr * sinp * cosh - cosr * sinh;
    mf[1][1] = sinr * sinp * sinh + cosr * cosh;
    mf[1][2] = sinr * cosp;
    mf[2][0] = cosr * sinp * cosh + sinr * sinh;
    mf[2][1] = cosr * sinp * sinh - sinr * cosh;
    mf[2][2] = cosr * cosp;
}

void guTranslateF(float mf[4][4], float x, float y, float z) {
    guMtxIdentF(mf);
    mf[3][0] = x;
    mf[3][1] = y;
    mf[3][2] = z;
}

void guNormalize(float* x, float* y, float* z) {
    float len = (*x) * (*x) + (*y) * (*y) + (*z) * (*z);
    if (len < 1e-12f) return;
    len = 1.f / sqrtf(len);
    *x *= len;
    *y *= len;
    *z *= len;
}
