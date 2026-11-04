/* nd_seam_test.c: seam host test (plan_native_system 9.5, gate G9). Step 1: read the N64 shapes of mac_01 and mac_02
 * (big-endian, loaded at SHAPE_BASE 0x80210000, pm_map.c:17), walk the file for G_VTX loads (F3DEX: word0 top byte 0x01,
 * n = ((w0 >> 12) & 0xFF) + 1, word1 = N64 address), read the Vtx records (16 bytes, s16 x y z at 0..5) and count the
 * distinct positions that both models have (shared edge candidates). Data words can match G_VTX, so the count is a candidate list.
 * Step 2 (needs nd_mtx.h, row S1-4): the same pairs through the old matrix path and nd_mtx with fp24 dp4 must be bit equal.
 * Until nd_mtx.h exists, step 2 reports "FAIL nd_mtx missing". Usage: nd_seam_test <mac_01_shape.bin> <mac_02_shape.bin> */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SHAPE_BASE 0x80210000u
#define G_VTX 0x01u
#define POS_MAX 65536u

typedef struct { int16_t x, y, z; } Pos;
typedef struct { Pos* p; unsigned n; uint8_t* raw; size_t len; } Shape;

static uint32_t be32(const uint8_t* p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }
static int16_t be16(const uint8_t* p) { return (int16_t)(p[0] << 8 | p[1]); }

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
    if (argc != 3) { fprintf(stderr, "usage: nd_seam_test <mac_01_shape.bin> <mac_02_shape.bin>\n"); return 2; }
    static Shape a, b;
    if (!load(argv[1], &a) || !load(argv[2], &b)) { fprintf(stderr, "cannot read the shapes\n"); return 2; }
    walk(&a); walk(&b);
    unsigned na = uniq(a.p, a.n), nb = uniq(b.p, b.n), shared = 0;
    for (unsigned i = 0, j = 0; i < na && j < nb;) {
        int c = cmp_pos(&a.p[i], &b.p[j]);
        if (c == 0) { shared++; i++; j++; } else if (c < 0) i++; else j++;
    }
    printf("seam step 1: mac_01 %u loaded, %u distinct; mac_02 %u loaded, %u distinct; shared positions %u\n", a.n, na, b.n, nb, shared);
#if __has_include("nd_mtx.h")
    printf("seam step 2: nd_mtx.h present (the comparison is written in S1-4)\n");
    return 1;
#else
    printf("FAIL nd_mtx missing (row S1-4): seam step 2 (fp24 dp4 bit equal) cannot run\n");
    return 1;
#endif
}
