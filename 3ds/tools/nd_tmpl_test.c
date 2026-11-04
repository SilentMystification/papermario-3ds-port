/* nd_tmpl_test.c: host test of the S1-3 template (plan_native_slices S1-3, gate G1).
 * ORACLE: the ndref v2 files hold, for every GPU draw call of the old path, the citro3d context words that C3Di_UpdateContext turns into register writes
 * (kind 4 records: effect block ctx[216..268), texenv block ctx[292..428), see 3ds/tests/ndref/FORMAT.md). For every state record the test builds the key from
 * the draw record and the state record, builds the template, and requires every template word to equal the register write the oracle words produce.
 * Not compared: a stage colour word that a colour source sets (PRIM, ENV, plan constants: per-draw uniforms), the buffer colour, the stencil op while the
 * stencil test is off (stale in the old path), the stencil test word beyond its enable bit while off.
 * Render rules: nd_render_state(omode) equals the held flags (zcmp, zupd, blend, atest, decal, nocol, memblend).
 * Usage: nd_tmpl_test <ndref.bin> ... */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"

/* host copy of libctru f32tof24 (float to the PICA 24 bit float). The oracle words check it. */
uint32_t f32tof24(float vf) {
    union { float f; uint32_t i; } v;
    v.f = vf;
    uint32_t sign = v.i >> 31, mant = (v.i & 0x7FFFFFu) >> 7;
    int exp = (int)((v.i >> 23) & 0xFF);
    if (exp == 0) return sign << 23;
    exp = exp - 127 + 63;
    if (exp < 0) return sign << 23;
    if (exp > 0x7F) return sign << 23 | 0x7Fu << 16;
    return mant | (uint32_t)exp << 16 | sign << 23;
}

static uint32_t rd32(const unsigned char* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

static int fails;
static const char* cur_file;
static unsigned cur_state;
#define CHECK(c, ...) do { if (!(c)) { fails++; if (fails < 40) { printf("FAIL %s state %u: ", cur_file, cur_state); printf(__VA_ARGS__); printf("\n"); } } } while (0)

static int word_of(const NdTmplWord* w, unsigned n, unsigned reg, uint32_t* val) {
    for (unsigned i = 0; i < n; i++) if (w[i].reg == reg) { *val = w[i].val; return 1; }
    return 0;
}

static unsigned n_state, n_pass[4], n_c8, n_stencil, n_tmpl_max, n_rs_ok, n_rs_skip;

/* one draw record (kind 1) body: TevKey 19 x u32 at 0, NdrefDraw at 76 */
typedef struct { uint32_t tk[19]; uint32_t z, graw, prog, cull, decal, blend, atest; } DrawInfo;

static void check_state(const DrawInfo* d, const unsigned char* s) {
    const uint32_t* h = (const uint32_t*)0; (void)h;
    uint32_t pass = rd32(s), hz = rd32(s + 4), zmark = rd32(s + 8), zcmp = rd32(s + 12), zupd = rd32(s + 16), nocol = rd32(s + 20), noise = rd32(s + 24), raw = rd32(s + 28);
    uint32_t w0 = rd32(s + 32), w1 = rd32(s + 36), two = rd32(s + 40), t1only = rd32(s + 44);
    uint32_t eff[13], tev[34];
    uint32_t om_h = rd32(s + 48), om_l = rd32(s + 52), om_d = rd32(s + 56);
    for (int i = 0; i < 13; i++) eff[i] = rd32(s + 60 + 4 * i);
    for (int i = 0; i < 34; i++) tev[i] = rd32(s + 60 + 52 + 4 * i);
    assert(pass < 4);
    NdTmplKey k;
    memset(&k, 0, sizeof k);
    k.pass = (uint8_t)pass; k.raw = (uint8_t)raw;
    k.cull = raw ? (uint8_t)d->cull : 0;
    k.z = hz != 0; k.zmark = zmark != 0; k.zcmp = zcmp != 0; k.zupd = zupd != 0; k.nocol = nocol != 0; k.decal = d->decal != 0;
    k.dbias = 0x3A83126Fu; /* 0.001f, the decal_bias default */
    k.atest_on = (int32_t)d->atest >= 0; k.aref = k.atest_on ? (uint8_t)d->atest : 0;
    k.combine = (uint8_t)d->tk[0]; k.tex = (uint8_t)d->tk[1]; k.tex1 = (uint8_t)d->tk[2]; k.i8_0 = (uint8_t)d->tk[3]; k.i8_1 = (uint8_t)d->tk[4];
    k.rgb_scale = (uint8_t)d->tk[5]; k.r_mix = (uint8_t)d->tk[6]; k.draw_3d = (uint8_t)d->tk[7]; k.rect_2d = (uint8_t)d->tk[8]; k.dview = d->tk[9] != 0;
    k.off_active = (uint8_t)d->tk[10]; k.off_i8 = (uint8_t)d->tk[11]; k.r_memblend = (uint8_t)d->tk[12]; k.r_replace = (uint8_t)d->tk[13]; k.r_blend = (uint8_t)d->tk[14];
    k.noise = noise != 0; k.two = (uint8_t)two; k.t1only = (uint8_t)t1only; k.w0 = w0; k.w1 = w1;
    n_pass[pass]++;
    /* render state rules: the flags the old path held equal the rules applied to the recorded other mode words. A draw inside a temporary state override
     * of pm_gpu.c (rect fill, depth write helpers) is counted and not compared. */
    if (om_h | om_l | om_d) {
        NdRender rs;
        nd_render_state(om_h, om_l, (int)om_d, &rs);
        int same = rs.zcmp == (zcmp != 0) && rs.zupd == (zupd != 0) && rs.blend == (d->tk[14] != 0) && rs.decal == (d->decal != 0) && rs.nocol == (nocol != 0) &&
                   (int)rs.atest == (int)d->atest && (!rs.set_memb || rs.memb == (d->tk[12] != 0));
        if (same) n_rs_ok++;
        else { n_rs_skip++; if (!k.rect_2d && !k.off_active) CHECK(0, "render rules: omode %08x %08x depth %u rules zcmp %u zupd %u blend %u decal %u nocol %u atest %d memb %u, held zcmp %u zupd %u blend %u decal %u nocol %u atest %d memb %u",
            om_h, om_l, om_d, rs.zcmp, rs.zupd, rs.blend, rs.decal, rs.nocol, rs.atest, rs.memb, zcmp, zupd, d->tk[14], d->decal, nocol, (int)d->atest, d->tk[12]); }
    }
    if (k.combine == 8) n_c8++;
    int id = nd_tmpl_build(&k);
    CHECK(id >= 0, "template build failed (combine %u w0 %08x w1 %08x two %u t1 %u)", k.combine, w0, w1, two, t1only);
    if (id < 0) return;
    CHECK(nd_tmpl_build(&k) == id, "same key gives another id");
    if ((unsigned)id > n_tmpl_max) n_tmpl_max = (unsigned)id;
    const NdTmplWord* w;
    unsigned nw = nd_tmpl_words(id, &w);
    uint32_t v;
    /* effect block: C3Di_EffectBind register map */
    CHECK(word_of(w, nw, 0x100, &v) && v == eff[0], "reg 100 template %08x oracle %08x", v, eff[0]);
    CHECK(word_of(w, nw, 0x130, &v) && v == eff[1], "reg 130 template %08x oracle %08x", v, eff[1]);
    CHECK(word_of(w, nw, 0x04D, &v) && v == eff[2], "reg 04D template %08x oracle %08x", v, eff[2]);
    CHECK(word_of(w, nw, 0x04E, &v) && v == eff[3], "reg 04E (pass %u decal %u) template %08x oracle %08x", pass, k.decal, v, eff[3]);
    CHECK(word_of(w, nw, 0x040, &v) && v == (eff[4] & 3u), "cull template %08x oracle %08x", v, eff[4] & 3u);
    CHECK(word_of(w, nw, 0x06D, &v) && v == ((eff[4] >> 8) & 0xFFu), "reg 06D template %08x oracle %08x", v, (eff[4] >> 8) & 0xFFu);
    CHECK(word_of(w, nw, 0x062, &v) && v == ((eff[4] >> 16) & 0xFFu), "reg 062 template %08x oracle %08x", v, (eff[4] >> 16) & 0xFFu);
    CHECK(word_of(w, nw, 0x118, &v) && v == ((eff[4] >> 16) & 0xFFu), "reg 118 template %08x oracle %08x", v, (eff[4] >> 16) & 0xFFu);
    CHECK(word_of(w, nw, 0x061, &v) && v == (eff[4] >> 24), "reg 061 template %08x oracle %08x", v, eff[4] >> 24);
    CHECK(word_of(w, nw, 0x06A, &v) && v == eff[5], "reg 06A template %08x oracle %08x", v, eff[5]);
    CHECK(word_of(w, nw, 0x104, &v) && v == eff[6], "alpha reg 104 template %08x oracle %08x (atest %d)", v, eff[6], (int)d->atest);
    uint32_t sten_en = eff[7] & 1u;
    CHECK(word_of(w, nw, 0x105, &v) && (v & 1u) == sten_en, "stencil enable template %08x oracle %08x (pass %u)", v, eff[7], pass);
    if (sten_en) {
        n_stencil++;
        CHECK(v == eff[7], "stencil test reg 105 template %08x oracle %08x (pass %u)", v, eff[7], pass);
        CHECK(word_of(w, nw, 0x106, &v) && v == eff[8], "stencil op reg 106 template %08x oracle %08x (pass %u)", v, eff[8], pass);
    }
    CHECK(word_of(w, nw, 0x107, &v) && v == eff[9], "depth reg 107 template %08x oracle %08x (pass %u z %u zmark %u zcmp %u zupd %u nocol %u decal %u)", v, eff[9], pass, hz, zmark, zcmp, zupd, nocol, k.decal);
    CHECK(word_of(w, nw, 0x126, &v) && v == (((0xAF02u >> ((eff[9] >> 3) & 14u)) << 24) & 0x3000000u), "reg 126 template %08x", v);
    CHECK(word_of(w, nw, 0x103, &v) && v == eff[10], "reg 103 template %08x oracle %08x", v, eff[10]);
    CHECK(word_of(w, nw, 0x101, &v) && v == eff[11], "blend reg 101 template %08x oracle %08x (off %u r_blend %u replace %u i8 %u)", v, eff[11], k.off_active, k.r_blend, k.r_replace, k.off_i8);
    CHECK(word_of(w, nw, 0x102, &v) && v == (eff[12] & 0xFFu), "reg 102 template %08x oracle %08x", v, eff[12] & 0xFFu);
    /* TEV */
    for (unsigned i = 0; i < 6; i++) {
        unsigned base = i < 4 ? 0xC0u + 8u * i : 0xF0u + 8u * (i - 4u);
        for (unsigned j = 0; j < 5; j++) {
            uint32_t t = 0;
            int have = word_of(w, nw, base + j, &t);
            CHECK(have, "reg %03x missing", base + j);
            if (j == 3 && t != 0xFFFFFFFFu) continue; /* a colour source sets this word: per-draw uniform */
            CHECK(t == tev[5 * i + j], "tev stage %u word %u template %08x oracle %08x (combine %u tex %u tex1 %u mix %u off %u i8 %u memb %u)", i, j, t, tev[5 * i + j],
                  k.combine, k.tex, k.tex1, k.r_mix, k.off_active, k.off_i8, k.r_memblend);
        }
    }
    if (k.combine == 8) CHECK(word_of(w, nw, 0x0E0, &v) && ((tev[30] >> 8) & 0xFFu) == 0, "texenv buffer update word %08x", tev[30]);
    n_state++;
}

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: nd_tmpl_test <ndref.bin> ...\n"); return 2; }
    unsigned n_files = 0;
    for (int a = 1; a < argc; a++) {
        FILE* f = fopen(argv[a], "rb");
        if (!f) { printf("FAIL: cannot open %s\n", argv[a]); return 1; }
        fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
        unsigned char* buf = malloc((size_t)sz);
        assert(buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz);
        fclose(f);
        assert(memcmp(buf, "NDR1", 4) == 0 && rd32(buf + 4) == 3);
        uint32_t cnt = rd32(buf + 12);
        size_t pos = 20;
        DrawInfo d;
        int have_draw = 0;
        unsigned seen = 0;
        cur_file = argv[a];
        cur_state = 0;
        for (size_t end = 20 + rd32(buf + 16); pos < end;) {
            uint32_t kind = rd32(buf + pos), size = rd32(buf + pos + 4);
            const unsigned char* b = buf + pos + 8;
            if (kind == 1) {
                for (int i = 0; i < 19; i++) d.tk[i] = rd32(b + 4 * i);
                d.z = rd32(b + 76); d.graw = rd32(b + 80); d.prog = rd32(b + 84); d.cull = rd32(b + 88); d.decal = rd32(b + 92); d.blend = rd32(b + 96); d.atest = rd32(b + 100);
                have_draw = 1; seen++;
            } else if (kind == 4) {
                assert(have_draw && size == 248);
                check_state(&d, b);
                cur_state++;
            }
            pos += 8 + size;
        }
        assert(seen == cnt && cur_state >= seen);
        n_files++;
        free(buf);
    }
    printf("nd_tmpl files %u states %u (pass 0:%u 1:%u 2:%u 3:%u) combine8 %u stencil-on %u templates %u render rules equal %u override %u\n", n_files, n_state, n_pass[0], n_pass[1], n_pass[2], n_pass[3], n_c8, n_stencil, n_tmpl_max + 1, n_rs_ok, n_rs_skip);
    if (fails == 0) puts("nd_tmpl state and TEV words: PASS");
    else printf("nd_tmpl state and TEV words: FAIL (%d checks)\n", fails);
    return fails == 0 ? 0 : 1;
}
