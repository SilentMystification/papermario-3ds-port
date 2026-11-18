/* VCOPY host test: the unit copy of mdl_local_gfx_update_vtx_pointers (extracted from src/model.c, the real code) against the words the old path
 * produced (a frozen reference loop), and the device entry of nd_vcopy.h against counts made by the test itself. Prints "vcopy ok <n>" or "vcopy FAIL <what>". */
#include "common.h"
#include "nd_vcopy.h"
#include <stdio.h>
#include <string.h>
void mdl_local_gfx_update_vtx_pointers(const void* nodeDlist, Vtx* baseVtx, uint32_t* arg2, Vtx* arg3);

static void ref_copy(const uint32_t* src, uint32_t* dst, uintptr_t baseVtx, uintptr_t newVtx) {
    unsigned i = 0;
    for (;; i++) {
        uint32_t w0 = src[2 * i], w1 = src[2 * i + 1];
        if ((w0 >> 24) == ND_FS_VTX) w1 = (uint32_t)(newVtx + (w1 - baseVtx));
        dst[2 * i] = w0; dst[2 * i + 1] = w1;
        if ((w0 >> 24) == ND_FS_END) break;
    }
}

int main(void) {
    static Vtx base[64], nv[64];
    int n = 0, c;
    for (c = 0; c < 6; c++) {
        uint32_t src[256], a[256], b[256];
        unsigned k = 0, ntri = 0, nu, j;
        NdNDev d;
        memset(a, 0, sizeof a); memset(b, 0, sizeof b);
        for (j = 0; j < 3 + (unsigned)c * 2; j++) {
            src[2 * k] = ((uint32_t)ND_FS_VTX << 24) | (4u << 12) | j; src[2 * k + 1] = (uint32_t)(uintptr_t)(base + 4 * j); k++;
            src[2 * k] = ((uint32_t)ND_FS_TRI1 << 24) | j; src[2 * k + 1] = 0x11; k++; ntri += 1;
            if (j & 1) { src[2 * k] = ((uint32_t)ND_FS_TRI2 << 24) | j; src[2 * k + 1] = 0x22; k++; ntri += 2; }
        }
        nu = k;
        src[2 * k] = (uint32_t)ND_FS_END << 24; src[2 * k + 1] = 0;
        mdl_local_gfx_update_vtx_pointers(src, base, a, nv);
        ref_copy(src, b, (uintptr_t)base, (uintptr_t)nv);
        if (memcmp(a, b, 8 * (nu + 1))) { printf("vcopy FAIL words case %d\n", c); return 1; }
        nd_vcopy_dev(&d, a);
        if (d.list != a || d.u != a || d.fl != 0 || d.nu != nu || d.ntri != ntri) { printf("vcopy FAIL dev case %d nu %u/%u ntri %u/%u\n", c, d.nu, nu, d.ntri, ntri); return 1; }
        n++;
    }
    printf("vcopy ok %d\n", n);
    return 0;
}
