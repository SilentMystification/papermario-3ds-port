/* IMGFX host test: the build-time anim units (nd_imgfx_tab.c, gen_imgfx_tab.py) keep the list shape that imgfx.c copies.
 * Case 0: the last unit of a list is ND_FS_END. Case 1: unit 0 is ND_FS_VTX and its b is a multiple of 16.
 * Case 2: every unit before the last is ND_FS_VTX, ND_FS_TRI1 or ND_FS_TRI2. The seed (END as TRI1) must fail case 0. */
#include <stdio.h>
#include <stdint.h>
#include "nd_fs.h"
#include "nd_imgfx.h"

static unsigned id_of(uint32_t w0) { return w0 >> 24; }

int main(void) {
    unsigned i, j, total = 0;
    for (i = 0; i < ND_IMGFX_N; i++) {
        const uint32_t* u = nd_imgfx_tab[i];
        const unsigned n = nd_imgfx_n[i];
        if (n == 0u) {
            printf("imgfxtab FAIL case 0\n");
            return 1;
        }
        total += n;
        if (id_of(u[2u * (n - 1u)]) != ND_FS_END) {
            printf("imgfxtab FAIL case 0\n");
            return 1;
        }
        if (id_of(u[0]) != ND_FS_VTX || (u[1] % 16u) != 0u) {
            printf("imgfxtab FAIL case 1\n");
            return 1;
        }
        for (j = 0; j + 1u < n; j++) {
            const unsigned id = id_of(u[2u * j]);
            if (id != ND_FS_VTX && id != ND_FS_TRI1 && id != ND_FS_TRI2) {
                printf("imgfxtab FAIL case 2\n");
                return 1;
            }
        }
    }
    printf("imgfxtab ok %u lists %u units\n", (unsigned)ND_IMGFX_N, total);
    return 0;
}
