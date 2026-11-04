/* nd_seam_test.c: seam host test (plan_native_system 9.5, gate G9). Step 1: read the N64 shapes of mac_01 and mac_02
 * (big-endian, loaded at SHAPE_BASE 0x80210000, pm_map.c:17), walk the file for G_VTX loads (F3DEX: word0 top byte 0x01,
 * n = ((w0 >> 12) & 0xFF) + 1, word1 = N64 address), read the Vtx records (16 bytes, s16 x y z at 0..5) and count the
 * distinct positions that both models have (shared edge candidates). Data words can match G_VTX, so the count is a candidate list.
 * Step 2 (S1-4): ndref kind 5 records hold the inputs (post, game matrix) of every palette product the old path made, and the kind 1 palette holds its
 * result (libcitro3d Mtx_Multiply). nd_mtx_pcomp must reproduce each result bit for bit. Then the shared positions of step 1 go through the fp24 dp4 with
 * the old matrix and the native matrix: the two must be equal, so no crack opens on a shared edge.
 * Usage: nd_seam_test <mac_01_shape.bin> <mac_02_shape.bin> <ndref.bin> ... */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "nd.h"

/* pm_interp.c stubs (nd_mtx.c links them): no replacement table */
int pm_interp_repl;
const float (*pm_interp_find(const void* m))[4] { (void)m; return 0; }

#define SHAPE_BASE 0x80210000u
#define G_VTX 0x01u
#define POS_MAX 65536u

typedef struct { int16_t x, y, z; } Pos;
typedef struct { Pos* p; unsigned n; uint8_t* raw; size_t len; } Shape;

static uint32_t be32(const uint8_t* p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }
static int16_t be16(const uint8_t* p) { return (int16_t)(p[0] << 8 | p[1]); }
static uint32_t le32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

/* fp24: sign, 7 bit exponent, 16 bit mantissa. Round to nearest even on the float32 mantissa (drop 7 bits). Same as ndt_host_test.c fp24. */
static float fp24(float x) {
    union { float f; uint32_t u; } v; uint32_t lsb;
    v.f = x;
    if ((v.u & 0x7fffffffu) == 0) return x;
    lsb = (v.u >> 7) & 1u;
    v.u += 0x3fu + lsb;
    v.u &= ~0x7fu;
    return v.f;
}
static float dp4_fp24(const float row[4], const float v[4]) {
    float acc = 0.f;
    for (int k = 0; k < 4; k++) acc = fp24(acc + fp24(fp24(row[k]) * fp24(v[k])));
    return acc;
}

static int load(const char* path, Shape* s) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END); s->len = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    s->raw = malloc(s->len);
    int ok = s->raw && fread(s->raw, 1, s->len, f) == s->len;
    fclose(f);
    s->p = malloc(POS_MAX * sizeof(Pos));
    return ok && s->p;
}

/* Every loaded record is kept. Vertex slots are reused by later display lists, so a slot is not a vertex. */
static void walk(Shape* s) {
    s->n = 0;
    for (size_t w = 0; w + 8 <= s->len; w += 8) {
        uint32_t w0 = be32(s->raw + w), w1 = be32(s->raw + w + 4);
        if ((w0 >> 24) != G_VTX) continue;
        unsigned n = ((w0 >> 12) & 0xFFu) + 1u;
        if (w1 < SHAPE_BASE) continue;
        size_t off = (size_t)(w1 - SHAPE_BASE);
        if (off + 16u * n > s->len) continue;
        for (unsigned i = 0; i < n && s->n < POS_MAX; i++) {
            const uint8_t* v = s->raw + off + 16u * i;
            s->p[s->n].x = be16(v); s->p[s->n].y = be16(v + 2); s->p[s->n].z = be16(v + 4);
            s->n++;
        }
    }
}

static int cmp_pos(const void* a, const void* b) {
    const Pos* x = a; const Pos* y = b;
    if (x->x != y->x) return x->x < y->x ? -1 : 1;
    if (x->y != y->y) return x->y < y->y ? -1 : 1;
    if (x->z != y->z) return x->z < y->z ? -1 : 1;
    return 0;
}

/* distinct positions of a model, in place. Returns the new count. */
static unsigned uniq(Pos* p, unsigned n) {
    if (!n) return 0;
    qsort(p, n, sizeof(Pos), cmp_pos);
    unsigned k = 1;
    for (unsigned i = 1; i < n; i++) if (cmp_pos(&p[i], &p[k - 1]) != 0) p[k++] = p[i];
    return k;
}

int main(int argc, char** argv) {
    if (argc < 4) { fprintf(stderr, "usage: nd_seam_test <mac_01_shape.bin> <mac_02_shape.bin> <ndref.bin> ...\n"); return 2; }
    static Shape a, b;
    static Pos shared_pos[POS_MAX];
    if (!load(argv[1], &a) || !load(argv[2], &b)) { fprintf(stderr, "cannot read the shapes\n"); return 2; }
    walk(&a); walk(&b);
    unsigned na = uniq(a.p, a.n), nb = uniq(b.p, b.n), shared = 0;
    for (unsigned i = 0, j = 0; i < na && j < nb;) {
        int c = cmp_pos(&a.p[i], &b.p[j]);
        if (c == 0) { shared_pos[shared++] = a.p[i]; i++; j++; } else if (c < 0) i++; else j++;
    }
    printf("seam step 1: mac_01 %u loaded, %u distinct; mac_02 %u loaded, %u distinct; shared positions %u\n", a.n, na, b.n, nb, shared);

    unsigned prod = 0, prod_bad = 0, dp_checked = 0, dp_bad = 0, files = 0;
    for (int f = 3; f < argc; f++) {
        FILE* fp = fopen(argv[f], "rb");
        if (!fp) { printf("FAIL: cannot open %s\n", argv[f]); return 1; }
        fseek(fp, 0, SEEK_END); long sz = ftell(fp); fseek(fp, 0, SEEK_SET);
        uint8_t* buf = malloc((size_t)sz);
        if (!buf || fread(buf, 1, (size_t)sz, fp) != (size_t)sz) { printf("FAIL: cannot read %s\n", argv[f]); return 1; }
        fclose(fp);
        if (memcmp(buf, "NDR1", 4) != 0 || le32(buf + 4) != 3) { printf("FAIL: %s is not ndref version 3\n", argv[f]); return 1; }
        files++;
        const uint8_t* pal = NULL; unsigned np1 = 0;
        for (size_t pos = 20, end = 20 + le32(buf + 16); pos < end;) {
            uint32_t kind = le32(buf + pos), size = le32(buf + pos + 4);
            const uint8_t* body = buf + pos + 8;
            if (kind == 1) { np1 = le32(body + 520); pal = body + 524; }
            if (kind == 5) {
                float post[16];
                memcpy(post, body, 64);
                unsigned np = le32(body + 64);
                if (np != np1 || !pal) { printf("FAIL: %s: kind 5 palette count %u, kind 1 count %u\n", argv[f], np, np1); return 1; }
                for (unsigned k = 0; k < np; k++) {
                    const uint8_t* g = body + 68 + 68 * k;
                    const uint8_t* old = pal + 68 * k; /* u32 gm, then pcomp */
                    float game[4][4], o[16], oldm[16];
                    if (le32(g) != le32(old)) { printf("FAIL: %s: game matrix index differs\n", argv[f]); return 1; }
                    memcpy(game, g + 4, 64);
                    memcpy(oldm, old + 4, 64);
                    nd_mtx_pcomp(o, post, game);
                    prod++;
                    if (memcmp(o, oldm, 64) != 0) { prod_bad++; if (prod_bad < 6) printf("FAIL: %s: pcomp of game matrix %u differs from the old path\n", argv[f], le32(g)); }
                    else if ((prod & 7u) == 0) /* every 8th matrix: the shared positions through both matrices */
                        for (unsigned q = 0; q < shared; q++) {
                            float v[4] = { 1.f, (float)shared_pos[q].z, (float)shared_pos[q].y, (float)shared_pos[q].x };
                            for (int r = 0; r < 4; r++) { dp_checked++; if (dp4_fp24(o + 4 * r, v) != dp4_fp24(oldm + 4 * r, v)) dp_bad++; }
                        }
                }
            }
            pos += 8 + size;
        }
        free(buf);
    }
    printf("seam step 2: %u files, %u palette products, %u differ from the old path; %u shared position dp4 checks, %u differ\n", files, prod, prod_bad, dp_checked, dp_bad);
    if (prod == 0 || prod_bad || dp_bad || !dp_checked) { puts("seam step 2: FAIL"); return 1; }
    puts("seam step 2: PASS");
    return 0;
}
