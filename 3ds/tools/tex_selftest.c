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
/* TEXSELF: dump the port's linear decode (expand, pm_tex.c via texbake_ref.h) for every request of the corpus.
 * Output per request: int32 index, pw, ph, bpp (0 = expand refused), then pw*ph*bpp bytes. Compare with tex_selftest.py. */
#define TEX_SCRATCH_PLANE (1024u * 512u * 2u)
#include "../../build3ds/texbake_ref.h"
static FILE* outf;
static void dump(int i, const TbReq* q, const uint8_t* tl, const uint8_t* src, void* u) {
    (void)u; GPU_TEXCOLOR g = gpu_fmt(q->fmt, q->siz); if (q->fmt == FMT_CI) g = GPU_RGBA5551;
    mk_on = 1; mk_ws = q->mw; mk_wt = q->mh; mk_ms = q->ms; mk_mt = q->mt; src_native = 0;
    unsigned rpw = pot_at_least_8(q->w), rph = pot_at_least_8(q->h);
    int ok = expand(src, q->fmt, q->siz, q->w, q->h, q->stride, q->tlut_n ? tl : NULL, q->tlut_n, rpw, rph, g);
    unsigned bpp = ok ? ((g == GPU_RGBA8) ? 4u : 2u) : 0u;
    if (g == GPU_L8 || g == GPU_L4 || g == GPU_LA4) bpp = ok ? 1u : 0u;
    int hdr[4] = { i, (int)rpw, (int)rph, (int)bpp };
    fwrite(hdr, sizeof hdr, 1, outf);
    if (ok) fwrite(scratch, 1, (size_t)rpw * rph * bpp, outf);
}
int main(int argc, char** argv) {
    if (argc < 3) return 2;
    long n; uint8_t* b = slurp(argv[1], &n);
    outf = fopen(argv[2], "wb"); if (!outf) return 3;
    tb_walk(b, n, dump, NULL);
    fclose(outf);
    return 0;
}
