/* Driver for tex_conv_test.py. Runs the real TX1 converter (3ds/src/pm_tex_tx1.h) on each request of a request file.
 * The call sequence is the one of load_miss in pm_tex.c: gf from gpu_fmt, native_tex picks tx1_build_native, else tx1_build.
 * Request (little endian int32 fields, 13): fmt siz native w h stride mw_raw mh_raw ms mt tlut_n src_len tlut_len, then src bytes, tlut bytes.
 * Output per request: gf (int32), out_len (int32), out bytes. The window mask (mw_raw, mh_raw) is reduced as pm_tex.c does. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
enum { FMT_RGBA, FMT_YUV, FMT_CI, FMT_IA, FMT_I };
enum { SIZ_4, SIZ_8, SIZ_16, SIZ_32 };
typedef enum { GPU_RGBA8 = 0, GPU_RGBA5551 = 2, GPU_LA8 = 5, GPU_L8 = 7, GPU_LA4 = 9, GPU_L4 = 10 } GPU_TEXCOLOR;

/* verbatim from pm_tex.c gpu_fmt */
static GPU_TEXCOLOR gpu_fmt(unsigned fmt, unsigned siz) {
    if (fmt == FMT_RGBA && siz == SIZ_32) return GPU_RGBA8;
    if (fmt == FMT_IA && siz == SIZ_16) return GPU_LA8;
    if (fmt == FMT_IA && (siz == SIZ_8 || siz == SIZ_4)) return GPU_LA4;
    if (fmt == FMT_I && siz == SIZ_8) return GPU_L8;
    if (fmt == FMT_I && siz == SIZ_4) return GPU_LA4;
    return GPU_RGBA5551;
}

#include "../src/pm_tex_tx1.h"

static u8 src[1 << 20], tl[1 << 12], dst[1 << 20];
static int32_t rd_i32(FILE* f) { uint8_t b[4]; if (fread(b, 1, 4, f) != 4) return -1; return (int32_t)((uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24); }
static void wr_i32(FILE* f, int32_t v) { uint8_t b[4] = { (uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24) }; fwrite(b, 1, 4, f); }

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    FILE* in = fopen(argv[1], "rb");
    FILE* out = fopen(argv[2], "wb");
    if (!in || !out) return 2;
    int32_t f[13];
    for (;;) {
        f[0] = rd_i32(in);
        if (f[0] < 0) break;
        for (int i = 1; i < 13; i++) f[i] = rd_i32(in);
        unsigned fmt = f[0], siz = f[1];
        int native = f[2], w = f[3], h = f[4], stride = f[5], mwr = f[6], mhr = f[7], ms = f[8], mt = f[9], tlut_n = f[10];
        int src_len = f[11], tlut_len = f[12];
        if (fread(src, 1, src_len, in) != (size_t)src_len) return 2;
        if (fread(tl, 1, tlut_len, in) != (size_t)tlut_len) return 2;
        int pw = 8, ph = 8;
        while (pw < w) pw <<= 1;
        while (ph < h) ph <<= 1;
        GPU_TEXCOLOR gf = gpu_fmt(fmt, siz);
        if (fmt == FMT_CI) gf = GPU_RGBA5551;
        int native_tex = (gf == GPU_L8 || gf == GPU_L4 || gf == GPU_LA4);
        unsigned bpp0 = (gf == GPU_RGBA8) ? 4u : native_tex ? 1u : 2u;
        int mw = (mwr && mwr < w) ? mwr : 0;
        int mh = (mhr && mhr < h) ? mhr : 0;
        const u8* tlut = tlut_n ? tl : NULL;
        memset(dst, 0xa5, sizeof dst);
        if (native_tex) tx1_build_native(dst, gf, fmt, siz, src, w, h, stride, pw, ph, mw, mh, ms, mt);
        else tx1_build(dst, src, fmt, siz, w, h, stride, tlut, tlut_n, pw, ph, (int)bpp0, mw, mh, ms, mt, native);
        unsigned bpp = (gf == GPU_RGBA8) ? 4u : (gf == GPU_L8 || gf == GPU_LA4 || gf == GPU_L4) ? 1u : 2u;
        wr_i32(out, (int32_t)gf);
        wr_i32(out, (int32_t)((unsigned)pw * (unsigned)ph * bpp));
        fwrite(dst, 1, (size_t)pw * (size_t)ph * bpp, out);
    }
    fclose(in);
    fclose(out);
    return 0;
}
