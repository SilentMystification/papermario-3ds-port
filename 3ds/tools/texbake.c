/* PG2a texbake: bake texture requests into the PICA-native pack (format: 3ds/src/pm_texbake.h).
 * Usage: texbake bake <req.bin> <out.pak>      write the pack and print the size and memory estimate
 * The decode rules are the rules of expand() and of the tile copy in pm_tex.c load_inner. texbake_test.sh checks them against the real code. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/pm_texbake.h"

enum { FMT_RGBA, FMT_YUV, FMT_CI, FMT_IA, FMT_I };
enum { SIZ_4, SIZ_8, SIZ_16, SIZ_32 };

static unsigned pot8(int n) { int p = 8; while (p < n && p < 512) p <<= 1; return (unsigned)p; }
static unsigned be16(const uint8_t* p) { return ((unsigned)p[0] << 8) | p[1]; }
static int wmod(int y, int m) { return y % m; }
static int mapc(int v, int m, int mirror) { if (!m) return v; int r = wmod(v, mirror ? 2 * m : m); return (mirror && r >= m) ? 2 * m - 1 - r : r; }

/* gf as gpu_fmt() and load_inner give it for a plain texture (no lut) */
int tb_gf(unsigned fmt, unsigned siz) {
    if (fmt == FMT_CI) return TB_GF_RGBA5551;
    if (fmt == FMT_RGBA && siz == SIZ_32) return TB_GF_RGBA8;
    if (fmt == FMT_IA) return TB_GF_LA8;
    if (fmt == FMT_I && (siz == SIZ_8 || siz == SIZ_4)) return TB_GF_LA8;
    return TB_GF_RGBA5551;
}

/* one decoded texel inside the window, as expand() writes it into the linear scratch: up to 4 bytes */
static void dec(const TbReq* q, const uint8_t* tl, const uint8_t* src, int sx, int sy, uint8_t* o) {
    int stride = q->stride;
    unsigned fmt = q->fmt, siz = q->siz;
    if (fmt == FMT_CI) {
        unsigned idx;
        if (siz == SIZ_4) { unsigned b = src[(sy * stride + sx) >> 1]; idx = (sx & 1) ? (b & 15) : (b >> 4); }
        else idx = src[sy * stride + sx];
        unsigned pix = (q->tlut_n && (int)idx < q->tlut_n) ? be16(tl + idx * 2) : 0xffffu;
        o[0] = (uint8_t)pix; o[1] = (uint8_t)(pix >> 8);
    } else if (fmt == FMT_RGBA && siz == SIZ_16) {
        unsigned c = be16(src + (sy * stride + sx) * 2);
        o[0] = (uint8_t)c; o[1] = (uint8_t)(c >> 8);
    } else if (fmt == FMT_RGBA && siz == SIZ_32) {
        memcpy(o, src + (sy * stride + sx) * 4, 4);
    } else if (fmt == FMT_IA && siz == SIZ_16) {
        unsigned c = be16(src + (sy * stride + sx) * 2);
        o[0] = (uint8_t)c; o[1] = (uint8_t)(c >> 8);
    } else if (fmt == FMT_IA && siz == SIZ_8) {
        unsigned c = src[sy * stride + sx];
        o[0] = (uint8_t)((c & 15) * 17); o[1] = (uint8_t)((c >> 4) * 17);
    } else if (fmt == FMT_IA && siz == SIZ_4) {
        unsigned b = src[(sy * stride + sx) >> 1], nb = (sx & 1) ? (b & 15) : (b >> 4);
        o[0] = (uint8_t)((nb & 1) ? 255 : 0); o[1] = (uint8_t)((nb >> 1) * 36);
    } else if (fmt == FMT_I && siz == SIZ_8) {
        o[0] = o[1] = src[sy * stride + sx];
    } else if (fmt == FMT_I && siz == SIZ_4) {
        unsigned b = src[(sy * stride + sx) >> 1], nb = (sx & 1) ? (b & 15) : (b >> 4);
        o[0] = o[1] = (uint8_t)(nb * 17);
    } else { o[0] = o[1] = 0xff; }
}

/* Bake one request into out (size pw * ph * bpp, the caller gives 512 KB). Returns bytes, 0 when the build size limit stops it (as expand()). Sets pw, ph, gf. */
unsigned tb_bake(const TbReq* q, const uint8_t* tl, const uint8_t* src, uint8_t* out, unsigned* ppw, unsigned* pph, int* pgf) {
    int w = q->w, h = q->h, mw = q->mw, mh = q->mh;
    unsigned pw = pot8(w), ph = pot8(h);
    int gf = tb_gf(q->fmt, q->siz);
    unsigned bpp = gf == TB_GF_RGBA8 ? 4u : 2u;
    if (pw * ph * bpp > 256u * 256u * 4u) return 0;
    for (unsigned y = 0; y < ph; y++) {
        for (unsigned x = 0; x < pw; x++) {
            int sx, sy; uint8_t t[4];
            if ((int)x < w && (int)y < h) { sx = mapc((int)x, mw, q->ms); sy = mapc((int)y, mh, q->mt); }
            else { /* pad: clamp to the last texel, apply the mask, then read the window texel (which applies the mask once more) */
                sy = (int)y < h ? (int)y : h - 1; if (mh) sy = mapc((int)y, mh, q->mt);
                sx = (int)x < w ? (int)x : w - 1; if (mw) sx = mapc((int)x, mw, q->ms);
                sx = mapc(sx, mw, q->ms); sy = mapc(sy, mh, q->mt);
            }
            dec(q, tl, src, sx, sy, t);
            unsigned ty = ph - 1 - y; /* PICA samples t = 0 from the last row */
            unsigned mx = x & 7u, my = ty & 7u;
            unsigned mor = (mx & 1u) | ((my & 1u) << 1) | ((mx & 2u) << 1) | ((my & 2u) << 2) | ((mx & 4u) << 2) | ((my & 4u) << 3);
            unsigned tile = (ty >> 3) * (pw >> 3) + (x >> 3);
            unsigned d0 = (tile * 64u + mor) * 2u;
            if (bpp == 2u) { out[d0] = t[0]; out[d0 + 1] = t[1]; }
            else { d0 = (d0 / 2u) * 4u; out[d0] = t[3]; out[d0 + 1] = t[2]; out[d0 + 2] = t[1]; out[d0 + 3] = t[0]; }
        }
    }
    *ppw = pw; *pph = ph; *pgf = gf;
    return pw * ph * bpp;
}

static uint8_t* slurp(const char* p, long* n) {
    FILE* f = fopen(p, "rb"); if (!f) { perror(p); exit(1); }
    fseek(f, 0, SEEK_END); *n = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t* b = malloc(*n + 1); if (fread(b, 1, *n, f) != (size_t)*n) exit(1); fclose(f); return b;
}

/* walk a request file: calls f for each request */
typedef void (*reqfn)(int i, const TbReq* q, const uint8_t* tl, const uint8_t* src, void* u);
int tb_walk(const uint8_t* b, long n, reqfn f, void* u) {
    uint32_t m, cnt; memcpy(&m, b, 4); memcpy(&cnt, b + 4, 4);
    if (m != TB_REQ_MAGIC) { fprintf(stderr, "bad request file\n"); exit(1); }
    long o = 8;
    for (uint32_t i = 0; i < cnt && o < n; i++) { /* count 0xffffffff: read to the end of the file (texdump writes it) */
        TbReq q; memcpy(&q, b + o, sizeof q); o += sizeof q;
        const uint8_t* tl = b + o; o += q.tlut_n * 2;
        const uint8_t* s = b + o; o += q.src_len;
        if (o > n) { fprintf(stderr, "short request file\n"); exit(1); }
        f((int)i, &q, tl, s, u);
    }
    return cnt == 0xffffffffu ? -1 : (int)cnt;
}

#ifndef TEXBAKE_LIB
static int cmprec(const void* a, const void* b) { uint64_t x = ((const TbRec*)a)->key, y = ((const TbRec*)b)->key; return x < y ? -1 : x > y; }

typedef struct { TbRec* r; uint8_t* data; unsigned n, dn, cap, skipped, dups; unsigned long long raw; unsigned hist[4]; unsigned maxb; } Bk;
static void on_req(int i, const TbReq* q, const uint8_t* tl, const uint8_t* src, void* u) {
    Bk* k = u; static uint8_t buf[512 * 1024]; unsigned pw, ph; int gf;
    (void)i;
    unsigned b = tb_bake(q, tl, src, buf, &pw, &ph, &gf);
    if (!b) { k->skipped++; return; }
    uint64_t key = tb_key(q, tl, src);
    for (unsigned j = 0; j < k->n; j++) if (k->r[j].key == key) { k->dups++; return; }
    if (k->n == k->cap) { k->cap = k->cap ? k->cap * 2 : 1024; k->r = realloc(k->r, k->cap * sizeof(TbRec)); }
    k->data = realloc(k->data, k->dn + ((b + 15u) & ~15u));
    memset(k->data + k->dn, 0, (b + 15u) & ~15u);
    memcpy(k->data + k->dn, buf, b);
    TbRec* r = &k->r[k->n++];
    memset(r, 0, sizeof *r); r->key = key; r->off = k->dn; r->bytes = b; r->pw = (uint16_t)pw; r->ph = (uint16_t)ph; r->gf = (uint8_t)gf;
    k->dn += (b + 15u) & ~15u;
    k->raw += q->src_len + q->tlut_n * 2u;
    k->hist[gf]++; if (b > k->maxb) k->maxb = b;
}

int main(int argc, char** argv) {
    if (argc < 4) { fprintf(stderr, "usage: texbake bake <req.bin> <out.pak>\n"); return 2; }
    if (!strcmp(argv[1], "bake")) {
        long n; uint8_t* b = slurp(argv[2], &n); Bk k; memset(&k, 0, sizeof k);
        int cnt = tb_walk(b, n, on_req, &k);
        qsort(k.r, k.n, sizeof(TbRec), cmprec);
        unsigned base = (16u + k.n * 24u + 15u) & ~15u;
        for (unsigned i = 0; i < k.n; i++) k.r[i].off += base;
        TbHead h = { TB_MAGIC, TB_VERSION, k.n, base };
        FILE* f = fopen(argv[3], "wb"); fwrite(&h, sizeof h, 1, f); fwrite(k.r, sizeof(TbRec), k.n, f);
        for (unsigned i = 16u + k.n * 24u; i < base; i++) fputc(0, f);
        fwrite(k.data, 1, k.dn, f); fclose(f);
        printf("requests %d baked %u skipped (over the build size) %u duplicates %u\n", cnt, k.n, k.skipped, k.dups);
        printf("pack %u bytes (%.2f MB), source bytes %llu, ratio %.2f\n", base + k.dn, (base + k.dn) / 1048576.0, k.raw, k.raw ? (double)(base + k.dn) / k.raw : 0.0);
        printf("RAM for the index: %u bytes (24 per texture). Largest texture %u bytes. By format: RGBA8 %u, RGBA5551 %u, LA8 %u\n", k.n * 24u, k.maxb, k.hist[0], k.hist[1], k.hist[2]);
        return 0;
    }
    return 2;
}
#endif
