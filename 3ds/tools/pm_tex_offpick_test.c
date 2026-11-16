/* pm_tex_offpick_test.c: host test of pm_offt_pick (pm_tex_offpick.h, WAVEWIDE). The two frame buffers are adjacent (153600 bytes) and a full
 * slot spans 256 rows of 640 bytes, so the slot of buffer 0 also holds rows 0..15 of buffer 1. The pick must take the slot with the nearest base.
 * Usage: pm_tex_offpick_test (no arguments). Prints "offpick ok 8/8" or "offpick FAIL <case>" (exit 1). */
#include <stdio.h>
#include "pm_tex_offpick.h"

static unsigned char mem[2 * 153600 + 4096];
static int n_ok;

static int check(const char* name, const PmOffT* t, int n, const unsigned char* img, unsigned gs, int want, unsigned want_d) {
    unsigned d = 0xffffffffu;
    const int got = pm_offt_pick(t, n, img, gs, &d);
    if (got != want || (want >= 0 && d != want_d)) {
        printf("offpick FAIL %s\n", name);
        return 1;
    }
    n_ok++;
    return 0;
}

int main(void) {
    unsigned char* const b0 = mem + 1024; /* gFrameBuf0 */
    unsigned char* const b1 = b0 + 153600; /* gFrameBuf1, adjacent */
    /* wide: slot base = buffer - 80 (pm_gpu.c pm_gpu_off_begin). Slot 0 = the old frame (grab_prev), slot 1 = the frame drawn now. */
    const PmOffT w[2] = { { b0 - 80, 320, 0, 2, 512, 256 }, { b1 - 80, 320, 0, 2, 512, 256 } };
    const PmOffT nw[2] = { { b0, 320, 0, 2, 512, 256 }, { b1, 320, 0, 2, 512, 256 } };
    const PmOffT one[1] = { { b0, 320, 0, 2, 512, 256 } };
    const PmOffT s64[1] = { { b0, 64, 0, 3, 64, 64 } };
    if (check("adj", w, 2, b1, 2, 1, 80)) return 1;
    if (check("adjnw", nw, 2, b1, 2, 1, 0)) return 1;
    if (check("row80", w, 2, b0 + 80 * 640, 2, 0, 80 * 640 + 80)) return 1;
    if (check("tail", one, 1, b1 + 6400, 2, 0, 153600 + 6400)) return 1;
    if (check("siz", nw, 2, b1, 1, -1, 0)) return 1;
    if (check("below", one, 1, b0 - 2, 2, -1, 0)) return 1;
    if (check("past", one, 1, b0 + 640 * 256, 2, -1, 0)) return 1;
    if (check("s64", s64, 1, b0 + 63 * 256 + 4, 3, 0, 63 * 256 + 4)) return 1;
    printf("offpick ok %d/8\n", n_ok);
    return 0;
}
