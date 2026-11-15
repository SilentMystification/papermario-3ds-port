/* MESHST host test: nd_mesh_build copies the animator mesh draw (animator.c) the same way the old per-frame copy did. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "common.h"
#include "nd_mesh.h"

static void ref_copy(Vtx* out, const Vtx* vl, int vso, const void* base, uint32_t w1, unsigned vi, unsigned n) {
    uintptr_t b = (uintptr_t)w1 + (uintptr_t)(vso + (int)vi) * 6u;
    if (base != NULL) b = (b & 0xFFFFFFu) + (uintptr_t)base;
    const Vec3s* s = (const Vec3s*)b;
    for (unsigned j = 0; j < n; j++) {
        out[j] = vl[vi + j];
        out[j].v.ob[0] = s[j].x;
        out[j].v.ob[1] = s[j].y;
        out[j].v.ob[2] = s[j].z;
    }
}

static Vtx vl[8];
static Vec3s pos[10];
static Vtx va[16];
static uint64_t buf[64];

static int run_case(int k) {
    const uint32_t w1 = k == 0 ? (uint32_t)(uintptr_t)pos : k == 1 ? 0x0A000000u : (uint32_t)(uintptr_t)va;
    const void* base = k == 1 ? (const void*)pos : NULL;
    const Vtx* vtab = k == 2 ? NULL : vl;
    const uint32_t units[6] = { ND_OP_VTX_W0(3, 0), w1, NDO_W0(NDO_END, 0), 0x5A5A5A5Au, ND_OP_VTX_W0(5, 3), w1 };
    NdMesh* m = (NdMesh*)(void*)buf;
    Vtx ref[8];

    if (nd_mesh_size(units, 3) != 184u) return 1;
    memset(buf, 0, sizeof(buf));
    nd_mesh_build(m, units, 3, vtab, 2, base);
    if (m->d.nu != 3u || m->d.ntri != 0u || m->d.fl != 0u) return 1;
    if (m->d.u[0] != units[0] || m->d.u[2] != units[2] || m->d.u[3] != units[3] || m->d.u[4] != units[4]) return 1;
    if (m->d.u[3] != 0x5A5A5A5Au) return 1;
    if (k == 2) {
        if (m->d.u[1] != w1 + 2u * 16u) return 1;
        if (m->d.u[5] != w1 + 5u * 16u) return 1;
        return 0;
    }
    ref_copy(ref, vl, 2, base, w1, 0, 3);
    if (memcmp((const void*)(uintptr_t)m->d.u[1], ref, 3 * sizeof(Vtx)) != 0) return 1;
    ref_copy(ref, vl, 2, base, w1, 3, 5);
    if (memcmp((const void*)(uintptr_t)m->d.u[5], ref, 5 * sizeof(Vtx)) != 0) return 1;
    return 0;
}

int main(void) {
    int k;
    for (k = 0; k < 8; k++) for (unsigned j = 0; j < 16; j++) ((uint8_t*)&vl[k])[j] = (uint8_t)(k * 16 + (int)j);
    for (int i = 0; i < 10; i++) { pos[i].x = (s16)(100 + i); pos[i].y = (s16)(200 + i); pos[i].z = (s16)(-i); }
    for (int i = 0; i < 16; i++) for (int j = 0; j < 16; j++) ((uint8_t*)&va[i])[j] = (uint8_t)(255 - j - i);
    for (k = 0; k < 3; k++) {
        if (run_case(k)) { printf("mesh FAIL case %d\n", k); return 1; }
    }
    printf("mesh ok\n");
    return 0;
}
