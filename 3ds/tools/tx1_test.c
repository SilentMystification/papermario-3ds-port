/* TX1 host test: tx1_build and tx1_build_dual (3ds/src/pm_tex_tx1.h) must give the bytes of expand(), expand_dual() and the tile copy of
 * load_inner in pm_tex.c (reference code extracted by texbake_ref.py). Inputs: the texbake corpus (real textures, masks, mirror, stride,
 * palettes), a native u16 variant of every RGBA16 request, and random shaded (dual) cases. Exit 1 on any difference. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#define MAX_DIM 512
typedef enum { GPU_RGBA8 = 0, GPU_RGBA5551 = 2, GPU_LA8 = 5, GPU_L8 = 7, GPU_LA4 = 9, GPU_L4 = 10 } GPU_TEXCOLOR;
static u8 scratch[512 * 1024 * 2];
static unsigned mk_ws, mk_wt; static int mk_on, mk_ms, mk_mt, src_native;
static void pm_log_w(const char* f, ...) { (void)f; }
#define TEXBAKE_LIB
#include "texbake.c"
#include "../../build3ds/texbake_ref.h"
#include "../src/pm_tex_tx1.h"

static u8 mine[512 * 1024] __attribute__((aligned(16)));
typedef struct { int n, bad, native, dual; } St;
static void one(int i, const TbReq* q, const uint8_t* tl, const uint8_t* src, int native, St* s) {
    mk_on = 1; mk_ws = q->mw; mk_wt = q->mh; mk_ms = q->ms; mk_mt = q->mt; src_native = native;
    int pw = (int)pot_at_least_8(q->w), ph = (int)pot_at_least_8(q->h);
    GPU_TEXCOLOR g = gpu_fmt(q->fmt, q->siz); if (q->fmt == FMT_CI) g = GPU_RGBA5551;
    const u8* tlut = q->tlut_n ? tl : NULL;
    if (!expand(src, q->fmt, q->siz, q->w, q->h, q->stride, tlut, q->tlut_n, pw, ph, g)) return;
    unsigned bpp = (g == GPU_RGBA8) ? 4u : 2u;
    const u8* up = ref_tile(bpp, pw, ph);
    int mw = (mk_ws && (int)mk_ws < q->w) ? (int)mk_ws : 0, mh = (mk_wt && (int)mk_wt < q->h) ? (int)mk_wt : 0;
    memset(mine, 0xa5, sizeof mine);
    tx1_build(mine, src, q->fmt, q->siz, q->w, q->h, q->stride, tlut, q->tlut_n, pw, ph, (int)bpp, mw, mh, q->ms, q->mt, native);
    s->n++; s->native += native;
    if (memcmp(up, mine, (size_t)pw * (size_t)ph * bpp)) { s->bad++; if (s->bad < 10) printf("req %d: MISMATCH fmt %u siz %u %dx%d stride %d mask %u %u native %d\n", i, q->fmt, q->siz, q->w, q->h, q->stride, q->mw, q->mh, native); }
}
static void chk(int i, const TbReq* q, const uint8_t* tl, const uint8_t* src, void* u) {
    St* s = u;
    one(i, q, tl, src, 0, s);
    if (q->fmt == FMT_RGBA && q->siz == SIZ_16) one(i, q, tl, src, 1, s);
}
int main(int argc, char** argv) {
    if (argc < 2) return 2;
    long n; uint8_t* b = slurp(argv[1], &n); St s; memset(&s, 0, sizeof s);
    tb_walk(b, n, chk, &s);
    /* shaded path: random 4 bit data, random lut, sizes and shifts */
    srand(5);
    static u8 d[300 * 300]; static unsigned lut[256];
    for (int t = 0; t < 400; t++) {
        int w = 1 + rand() % 150, h = 1 + rand() % 150, stride = w + rand() % 3, dx = rand() % 17 - 8, dy = rand() % 17 - 8;
        for (unsigned k = 0; k < sizeof d; k++) d[k] = (u8)rand();
        for (int k = 0; k < 256; k++) lut[k] = ((unsigned)rand() << 16) ^ (unsigned)rand();
        int pw = (int)pot_at_least_8(w), ph = (int)pot_at_least_8(h);
        if (!expand_dual(d, w, h, stride, lut, dx, dy, pw, ph)) continue;
        const u8* up = ref_tile(4, pw, ph);
        memset(mine, 0x5a, sizeof mine);
        tx1_build_dual(mine, d, w, h, stride, lut, dx, dy, pw, ph);
        s.n++; s.dual++;
        if (memcmp(up, mine, (size_t)pw * (size_t)ph * 4)) { s.bad++; if (s.bad < 10) printf("dual %dx%d stride %d dx %d dy %d: MISMATCH\n", w, h, stride, dx, dy); }
    }
    printf("tx1_test: cases %d (native %d, dual %d), mismatches %d\n", s.n, s.native, s.dual, s.bad);
    return s.bad ? 1 : 0;
}
