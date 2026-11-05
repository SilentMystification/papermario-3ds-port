/* nd_tex_test.c: host test of nd_tex.c (plan_native_slices S1-6, gate G1).
 * 1. wrap rules: known answers for every (cm, mask) pair of the RDP tile wrap and for the final wrap of a mask wide texture.
 * 2. ndref words: every unit 0 and unit 1 block of the 8 ndref files (dim, param) is rebuilt from the fields it holds with nd_tex_dim and nd_tex_param and must be equal.
 *    The other bits of the param word (the texture mode) are kept as they are in the old words. The data address word is not derived (it is the VRAM or staging address).
 * 3. descriptor table: the same descriptor gives the same block id with one load, another wrap gives another id, the words are the slot words with the bind's wrap and filter.
 * Usage: nd_tex_test <ndref.bin> ... */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; if (fails < 30) { printf("FAIL: " __VA_ARGS__); printf("\n"); } } } while (0)

/* pm_tex.c stubs: a load binds slot 7 whose words hold the size and an address, and the unit wrap and filter come from the test */
static unsigned loads, last_w, last_h, last_cms;
static uint32_t stub_ws = 2, stub_wt = 2;
static unsigned stub_filter = 1;
void pm_tex_set_wrap(unsigned cms, unsigned cmt, unsigned masks, unsigned maskt) { last_cms = cms; (void)cmt; (void)masks; (void)maskt; }
int pm_tex_load(const void* img, unsigned fmt, unsigned siz, int width, int height, const void* tlut, int tlut_n, int stride) {
    (void)img; (void)fmt; (void)siz; (void)tlut; (void)tlut_n; (void)stride;
    loads++; last_w = (unsigned)width; last_h = (unsigned)height;
    return 1;
}
void pm_tex_cur_slots(int* a, int* b) { *a = 7; *b = -1; }
int pm_tex_slot_words(int i, uint32_t* out) {
    assert(i == 7);
    out[0] = (last_w << 16) | last_h; out[1] = 0x00000000u; out[2] = 0; out[3] = 0x20000000u + loads;
    return 1;
}
void pm_tex_unit0_param(uint32_t* ws, uint32_t* wt, unsigned* filter) { *ws = stub_ws; *wt = stub_wt; *filter = stub_filter; }

static uint32_t rd32(const unsigned char* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

static void test_wrap(void) {
    for (unsigned cm = 0; cm < 4; cm++) {
        CHECK(nd_tex_wrap_of(cm, 0) == 0, "mask 0 is clamp (cm %u)", cm);
        uint32_t want = (cm & 2) ? 0u : (cm & 1) ? 3u : 2u;
        CHECK(nd_tex_wrap_of(cm, 5) == want, "cm %u mask 5: %u want %u", cm, nd_tex_wrap_of(cm, 5), want);
    }
    CHECK(nd_tex_wrap_final(0, 1, 2) == 2 && nd_tex_wrap_final(0, 0, 0) == 0, "a full width texture keeps the tile wrap");
    CHECK(nd_tex_wrap_final(1, 1, 0) == 3 && nd_tex_wrap_final(1, 0, 0) == 2, "a mask wide texture repeats or mirrors");
}

static unsigned n_units, n_param_other;
static void check_unit(const unsigned char* t, const char* file) {
    uint32_t dim = rd32(t), param = rd32(t + 4);
    if (dim == 0 && param == 0) return;
    n_units++;
    CHECK(nd_tex_dim(dim >> 16, dim & 0xFFFFu) == dim, "%s: dim %08x", file, dim);
    unsigned filter = (param >> 1) & 1u;
    CHECK(((param >> 2) & 1u) == filter, "%s: mag and min filter differ in param %08x", file, param);
    uint32_t base = param & ~((1u << 1) | (1u << 2) | (3u << 12) | (3u << 8));
    CHECK(nd_tex_param(base, filter, (param >> 12) & 3u, (param >> 8) & 3u) == param, "%s: param %08x not rebuilt", file, param);
    if (base != 0) n_param_other++; /* bits other than filter and wrap are set */
}

static void test_ndref(int argc, char** argv) {
    for (int a = 1; a < argc; a++) {
        FILE* f = fopen(argv[a], "rb");
        if (!f) { CHECK(0, "cannot open %s", argv[a]); continue; }
        fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
        unsigned char* buf = malloc((size_t)sz);
        assert(buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz);
        fclose(f);
        assert(memcmp(buf, "NDR1", 4) == 0 && rd32(buf + 4) == 4);
        for (size_t pos = 20, end = 20 + rd32(buf + 16); pos < end;) {
            uint32_t kind = rd32(buf + pos), size = rd32(buf + pos + 4);
            if (kind == 1) { check_unit(buf + pos + 8 + 128, argv[a]); check_unit(buf + pos + 8 + 144, argv[a]); }
            pos += 8 + size;
        }
        free(buf);
    }
}

static void test_table(void) {
    static unsigned char img_a[16], img_b[16];
    NdTexDesc d;
    memset(&d, 0, sizeof d);
    d.img = img_a; d.fmt = 0; d.siz = 2; d.w = 32; d.h = 16; d.cms = 0; d.cmt = 0; d.masks = 5; d.maskt = 4; d.stride = 32;
    nd_tex_reset();
    int a = nd_tex_get(&d);
    int a2 = nd_tex_get(&d);
    CHECK(a >= 0 && a2 == a && loads == 1, "same descriptor: ids %d %d, loads %u", a, a2, loads);
    CHECK(last_w == 32 && last_h == 16 && last_cms == 0, "the load got the descriptor size");
    const uint32_t* w = nd_tex_block(a);
    CHECK(w[0] == (32u << 16 | 16u) && w[1] == ((1u << 1) | (1u << 2) | (2u << 12) | (2u << 8)) && w[3] == 0x20000001u, "block words %08x %08x %08x %08x", w[0], w[1], w[2], w[3]);
    stub_ws = 0; stub_wt = 3; stub_filter = 0;
    d.cms = 2;
    int b = nd_tex_get(&d);
    CHECK(b >= 0 && b != a && loads == 2, "another wrap flag gives another block");
    w = nd_tex_block(b);
    CHECK(w[1] == ((0u << 12) | (3u << 8)), "param with clamp s, mirror t, nearest: %08x", w[1]);
    d.img = img_b;
    int c = nd_tex_get(&d);
    CHECK(c >= 0 && c != a && c != b, "another raster gives another block");
    CHECK(nd_tex_get(&d) == c && loads == 3, "the third descriptor loads once");
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: nd_tex_test <ndref.bin> ...\n"); return 2; }
    test_wrap();
    test_ndref(argc, argv);
    test_table();
    printf("nd_tex units %u (param with other bits %u)\n", n_units, n_param_other);
    CHECK(n_units > 100, "too few texture units checked: %u", n_units);
    puts(fails ? "nd_tex: FAIL" : "nd_tex: PASS");
    return fails != 0;
}
