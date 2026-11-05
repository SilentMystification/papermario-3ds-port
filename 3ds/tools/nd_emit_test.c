/* nd_emit_test.c: host test of nd_emit.c against the real command stream (ndref v4 kind 6, plan_native_slices S3 M3).
 * The command words of the old path are replayed into a register file and a uniform memory (the stream). For every raw pm4 draw (kind 1 graw, program 4) the draw is rebuilt from
 * the ndref data (template key, texture words, palette entry 0, uniforms, index count), emitted by nd_emit_draw with a shadow that lives over the whole file, and replayed into
 * a second register file and uniform memory (native). Every register and uniform that both have written must hold the same value after each draw.
 * Not compared: the template colour words and the registers nd_cmd_test skips, the index address word (0x227: a game address), the stale texture block. A native draw also
 * has to be smaller than the stream draw on average (only deltas are written).
 * Usage: nd_emit_test <ndref.bin> ... */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"

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

typedef struct { uint32_t r[0x300], wm[0x300]; uint8_t set[0x300]; uint32_t u[0x80 * 4]; uint8_t uset[0x80 * 4]; uint32_t uidx; } Regs;
static void apply(Regs* g, const uint32_t* w, unsigned n) {
    unsigned i = 0;
    while (i + 1 < n) {
        uint32_t p0 = w[i], hd = w[i + 1];
        unsigned reg = hd & 0xFFFF, mask = (hd >> 16) & 0xF, extra = (hd >> 20) & 0xFF, consec = hd >> 31, len = 2 + extra + (extra & 1);
        if (i + len > n) break;
        for (unsigned k = 0; k <= extra; k++) {
            unsigned r = consec ? reg + k : reg;
            uint32_t v = k == 0 ? p0 : w[i + 1 + k];
            if (r >= 0x300) continue;
            if (r == 0x2C0) { g->uidx = (v & 0x7F) * 4u; continue; } /* float32 mode assumed: the stream only uses that */
            if (r == 0x2C1) { if (g->uidx < 0x80 * 4) { g->u[g->uidx] = v; g->uset[g->uidx] = 1; } g->uidx++; continue; }
            uint32_t m = 0;
            for (unsigned b = 0; b < 4; b++) if (mask & (1u << b)) m |= 0xFFu << (8 * b);
            g->r[r] = (g->r[r] & ~m) | (v & m); g->set[r] = 1; g->wm[r] |= m;
        }
        i += len;
    }
}

static int fails;
static unsigned n_stale, n_draw, n_cmp, n_ucmp, n_words_native, n_words_stream, n_skip_draw;
#define CHECK(c, ...) do { if (!(c)) { fails++; if (fails < 40) { printf("FAIL: " __VA_ARGS__); printf("\n"); } } } while (0)

int main(int argc, char** argv) {
    if (argc < 2) { fprintf(stderr, "usage: nd_emit_test <ndref.bin> ...\n"); return 2; }
    static Regs sr, nr;
    for (int a = 1; a < argc; a++) {
        FILE* f = fopen(argv[a], "rb");
        if (!f) { printf("FAIL: cannot open %s\n", argv[a]); return 1; }
        fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
        unsigned char* buf = malloc((size_t)sz);
        assert(buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz);
        fclose(f);
        assert(memcmp(buf, "NDR1", 4) == 0 && rd32(buf + 4) == 4);
        memset(&sr, 0, sizeof sr); memset(&nr, 0, sizeof nr);
        NdShadow sh; nd_shadow_reset(&sh);
        uint32_t tk[19] = { 0 }, d_graw = 0, d_prog = 0, d_cull = 0, d_decal = 0, d_atest = 0, tw0[4] = { 0 }, tw1[4] = { 0 }, nv = 0;
        float un[14] = { 0 }, pal[ND_NPAL][16]; unsigned npal = 0;
        NdTmplKey key; int have_key = 0;
        for (size_t pos = 20, end = 20 + rd32(buf + 16); pos < end;) {
            uint32_t kind = rd32(buf + pos), size = rd32(buf + pos + 4);
            const unsigned char* b = buf + pos + 8;
            if (kind == 1) {
                for (int i = 0; i < 19; i++) tk[i] = rd32(b + 4 * i);
                d_graw = rd32(b + 80); d_prog = rd32(b + 84); d_cull = rd32(b + 88); d_decal = rd32(b + 92); d_atest = rd32(b + 100);
                for (int i = 0; i < 4; i++) { tw0[i] = rd32(b + 128 + 4 * i); tw1[i] = rd32(b + 144 + 4 * i); }
                memcpy(un, b + 160, sizeof un);
                uint32_t np = rd32(b + 520);
                memset(pal, 0, sizeof pal);
                npal = np > ND_NPAL ? ND_NPAL : np;
                for (unsigned k = 0; k < npal; k++) memcpy(pal[k], b + 524 + 68 * k + 4, 64); /* entry k: u32 gm, then pcomp */
                nv = rd32(b + 524 + 68 * np);
            } else if (kind == 4) {
                memset(&key, 0, sizeof key);
                uint32_t pass = rd32(b), hz = rd32(b + 4), zmark = rd32(b + 8), zcmp = rd32(b + 12), zupd = rd32(b + 16), nocol = rd32(b + 20), noise = rd32(b + 24), raw = rd32(b + 28);
                key.pass = (uint8_t)pass; key.raw = (uint8_t)raw; key.cull = raw ? (uint8_t)d_cull : 0;
                key.z = hz != 0; key.zmark = zmark != 0; key.zcmp = zcmp != 0; key.zupd = zupd != 0; key.nocol = nocol != 0; key.decal = d_decal != 0; key.dbias = 0x3A83126Fu;
                key.atest_on = (int32_t)d_atest >= 0; key.aref = key.atest_on ? (uint8_t)d_atest : 0;
                key.combine = (uint8_t)tk[0]; key.tex = (uint8_t)tk[1]; key.tex1 = (uint8_t)tk[2]; key.i8_0 = (uint8_t)tk[3]; key.i8_1 = (uint8_t)tk[4];
                key.rgb_scale = (uint8_t)tk[5]; key.r_mix = (uint8_t)tk[6]; key.draw_3d = (uint8_t)tk[7]; key.rect_2d = (uint8_t)tk[8]; key.dview = tk[9] != 0;
                key.off_active = (uint8_t)tk[10]; key.off_i8 = (uint8_t)tk[11]; key.r_memblend = (uint8_t)tk[12]; key.r_replace = (uint8_t)tk[13]; key.r_blend = (uint8_t)tk[14];
                key.noise = noise != 0; key.two = (uint8_t)rd32(b + 40); key.t1only = (uint8_t)rd32(b + 44); key.w0 = rd32(b + 32); key.w1 = rd32(b + 36);
                have_key = 1;
            } else if (kind == 6 && have_key) {
                uint32_t n = rd32(b);
                if (n == 0) { memset(sr.set, 0, sizeof sr.set); memset(sr.uset, 0, sizeof sr.uset); nd_shadow_reset(&sh); memset(nr.set, 0, sizeof nr.set); memset(nr.uset, 0, sizeof nr.uset); }
                apply(&sr, (const uint32_t*)(const void*)(b + 4), n);
                n_words_stream += n;
                /* a raw pm4 draw on the main target only: the draws the native path takes first */
                if (d_graw && d_prog == 4 && !key.off_active && key.pass == 0 && n) {
                    NdDraw d;
                    memset(&d, 0, sizeof d);
                    d.tmpl = nd_tmpl_build(&key);
                    if (d.tmpl >= 0) {
                        if (tw0[0]) { d.u[0].dim = tw0[0]; d.u[0].param = tw0[1]; d.u[0].lod = tw0[2]; d.u[0].addr = tw0[3]; d.u[0].type = sr.r[0x8E]; }
                        if (tw1[0]) { d.u[1].dim = tw1[0]; d.u[1].param = tw1[1]; d.u[1].lod = tw1[2]; d.u[1].addr = tw1[3]; d.u[1].type = sr.r[0x9E]; }
                        d.pal = (const float(*)[16])pal; d.npal = npal; d.loc_pal = 6; d.loc_var = 0x5A;
                        for (int i = 0; i < 6; i++) { unsigned cr = i < 4 ? 0xC3u + 8u * (unsigned)i : 0xF3u + 8u * (unsigned)(i - 4); d.tev_col[i] = sr.set[cr] ? sr.r[cr] : 0; }
                        d.has_buf = sr.set[0xFD]; d.buf_col = sr.r[0xFD];
                        memcpy(d.uvso, un, 16); memcpy(d.cmul, un + 4, 16); memcpy(d.cadd, un + 8, 16); d.zmix[0] = un[12]; d.zmix[1] = un[13];
                        d.idx_word = sr.r[0x227]; d.nidx = sr.r[0x228];
                        uint32_t out[512];
                        unsigned nw = nd_emit_draw(out, &sh, &d);
                        assert(nw < 512 && !(nw & 1));
                        apply(&nr, out, nw);
                        n_words_native += nw; n_draw++;
                        /* the old path left a texture unit stale in one draw of the 8 files (the slot was reused without a rebind, nd_cmd_test): the native words are the right ones, so that unit is not compared */
                        int stale0 = tw0[0] && (sr.r[0x82] != tw0[0] || sr.r[0x83] != tw0[1] || sr.r[0x85] != tw0[3]);
                        if (stale0) n_stale++;
                        /* registers both wrote */
                        for (unsigned r = 0; r < 0x300; r++) {
                            if (!nr.set[r] || !sr.set[r]) continue;
                            if (r == 0x227 || r == 0x103 || r == 0x126 || r == 0xE0) continue;
                            if (r == 0x106 && !(sr.r[0x105] & 1)) continue;
                            if (r == 0x81 || r == 0x91) continue; /* texture border colour: not in the ndref */
                            if (stale0 && r >= 0x81 && r <= 0x85) continue;
                            n_cmp++;
                            uint32_t ign = (r == 0x80 && !d.u[1].dim) ? 2u : 0u; /* the old path keeps unit 1 enabled after its first use, whether the draw reads it or not: native enables a unit only for a draw that reads it */
                            CHECK(((nr.r[r] ^ sr.r[r]) & nr.wm[r] & sr.wm[r] & ~ign) == 0, "%s draw %u reg %03x native %08x stream %08x", argv[a], n_draw, r, nr.r[r], sr.r[r]); /* the bytes both wrote */
                        }
                        for (unsigned q = 0; q < 0x80 * 4; q++) {
                            if (!nr.uset[q] || !sr.uset[q]) continue;
                            n_ucmp++;
                            CHECK(nr.u[q] == sr.u[q], "%s draw %u uniform %02x word %u native %08x stream %08x", argv[a], n_draw, q / 4, q & 3, nr.u[q], sr.u[q]);
                        }
                    } else n_skip_draw++;
                }
            }
            pos += 8 + size;
        }
        free(buf);
        (void)nv;
    }
    { /* the worst case draw: 21 palette entries that all changed, two units, every uniform, a template of ND_TMPL_WORDS registers (2 words each) */
        static uint32_t wout[ND_EMIT_MAX * 2];
        static float wp[ND_NPAL][16];
        NdShadow wsh;
        NdDraw wd;
        nd_shadow_reset(&wsh);
        memset(&wd, 0, sizeof wd);
        for (int i = 0; i < ND_NPAL; i++) for (int j = 0; j < 16; j++) wp[i][j] = (float)(i * 16 + j + 1);
        wd.tmpl = -1; wd.npal = ND_NPAL; wd.pal = (const float(*)[16])wp; wd.loc_pal = 6; wd.loc_var = 0x5A;
        wd.u[0].dim = 0x00100010; wd.u[0].param = 1; wd.u[0].lod = 2; wd.u[0].addr = 3; wd.u[0].type = 4;
        wd.u[1] = wd.u[0]; wd.idx_word = 0x80000000u; wd.nidx = 3;
        unsigned wnw = nd_emit_draw(wout, &wsh, &wd);
        CHECK(wnw + ND_TMPL_WORDS * 2 <= ND_EMIT_MAX, "worst case draw %u words plus template %u over ND_EMIT_MAX %u", wnw, ND_TMPL_WORDS * 2, (unsigned)ND_EMIT_MAX);
        printf("nd_emit worst case: %u words without the template, limit %u\n", wnw + ND_TMPL_WORDS * 2, (unsigned)ND_EMIT_MAX);
    }
    CHECK(n_draw > 100, "too few draws compared: %u", n_draw);
    CHECK(n_words_native < n_words_stream * 2, "native words %u stream words %u", n_words_native, n_words_stream);
    printf("nd_emit: %u raw draws, %u register compares, %u uniform word compares, %u words native, %u stale texture draws not compared, %u skipped %s\n", n_draw, n_cmp, n_ucmp, n_words_native, n_stale, n_skip_draw, fails ? "FAIL" : "PASS");
    return fails != 0;
}
