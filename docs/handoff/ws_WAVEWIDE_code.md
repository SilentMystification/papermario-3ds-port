# ws_WAVEWIDE_code: exact blocks (designer: oracle 37). LF line ends. Copy byte for byte.

## Block A: new file 3ds/include/pm_tex_offpick.h
```c
/* pm_tex_offpick.h: the off-screen slot that holds an image address (WAVEWIDE). pm_tex.c load_inner and pm_tex_off_rewin use it.
 * The two frame buffers are adjacent (gFrameBuf1 = gFrameBuf0 + 153600 bytes) and a full slot spans 256 rows of 640 bytes, so the slot
 * of gFrameBuf0 also holds the first 16 rows of gFrameBuf1. The slot with the nearest base (the smallest offset) is the image of the address. */
#ifndef PM_TEX_OFFPICK_H
#define PM_TEX_OFFPICK_H
typedef struct { const unsigned char* base; unsigned stride; void* tex; int siz, w, h; } PmOffT;
/* gs: 3 = 32 bit, 2 = 16 bit, 1 = 8 bit. Returns the slot index or -1. *d_out gets the byte offset of img in that slot. */
static inline int pm_offt_pick(const PmOffT* t, int n, const void* img, unsigned gs, unsigned* d_out) {
    const unsigned char* p = (const unsigned char*)img;
    const unsigned bpp = gs == 3 ? 4u : gs;
    int best = -1;
    unsigned bd = 0;
    for (int i = 0; gs && i < n; i++) {
        if (!t[i].base || (unsigned)t[i].siz != gs || p < t[i].base) continue;
        const unsigned d = (unsigned)(p - t[i].base);
        if (d >= t[i].stride * bpp * (unsigned)t[i].h) continue;
        if (best < 0 || d < bd) { best = i; bd = d; }
    }
    *d_out = bd;
    return best;
}
#endif
```

## Block B: 3ds/src/pm_tex.c line 9. Before: `#include "pm_tex.h"`  After (two lines):
```c
#include "pm_tex.h"
#include "pm_tex_offpick.h"
```

## Block C: 3ds/src/pm_tex.c line 139: the edit shape of ws_WAVEWIDE.md (offt becomes `static PmOffT offt[10];`).

## Block D: 3ds/src/pm_tex.c
D1, pm_tex_off_rewin (lines 171-173). Before:
```c
    if (!gs || (unsigned)offt[off_cur].siz != gs || (const u8*)img < offt[off_cur].base) return 0;
    const unsigned bpp = gs == 3 ? 4u : gs, d = (unsigned)((const u8*)img - offt[off_cur].base), row = offt[off_cur].stride * bpp;
    if (d >= row * (unsigned)offt[off_cur].h) return 0;
```
After:
```c
    unsigned d;
    if (!gs || pm_offt_pick(offt, offt_n, img, gs, &d) != off_cur) return 0; /* WAVEWIDE: only the nearest slot (pm_tex_offpick.h) */
    const unsigned bpp = gs == 3 ? 4u : gs, row = offt[off_cur].stride * bpp;
```
D2, load_inner (lines 1712-1729). Before: from `        for (int i = 0; gs && i < offt_n; i++) {` to the second `        }` after `                return 1;`
(the loop, 18 lines). After:
```c
        unsigned d;
        const int i = pm_offt_pick(offt, offt_n, img, gs, &d); /* WAVEWIDE: the nearest slot, not the first (pm_tex_offpick.h) */
        if (i >= 0) {
            unsigned bpp = gs == 3 ? 4u : gs, row = offt[i].stride * bpp;
            /* P15: a power of two row (and bpp) needs no divide. Same result. */
            if ((row & (row - 1)) == 0) {
                off_oy = (int)(d >> __builtin_ctz(row));
                off_ox = (int)((d & (row - 1)) / bpp);
            } else {
                off_oy = (int)(d / row);
                off_ox = (int)((d % row) / bpp);
            }
            off_cur = i;
            if (offt[i].w == 64) { static int n; if (n++ < 6) pm_log_d("X02: overlay reads 64x64 slot %d bits %d addr %p", i, gs == 3 ? 32 : gs == 2 ? 16 : 8, img); }
            have = 1;
            return 1;
        }
```
Keep the line `unsigned gs = ...` above it and the closing `    }` of the block below it.

## Block E1: new file 3ds/tools/pm_tex_offpick_test.c
```c
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
```

## Block E2: new file 3ds/tests/host.d/offpick.sh
```sh
#!/bin/sh
# WAVEWIDE: pm_offt_pick takes the slot with the nearest base. The seed (the old first-match rule) must fail the adjacent frame buffer case.
mkdir -p /tmp/offpick_seed
sed 's/if (best < 0 || d < bd) {/if (best < 0) {/' 3ds/include/pm_tex_offpick.h > /tmp/offpick_seed/pm_tex_offpick.h
gcc -m32 -Wall -Wextra -Werror -I /tmp/offpick_seed -o /tmp/offpick_seed/t 3ds/tools/pm_tex_offpick_test.c || exit 1
got=$(/tmp/offpick_seed/t || true); if [ "$got" != "offpick FAIL adj" ]; then echo "offpick seed FAIL: got [$got]"; exit 1; fi
gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/offpick_t 3ds/tools/pm_tex_offpick_test.c && /tmp/offpick_t || exit 1
```

## Block F: src/effects/underwater.c
F1, line 170. Before: `void pm_log(const char* fmt, ...);`  After (two lines):
```c
void pm_log(const char* fmt, ...);
extern int pm_wide; /* WAVEWIDE: pm_gpu.c */
```
F2, tidal_grid_fill (lines 184-189). Before:
```c
            xs = a * 16 + 16;
            if (a == 0) {
                xs -= 4;
            } else if (a == TIDAL_COLS - 1) {
                xs += 4;
            }
```
After:
```c
            xs = a * 16 + 16;
            if (a == 0) {
                xs -= 4;
            } else if (a == TIDAL_COLS - 1) {
                xs += 4;
            }
            /* WAVEWIDE: in wide the end columns reach the screen edges: game x -40 and 360 (screen 0 and 400, ui ortho 40 + x). The texel is the
             * same column: the grab slot holds all 400 columns. No vertex sits on the game edge, so the wide edge rule (pm_gpu.c wide_on_edge) moves none. */
            if (pm_wide && a == 0) {
                xs = -40;
            } else if (pm_wide && a == TIDAL_COLS - 1) {
                xs = SCREEN_WIDTH + 40;
            }
```
