/* nd_tmpl_test.c: host test of the S1-3 render state template (plan_native_slices S1-3, gate G1).
 * For every draw of the ndref files (argv), the template of its key must decode to the ndref cull, decal, alpha test
 * and blend fields. Depth: a draw with z 0 must have the depth test off. Draws with z 1 need the zmark, zcmp and zupd
 * flags, which the ndref does not hold: they are counted and not checked.
 * Usage: nd_tmpl_test <ndref.bin> ... */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"

/* host stand-in for libctru f32tof24: 0.0 maps to 0, any other value to a non-zero word (the bits are not checked) */
uint32_t f32tof24(float f) { return f == 0.0f ? 0u : 0x00ABCDEFu; }

static uint32_t rd32(const unsigned char* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

static int word_of(const NdTmplWord* w, unsigned n, unsigned reg, uint32_t* val) {
    for (unsigned i = 0; i < n; i++) if (w[i].reg == reg) { *val = w[i].val; return 1; }
    return 0;
}

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; if (fails < 20) { printf("FAIL: " __VA_ARGS__); printf("\n"); } } } while (0)

static void check_draw(const unsigned char* d, unsigned* n_draw, unsigned* n_z1, unsigned* n_off) {
    uint32_t off_active = rd32(d + 40);
    const unsigned char* nd = d + 76; /* NdrefDraw */
    int z = (int32_t)rd32(nd), graw = (int32_t)rd32(nd + 4);
    uint32_t cull = rd32(nd + 12);
    int decal = (int32_t)rd32(nd + 16), blend = (int32_t)rd32(nd + 20), atest = (int32_t)rd32(nd + 24);
    assert(atest < 0 || atest <= 255);
    NdTmplKey k;
    memset(&k, 0, sizeof k);
    k.cull = graw ? (uint8_t)cull : 0;
    k.z = z != 0;
    k.zcmp = z != 0; /* arbitrary for z 1: the depth check is skipped for z 1 */
    k.decal = decal != 0;
    k.dbias = 0x3F000000u;
    k.atest_on = atest >= 0;
    k.aref = atest >= 0 ? (uint8_t)atest : 0;
    k.blend = blend != 0;
    int id = nd_tmpl_build(&k);
    CHECK(id >= 0, "template table full");
    CHECK(nd_tmpl_build(&k) == id, "same key gives another id");
    const NdTmplWord* w;
    unsigned nw = nd_tmpl_words(id, &w);
    CHECK(nw == ND_TMPL_WORDS, "word count %u", nw);
    uint32_t v;
    CHECK(word_of(w, nw, 0x040, &v) && v == k.cull, "cull word");
    CHECK(word_of(w, nw, 0x06D, &v) && v == 1, "depth map enable word");
    CHECK(word_of(w, nw, 0x04E, &v) && (v != 0) == (decal != 0), "decal offset word (decal %d)", decal);
    CHECK(word_of(w, nw, 0x104, &v) && (v & 1) == (uint32_t)(atest >= 0), "alpha enable (atest %d)", atest);
    if (atest >= 0) CHECK(((v >> 8) & 0xFF) == (uint32_t)atest && ((v >> 4) & 7) == 6, "alpha ref or func (atest %d)", atest);
    if (!off_active) {
        CHECK(word_of(w, nw, 0x101, &v), "blend word");
        CHECK(((v >> 16) & 0xF) == (blend ? 6u : 1u), "blend src factor (blend %d)", blend);
        CHECK(((v >> 20) & 0xF) == (blend ? 7u : 0u), "blend dst factor (blend %d)", blend);
        (*n_off)++;
    }
    CHECK(word_of(w, nw, 0x107, &v), "depth word");
    if (z == 0) {
        CHECK((v & 1) == 0, "depth test on for z 0");
        CHECK(((v >> 8) & 0x1F) == 0x0F, "depth mask for z 0");
        CHECK(((v >> 4) & 7) == 6, "depth func for z 0");
        (*n_draw)++;
    } else (*n_z1)++;
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: nd_tmpl_test <ndref.bin> ...\n"); return 2; }
    unsigned n_draw = 0, n_z1 = 0, n_off = 0, n_files = 0;
    for (int a = 1; a < argc; a++) {
        FILE* f = fopen(argv[a], "rb");
        if (!f) { printf("FAIL: cannot open %s\n", argv[a]); return 1; }
        fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
        unsigned char* buf = malloc((size_t)sz);
        assert(buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz);
        fclose(f);
        assert(memcmp(buf, "NDR1", 4) == 0 && rd32(buf + 4) == 1);
        uint32_t cnt = rd32(buf + 12);
        size_t pos = 20;
        uint32_t seen = 0;
        for (uint32_t r = 0; r < cnt; r++) {
            uint32_t kind = rd32(buf + pos), size = rd32(buf + pos + 4);
            if (kind == 1) { check_draw(buf + pos + 8, &n_draw, &n_z1, &n_off); seen++; }
            pos += 8 + size;
        }
        assert(seen > 0);
        n_files++;
        free(buf);
    }
    printf("nd_tmpl files %u draws z0 checked %u z1 skipped %u blend checked %u\n", n_files, n_draw, n_z1, n_off);
    if (fails == 0) puts("nd_tmpl render state: PASS");
    else printf("nd_tmpl render state: FAIL (%d checks)\n", fails);
    return fails == 0 ? 0 : 1;
}
