/* NPSO3: nd_psotab_test.c, the table of gen_pso_tab.py (nd_pso_tab.c, made from 3ds/tests/pso) against nd_pso_build.
 * For id i, nd_pso(i) must equal byte for byte nd_pso_build of the same description. --fault compares id i with description
 * (i + 1) % 2: the bad count must then be above 0. Prints "psotab entries <n> bad <k>". */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nd.h"
#include "nd_api.h"
#include "../tests/pso/fix_descs.c"

extern const unsigned nd_pso_n; /* nd_pso_tab.c */

uint32_t f32tof24(float vf) { /* host copy of libctru f32tof24, as nd_decide_test.c has it */
    union { float f; uint32_t i; } v;
    v.f = vf;
    uint32_t sign = v.i >> 31, mant = (v.i & 0x7FFFFFu) >> 7;
    int exp = (int)((v.i >> 23) & 0xFF);
    if (exp == 0) return sign << 23;
    exp = exp - 127 + 63;
    if (exp < 0) return sign << 23;
    if (exp > 0x7F) return sign << 23 | 0x7Fu << 16;
    return mant | (uint32_t)exp << 16 | sign << 23;
}

int main(int argc, char** argv) {
    const NdPsoDesc* const descs[2] = { &pso_fix_a, &pso_fix_b };
    int fault = argc > 1 && strcmp(argv[1], "--fault") == 0;
    unsigned i, bad = 0;
    for (i = 0; i < 2; i++) {
        NdPsoOut ref;
        const NdPsoOut* got = nd_pso((NdPso)i);
        nd_pso_build(descs[fault ? (i + 1) % 2 : i], &ref);
        if (!got || memcmp(got, &ref, sizeof ref) != 0) bad++;
    }
    printf("psotab entries %u bad %u\n", nd_pso_n, bad);
    return 0;
}
