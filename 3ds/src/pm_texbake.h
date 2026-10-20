/* PG2a: baked texture pack format and key. Plain C, no 3DS headers. The host tool 3ds/tools/texbake.c and the loader in pm_tex.c include this file.
 * A baked texture holds the exact bytes that load_inner passes to C3D_TexLoadImage (PICA tile order, row 0 stored last).
 * Pack file (little endian): TbHead, then TbRec[count] sorted by key, then the data. Data of one record starts at off (from the file start), is 16 byte aligned. */
#ifndef PM_TEXBAKE_H
#define PM_TEXBAKE_H
#include <stdint.h>
#include <string.h>

#define TB_MAGIC 0x42544d50u /* "PMTB" */
#define TB_VERSION 1u
#define TB_REQ_MAGIC 0x51544d50u /* "PMTQ" */

enum { TB_GF_RGBA8 = 0, TB_GF_RGBA5551 = 1, TB_GF_LA8 = 2 }; /* own values: the loader maps them to GPU_RGBA8, GPU_RGBA5551, GPU_LA8 */

typedef struct { uint32_t magic, version, count, data_off; } TbHead; /* 16 bytes */
typedef struct {
    uint64_t key;
    uint32_t off, bytes;
    uint16_t pw, ph;
    uint8_t gf, pad[3];
} TbRec; /* 24 bytes */

/* A texture request: what load_inner knows after the mask handling (width, height, stride, mask widths are final values).
 * Request file: u32 TB_REQ_MAGIC, u32 count, then per request TbReq, tlut bytes (tlut_n * 2), source bytes (src_len). */
typedef struct {
    uint8_t fmt, siz, ms, mt;       /* N64 fmt and siz as pm_tex.c enums, ms and mt = mirror flags of the tile */
    uint16_t w, h, stride;          /* texels */
    uint16_t mw, mh;                /* effective mask window as expand() computes it: 0 = none, else less than w or h */
    uint16_t tlut_n;                /* palette entries, 0 = no palette */
    uint16_t pad[2];
    uint32_t src_len;               /* bytes in the source range from the image pointer */
} TbReq; /* 24 bytes */

/* Bytes of source row y that expand() can read: texel range [y*stride, y*stride+ww), bits per texel from siz. Returns the byte start and sets *n. */
static inline unsigned tb_row_span(unsigned siz, int y, int stride, int ww, unsigned* n) {
    unsigned bits = siz == 0 ? 4u : siz == 1 ? 8u : siz == 2 ? 16u : 32u;
    unsigned a = ((unsigned)y * (unsigned)stride * bits) >> 3;
    unsigned b = (((unsigned)y * (unsigned)stride + (unsigned)ww) * bits + 7u) >> 3;
    *n = b - a;
    return a;
}

static inline uint64_t tb_fnv(uint64_t h, const uint8_t* p, unsigned n) {
    unsigned i = 0;
    for (; i + 4 <= n; i += 4) { uint32_t w; memcpy(&w, p + i, 4); h = (h ^ w) * 1099511628211ull; }
    for (; i < n; i++) h = (h ^ p[i]) * 1099511628211ull;
    return h;
}

/* Bytes of the source range a request needs: end of the last row that expand() reads. The dump hook writes this many bytes from the image pointer. */
static inline unsigned tb_src_len(const TbReq* q) {
    unsigned n, a = tb_row_span(q->siz, (q->mh ? q->mh : q->h) - 1, q->stride, q->mw ? q->mw : q->w, &n);
    return a + n;
}

/* Key of a plain texture: FNV-1a (64 bit, 32 bit words, then bytes) over the parameters, the palette bytes and the source bytes of the rows that expand() reads.
 * img is the image pointer (not hashed). Rows read: 0 to hh - 1, hh = mh ? mh : h, ww = mw ? mw : w. */
static inline uint64_t tb_key(const TbReq* q, const uint8_t* tlut, const uint8_t* img) {
    uint64_t h = 14695981039346656037ull;
    uint8_t pr[16] = { q->fmt, q->siz, q->ms, q->mt, (uint8_t)q->w, (uint8_t)(q->w >> 8), (uint8_t)q->h, (uint8_t)(q->h >> 8),
                       (uint8_t)q->stride, (uint8_t)(q->stride >> 8), (uint8_t)q->mw, (uint8_t)(q->mw >> 8), (uint8_t)q->mh, (uint8_t)(q->mh >> 8),
                       (uint8_t)q->tlut_n, (uint8_t)(q->tlut_n >> 8) };
    h = tb_fnv(h, pr, 16);
    if (q->tlut_n) h = tb_fnv(h, tlut, (unsigned)q->tlut_n * 2u);
    int hh = q->mh ? q->mh : q->h, ww = q->mw ? q->mw : q->w;
    for (int y = 0; y < hh; y++) { unsigned n, a = tb_row_span(q->siz, y, q->stride, ww, &n); h = tb_fnv(h, img + a, n); }
    return h ? h : 1;
}
#endif
