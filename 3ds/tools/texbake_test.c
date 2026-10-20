/* PG2a host test: the baked bytes must equal the bytes that pm_tex.c load_inner passes to C3D_TexLoadImage (reference code extracted from pm_tex.c).
 * The mask, mirror, stride, pad and palette cases are in the corpus (texbake_corpus.py). */
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

typedef struct { int n, bad, skipped, fmtbad; unsigned long long bytes; } St;
static void chk(int i, const TbReq* q, const uint8_t* tl, const uint8_t* src, void* u) {
    St* s = u; static u8 mine[512 * 1024]; unsigned pw, ph; int gf;
    unsigned nb = tb_bake(q, tl, src, mine, &pw, &ph, &gf);
    /* reference: the real code. mask globals as load_inner sets them for expand() */
    mk_on = 1; mk_ws = q->mw; mk_wt = q->mh; mk_ms = q->ms; mk_mt = q->mt; src_native = 0;
    unsigned rpw = pot_at_least_8(q->w), rph = pot_at_least_8(q->h);
    GPU_TEXCOLOR g = gpu_fmt(q->fmt, q->siz); if (q->fmt == FMT_CI) g = GPU_RGBA5551;
    int ok = expand(src, q->fmt, q->siz, q->w, q->h, q->stride, q->tlut_n ? tl : NULL, q->tlut_n, rpw, rph, g);
    s->n++;
    if (!ok) { if (nb) { s->bad++; printf("req %d: ref over the size limit, tool baked\n", i); } else s->skipped++; return; }
    unsigned bpp = (g == GPU_RGBA8) ? 4u : 2u;
    unsigned rb = rpw * rph * bpp;
    const u8* up = ref_tile(bpp, (int)rpw, (int)rph);
    if (nb != rb || pw != rpw || ph != rph || memcmp(up, mine, rb)) {
        s->bad++; if (s->bad < 10) printf("req %d: MISMATCH fmt %u siz %u %dx%d stride %d mask %u %u bytes %u vs %u\n", i, q->fmt, q->siz, q->w, q->h, q->stride, q->mw, q->mh, nb, rb);
    }
    if ((g == GPU_RGBA8) != (gf == TB_GF_RGBA8) || (g == GPU_LA8) != (gf == TB_GF_LA8)) s->fmtbad++;
    s->bytes += rb;
}
int main(int argc, char** argv) {
    if (argc < 2) return 2;
    long n; uint8_t* b = slurp(argv[1], &n); St s; memset(&s, 0, sizeof s);
    tb_walk(b, n, chk, &s);
    printf("texbake_test: requests %d, mismatches %d, skipped (size limit, both) %d, format class errors %d, baked bytes %llu\n", s.n, s.bad, s.skipped, s.fmtbad, s.bytes);
    return (s.bad || s.fmtbad) ? 1 : 0;
}
