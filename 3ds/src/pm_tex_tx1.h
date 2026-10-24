/* TX1: one-pass texture expand and tile. Writes the 8x8 Morton tiles of the PICA texture straight from the source rows.
 * The result is byte equal to expand() (or expand_dual()) followed by the tile copy of load_inner in pm_tex.c.
 * No scratch image and no C3D_TexLoadImage copy. Used by pm_tex.c and by the host test 3ds/tools/tx1_test.c.
 * The includer defines u8 u16 u32, FMT_* and SIZ_*. Only RGBA5551, LA8 and RGBA8 targets (2 and 4 bytes per texel). */
#ifndef PM_TEX_TX1_H
#define PM_TEX_TX1_H
#include <string.h>

#define TX1_MAXD 1024

static unsigned char tx1_mort[64];
static unsigned short tx1_ia8[256], tx1_ia4[16];
static unsigned short tx1_sx[TX1_MAXD], tx1_sy[TX1_MAXD];
static int tx1_ready;
static unsigned tx1_bad_ci; /* CI texels with an index outside the palette in the last build */

static void tx1_init(void) {
    if (tx1_ready) return;
    for (unsigned y = 0; y < 8; y++)
        for (unsigned x = 0; x < 8; x++)
            tx1_mort[y * 8 + x] = (unsigned char)((x & 1u) | ((y & 1u) << 1) | ((x & 2u) << 1) | ((y & 2u) << 2) | ((x & 4u) << 2) | ((y & 4u) << 3));
    for (unsigned c = 0; c < 256; c++) tx1_ia8[c] = (unsigned short)(((c & 15u) * 17u) | (((c >> 4) * 17u) << 8));
    for (unsigned n = 0; n < 16; n++) tx1_ia4[n] = (unsigned short)(((n & 1u) ? 255u : 0u) | (((n >> 1) * 36u) << 8));
    tx1_ready = 1;
}

static inline unsigned tx1_be16(const u8* p) { return ((unsigned)p[0] << 8) | p[1]; }
static inline unsigned tx1_n4(const u8* src, int idx, int sx) { unsigned b = src[idx >> 1]; return (sx & 1) ? (b & 15u) : (b >> 4); }
/* source coordinate of output coordinate v: the tile mask wraps or mirrors, else the edge repeats */
static inline int tx1_map(int v, int n, int m, int mir) {
    if (m) {
        int md = 2 * m;
        int r = (mir ? ((md & (md - 1)) == 0 ? (v & (md - 1)) : v % md) : ((m & (m - 1)) == 0 ? (v & (m - 1)) : v % m));
        return (mir && r >= m) ? 2 * m - 1 - r : r;
    }
    return v < n ? v : n - 1;
}

#define TX1_TILES(T, EXPR) do { \
    const int tw_ = pw >> 3, th_ = ph >> 3; \
    for (int ty = 0; ty < th_; ty++) \
        for (int tx = 0; tx < tw_; tx++) { \
            T* d = (T*)dst + ((size_t)ty * (size_t)tw_ + (size_t)tx) * 64u; \
            const unsigned short* xs = tx1_sx + tx * 8; \
            for (int r = 0; r < 8; r++) { \
                const int rb = (int)tx1_sy[ph - 1 - (ty * 8 + r)] * stride; \
                const unsigned char* mo = tx1_mort + r * 8; \
                for (int c = 0; c < 8; c++) { const int sx = xs[c]; (void)rb; (void)sx; d[mo[c]] = (T)(EXPR); } \
            } \
        } \
} while (0)

/* dst: pw * ph * bpp bytes, 4 byte aligned. pw and ph are powers of two from 8 to 512. mw and mh are the tile masks as expand() computes them (0 = none). */
static void tx1_build(u8* dst, const u8* src, unsigned fmt, unsigned siz, int w, int h, int stride, const u8* tlut, int tlut_n, int pw, int ph, int bpp,
                      int mw, int mh, int mir_s, int mir_t, int native) {
    tx1_init();
    for (int x = 0; x < pw; x++) tx1_sx[x] = (unsigned short)tx1_map(x, w, mw, mir_s);
    for (int y = 0; y < ph; y++) tx1_sy[y] = (unsigned short)tx1_map(y, h, mh, mir_t);
    tx1_bad_ci = 0;
    unsigned bad = 0;
    if (fmt == FMT_CI) {
        unsigned short pal[256];
        int np = siz == SIZ_4 ? 16 : 256;
        for (int i = 0; i < np; i++) pal[i] = (unsigned short)((tlut && i < tlut_n) ? tx1_be16(tlut + i * 2) : 0xffffu);
        int tn = tlut ? tlut_n : 0;
        if (siz == SIZ_4) TX1_TILES(unsigned short, ({ unsigned i_ = tx1_n4(src, rb + sx, sx); bad += (int)i_ >= tn; pal[i_]; }));
        else TX1_TILES(unsigned short, ({ unsigned i_ = src[rb + sx]; bad += (int)i_ >= tn; pal[i_]; }));
    } else if (fmt == FMT_RGBA && siz == SIZ_16) {
        if (native) TX1_TILES(unsigned short, ({ const u8* p_ = src + (rb + sx) * 2; ((unsigned)p_[1] << 8) | p_[0]; }));
        else TX1_TILES(unsigned short, tx1_be16(src + (rb + sx) * 2));
    } else if (fmt == FMT_RGBA && siz == SIZ_32) {
        TX1_TILES(unsigned, ({ const u8* p_ = src + (rb + sx) * 4; ((unsigned)p_[0] << 24) | ((unsigned)p_[1] << 16) | ((unsigned)p_[2] << 8) | p_[3]; }));
    } else if (fmt == FMT_IA && siz == SIZ_16) {
        TX1_TILES(unsigned short, tx1_be16(src + (rb + sx) * 2));
    } else {
        TX1_TILES(unsigned short, 0xffffu);
    }
    tx1_bad_ci = bad;
    (void)bpp;
}

/* TX-B: PICA-native L8, L4, LA4 tiles (no LA8 expand). */
static void tx1_build_native(u8* dst, GPU_TEXCOLOR gf, unsigned src_fmt, unsigned src_siz, const u8* src, int w, int h, int stride, int pw, int ph, int mw, int mh,
                             int mir_s, int mir_t) {
    tx1_init();
    for (int x = 0; x < pw; x++) tx1_sx[x] = (unsigned short)tx1_map(x, w, mw, mir_s);
    for (int y = 0; y < ph; y++) tx1_sy[y] = (unsigned short)tx1_map(y, h, mh, mir_t);
    const int tw = pw >> 3, th = ph >> 3;
    if (gf == GPU_L4) {
        for (int ty = 0; ty < th; ty++)
            for (int tx = 0; tx < tw; tx++) {
                u8* d = dst + ((size_t)ty * (size_t)tw + (size_t)tx) * 32u;
                memset(d, 0, 32);
                const unsigned short* xs = tx1_sx + tx * 8;
                for (int r = 0; r < 8; r++) {
                    const int rb = (int)tx1_sy[ph - 1 - (ty * 8 + r)] * stride;
                    const unsigned char* mo = tx1_mort + r * 8;
                    for (int c = 0; c < 8; c++) {
                        int sx = (int)xs[c];
                        unsigned nib = (unsigned)tx1_n4(src, rb + sx, sx);
                        unsigned m = mo[c];
                        unsigned bi = m >> 1;
                        if (m & 1u) d[bi] = (u8)((d[bi] & 0xf0u) | nib);
                        else d[bi] = (u8)((d[bi] & 0x0fu) | (nib << 4));
                    }
                }
            }
    } else if (gf == GPU_LA4 && src_fmt == FMT_I && src_siz == SIZ_4) {
        for (int ty = 0; ty < th; ty++)
            for (int tx = 0; tx < tw; tx++) {
                u8* d = dst + ((size_t)ty * (size_t)tw + (size_t)tx) * 64u;
                const unsigned short* xs = tx1_sx + tx * 8;
                for (int r = 0; r < 8; r++) {
                    const int rb = (int)tx1_sy[ph - 1 - (ty * 8 + r)] * stride;
                    const unsigned char* mo = tx1_mort + r * 8;
                    for (int c = 0; c < 8; c++) {
                        int sx = (int)xs[c];
                        unsigned nib = (unsigned)tx1_n4(src, rb + sx, sx);
                        d[mo[c]] = (u8)((nib << 4) | nib);
                    }
                }
            }
    } else {
        TX1_TILES(unsigned char, src[rb + sx]);
    }
}

/* The shaded path (expand_dual): RGBA8 target, texels outside the w x h window are 0. */
static void tx1_build_dual(u8* dst, const u8* src, int w, int h, int stride, const unsigned* lut, int dx4, int dy4, int pw, int ph) {
    tx1_init();
    static unsigned short cx[TX1_MAXD], cy[TX1_MAXD];
    for (int x = 0; x < w; x++) { int s = (4 * x + 2 - dx4) >> 2; cx[x] = (unsigned short)(s < 0 ? 0 : s > w - 1 ? w - 1 : s); }
    for (int y = 0; y < h; y++) { int s = (4 * y + 2 - dy4) >> 2; cy[y] = (unsigned short)(s < 0 ? 0 : s > h - 1 ? h - 1 : s); }
    for (int x = 0; x < pw; x++) tx1_sx[x] = (unsigned short)(x < w ? x : 0xffffu);
    for (int y = 0; y < ph; y++) tx1_sy[y] = (unsigned short)(y < h ? y : 0xffffu);
    const int tw = pw >> 3, th = ph >> 3;
    for (int ty = 0; ty < th; ty++)
        for (int tx = 0; tx < tw; tx++) {
            unsigned* d = (unsigned*)dst + ((size_t)ty * (size_t)tw + (size_t)tx) * 64u;
            for (int r = 0; r < 8; r++) {
                const int y = ph - 1 - (ty * 8 + r);
                const unsigned char* mo = tx1_mort + r * 8;
                for (int c = 0; c < 8; c++) {
                    const int x = tx * 8 + c;
                    unsigned v = 0;
                    if (x < w && y < h) v = lut[tx1_n4(src, y * stride + x, x) * 16 + tx1_n4(src, cy[y] * stride + cx[x], cx[x])];
                    d[mo[c]] = v;
                }
            }
        }
}
#endif
