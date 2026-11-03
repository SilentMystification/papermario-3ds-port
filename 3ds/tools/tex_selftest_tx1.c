/* TEXSELF2: dump the 3DS video memory image of every corpus request, as the port's TX1 builders write it
 * (pm_tex_tx1.h tx1_build and tx1_build_native, the path load_inner takes for every texture up to 1024 wide).
 * Output per request: int32 index, pw, ph, bpp (0 = not built), then pw*ph*bpp bytes in memory order.
 * Build (repo root, container): gcc -O2 -o build3ds/texbake/tex_selftest_tx1 3ds/tools/tex_selftest_tx1.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
enum { FMT_RGBA, FMT_YUV, FMT_CI, FMT_IA, FMT_I };
enum { SIZ_4, SIZ_8, SIZ_16, SIZ_32 };
typedef enum { GPU_RGBA8 = 0, GPU_RGBA5551 = 2, GPU_LA8 = 5, GPU_L8 = 7, GPU_LA4 = 9, GPU_L4 = 10 } GPU_TEXCOLOR;
static unsigned mk_ws, mk_wt; static int mk_on, mk_ms, mk_mt;
#include "../src/pm_tex_tx1.h"
static int pot8(int v) { int p = 8; while (p < v) p *= 2; return p; }
static u8 outb[1024 * 1024 * 4];
int main(int argc, char** argv) {
    if (argc < 3) return 2;
    FILE* f = fopen(argv[1], "rb"); if (!f) return 3;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    u8* b = malloc(n); if (fread(b, 1, n, f) != (size_t)n) return 4; fclose(f);
    FILE* o = fopen(argv[2], "wb"); if (!o) return 5;
    u32 magic, cnt; memcpy(&magic, b, 4); memcpy(&cnt, b + 4, 4);
    long off = 8;
    for (u32 i = 0; i < cnt; i++) {
        u8 fmt = b[off], siz = b[off + 1], ms = b[off + 2], mt = b[off + 3];
        u16 w, h, stride, mw, mh, tn; u32 ln;
        memcpy(&w, b + off + 4, 2); memcpy(&h, b + off + 6, 2); memcpy(&stride, b + off + 8, 2);
        memcpy(&mw, b + off + 10, 2); memcpy(&mh, b + off + 12, 2); memcpy(&tn, b + off + 14, 2);
        memcpy(&ln, b + off + 20, 4);
        off += 24;
        const u8* tl = b + off; off += tn * 2;
        const u8* src = b + off; off += ln;
        GPU_TEXCOLOR gf;
        if (fmt == FMT_RGBA && siz == SIZ_32) gf = GPU_RGBA8;
        else if (fmt == FMT_IA && siz == SIZ_16) gf = GPU_LA8;
        else if (fmt == FMT_IA && (siz == SIZ_8 || siz == SIZ_4)) gf = GPU_LA4;
        else if (fmt == FMT_I && siz == SIZ_8) gf = GPU_L8;
        else if (fmt == FMT_I && siz == SIZ_4) gf = GPU_LA4;
        else gf = GPU_RGBA5551;
        int pw = pot8(w), ph = pot8(h);
        int native = (gf == GPU_L8 || gf == GPU_L4 || gf == GPU_LA4);
        unsigned bpp = (gf == GPU_RGBA8) ? 4u : native ? 1u : 2u;
        int mw1 = (mk_on = 1, mk_ms = ms, mk_mt = mt, mk_ws = mw, mk_wt = mh, (mk_on && mk_ws && (int)mk_ws < w) ? (int)mk_ws : 0);
        int mh1 = (mk_on && mk_wt && (int)mk_wt < h) ? (int)mk_wt : 0;
        int ok = 1;
        if (fmt == FMT_YUV || (fmt == FMT_I && siz == SIZ_16) || fmt > FMT_I) ok = 0;
        if (ok) {
            if (native) tx1_build_native(outb, gf, fmt, siz, src, w, h, stride, pw, ph, mw1, mh1, ms, mt);
            else tx1_build(outb, src, fmt, siz, w, h, stride, tl, tn, pw, ph, (int)bpp, mw1, mh1, ms, mt, 0);
        }
        int hdr[4] = { (int)i, pw, ph, ok ? (int)bpp : 0 };
        fwrite(hdr, sizeof hdr, 1, o);
        if (ok) fwrite(outb, 1, (size_t)pw * ph * bpp, o);
    }
    fclose(o);
    return 0;
}
