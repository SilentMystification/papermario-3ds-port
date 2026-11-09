/* nd_cmd_test.c: host test of nd_tmpl words against the real command stream (ndref v4 kind 6, S3 oracle).
 * The command words of every draw call are replayed into a register file (a write marks the register as set). For each draw the template of its key must equal the
 * register file on every register the template writes and the frame has written by then. This checks the EffectBind and TexEnvBind register map of nd_tmpl against the
 * words the GPU gets, not against the citro3d context copy (nd_tmpl_test).
 * Usage: nd_cmd_test <ndref.bin> ... */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd.h"
#include "ultra64.h"
#include "nd_op.h"

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
static uint32_t rf[0x300]; static uint8_t rset[0x300];
static int fails; static unsigned n_draw, n_cmp, n_skip, n_tex, n_texbad;
#define CHECK(c, ...) do { if (!(c)) { fails++; if (fails < 100000) { printf("FAIL: " __VA_ARGS__); printf("\n"); } } } while (0)

static void apply(const uint32_t* w, unsigned n) {
    unsigned i = 0;
    while (i + 1 < n) {
        uint32_t p0 = w[i], hd = w[i + 1];
        unsigned reg = hd & 0xFFFF, mask = (hd >> 16) & 0xF, extra = (hd >> 20) & 0xFF, consec = hd >> 31;
        unsigned len = 2 + extra + (extra & 1);
        if (i + len > n) break;
        for (unsigned k = 0; k <= extra; k++) {
            unsigned r = consec ? reg + k : reg;
            uint32_t v = k == 0 ? p0 : w[i + 1 + k];
            if (r >= 0x300) continue;
            uint32_t m = 0;
            for (unsigned b = 0; b < 4; b++) if (mask & (1u << b)) m |= 0xFFu << (8 * b);
            rf[r] = (rf[r] & ~m) | (v & m); rset[r] = 1;
        }
        i += len;
    }
}

/* The F3DEX2 othermode word of a typed othermode op (the inverse of nd_op_xlate). */
static uint32_t nd_op_om_word(const uint32_t u[2]) {
    const unsigned a = u[0] & 0xffffffu, sft = a & 0xffu, len = (a >> 8) & 0xffu;
    const unsigned op = (u[0] >> 24) == NDO_OMH ? G_SETOTHERMODE_H : G_SETOTHERMODE_L;
    return ((uint32_t)op << 24) | (((32u - sft - len) & 0xffu) << 8) | ((len - 1u) & 0xffu);
}

/* HC4a: the typed op format (nd_op.h): ids free of the F3DEX2 and G_PM_* ops, the translation of each scalar op, the othermode inverse. */
static void op_test(void) {
    for (unsigned id = NDO_BASE; id < NDO_END_; id++) {
        CHECK(!(id >= 0x01 && id <= 0x07) && !(id >= 0x0d && id <= 0x13) && id < 0xd7, "typed id %02x collides", id);
        if (id != NDO_RARE) CHECK(nd_op_n64[id - NDO_BASE] >= 0xd7, "typed id %02x has no F3DEX2 op", id);
    }
    Gfx g[16]; uint32_t t[2];
    gDPSetCycleType(&g[0], G_CYC_2CYCLE);
    CHECK(nd_op_xlate(g[0].words.w0, g[0].words.w1, t) && (t[0] >> 24) == NDO_OMH && t[1] == g[0].words.w1, "cycle type");
    CHECK(nd_op_om_mask(t[0] & 0xffffffu) == (3u << G_MDSFT_CYCLETYPE), "cycle type mask %08x", nd_op_om_mask(t[0] & 0xffffffu));
    CHECK(nd_op_om_word(t) == g[0].words.w0, "othermode H inverse %08x %08x", nd_op_om_word(t), g[0].words.w0);
    gDPSetRenderMode(&g[1], G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
    CHECK(nd_op_xlate(g[1].words.w0, g[1].words.w1, t) && (t[0] >> 24) == NDO_OML && nd_op_om_word(t) == g[1].words.w0, "render mode");
    CHECK(nd_op_om_mask(t[0] & 0xffffffu) == 0xfffffff8u, "render mode mask %08x", nd_op_om_mask(t[0] & 0xffffffu));
    gSPSetOtherMode(&g[2], G_SETOTHERMODE_L, 0, 32, 0x12345678);
    CHECK(nd_op_xlate(g[2].words.w0, g[2].words.w1, t) && nd_op_om_mask(t[0] & 0xffffffu) == 0xffffffffu, "othermode 32 bits");
    gSPGeometryMode(&g[3], G_LIGHTING, G_ZBUFFER | G_SHADE);
    CHECK(nd_op_xlate(g[3].words.w0, g[3].words.w1, t) && (t[0] >> 24) == NDO_GEOM && (t[0] & 0xffffffu) == (g[3].words.w0 & 0xffffffu) &&
          t[1] == g[3].words.w1, "geometry mode");
    gDPSetPrimColor(&g[4], 3, 200, 1, 2, 3, 4);
    CHECK(nd_op_xlate(g[4].words.w0, g[4].words.w1, t) && (t[0] >> 24) == NDO_PRIM && (t[0] & 0xffffffu) == (g[4].words.w0 & 0xffffu) &&
          t[1] == 0x01020304u, "prim colour");
    gSPTexture(&g[5], 0x8000, 0x4000, 0, G_TX_RENDERTILE, G_ON);
    CHECK(nd_op_xlate(g[5].words.w0, g[5].words.w1, t) && (t[0] >> 24) == NDO_TEXTURE && (t[0] & 0xffffffu) == 1u && t[1] == 0x80004000u, "texture");
    gSPSegment(&g[6], 5, 0x00123450);
    CHECK(nd_op_xlate(g[6].words.w0, g[6].words.w1, t) && t[0] == NDO_W0(NDO_MW, NDO_MW_SEG << 16 | 5u) && t[1] == 0x00123450u, "segment");
    gSPFogPosition(&g[7], 900, 1000);
    CHECK(nd_op_xlate(g[7].words.w0, g[7].words.w1, t) && t[0] == NDO_W0(NDO_MW, NDO_MW_FOG << 16), "fog position");
    gSPNumLights(&g[8], NUMLIGHTS_2);
    CHECK(nd_op_xlate(g[8].words.w0, g[8].words.w1, t) && t[0] == NDO_W0(NDO_MW, NDO_MW_NUMLIGHT << 16) && t[1] == g[8].words.w1, "light count");
    gMoveWd(&g[9], G_MW_LIGHTCOL, G_MWO_aLIGHT_3, 0x11223300);
    CHECK(nd_op_xlate(g[9].words.w0, g[9].words.w1, t) && t[0] == NDO_W0(NDO_MW, NDO_MW_LIGHTCOL << 16 | 2u), "light colour %08x", t[0]);
    gDPSetScissor(&g[10], G_SC_NON_INTERLACE, 10, 20, 300, 220);
    CHECK(nd_op_xlate(g[10].words.w0, g[10].words.w1, t) && (t[0] >> 24) == NDO_SCISSOR && (t[0] & 0xffffffu) == (g[10].words.w0 & 0xffffffu) &&
          t[1] == (g[10].words.w1 & 0xffffffu), "scissor");
    gDPSetEnvColor(&g[11], 9, 8, 7, 6);
    CHECK(nd_op_xlate(g[11].words.w0, g[11].words.w1, t) && t[0] == NDO_W0(NDO_ENV, 0) && t[1] == 0x09080706u, "env colour");
    gSPVertex(&g[12], (Vtx*)0x00200000, 4, 0);
    CHECK(!nd_op_xlate(g[12].words.w0, g[12].words.w1, t) && t[0] == g[12].words.w0 && t[1] == g[12].words.w1, "vertex word kept");
    gDPSetCombineMode(&g[13], G_CC_SHADE, G_CC_SHADE);
    CHECK(nd_op_xlate(g[13].words.w0, g[13].words.w1, t) && (t[0] >> 24) == NDO_COMB && NDO_COMB_W0(t[0]) == g[13].words.w0 &&
          t[1] == g[13].words.w1, "combine (HC4b)");
    gDPSetRenderMode(&g[14], G_RM_FOG_SHADE_A, G_RM_AA_ZB_OPA_SURF2);
    CHECK(nd_op_xlate(g[14].words.w0, g[14].words.w1, t) && t[0] == NDO_W0(NDO_OML, NDO_A_RENDERMODE) && t[1] == g[14].words.w1,
          "render mode a field %08x", t[0]);
    gDPPipeSync(&g[15]);
    CHECK(!nd_op_xlate(g[15].words.w0, g[15].words.w1, t), "sync stays a word");
    /* HC4c: the texture ops keep their F3DEX2 fields (a = w0 low 24 bits, b = w1); a bind list types in place without its syncs, to NDO_END. */
    Gfx l[16];
    Gfx* lp = l;
    static const uint16_t pal[16];
    gDPLoadTLUT_pal16(lp++, 0, pal);
    gDPLoadTextureTile(lp++, (void*)0x00123000, G_IM_FMT_CI, G_IM_SIZ_8b, 32, 16, 0, 0, 31, 15, 0, G_TX_WRAP, G_TX_WRAP, 5, 4, G_TX_NOLOD, G_TX_NOLOD);
    gSPEndDisplayList(lp++);
    const unsigned nw = (unsigned)(lp - l);
    Gfx c[16];
    memcpy(c, l, sizeof l);
    const unsigned nt = nd_op_type_run((uint32_t*)c, nw);
    static const unsigned want[] = { NDO_TIMG, NDO_TILE, NDO_LOADTLUT, NDO_TIMG, NDO_TILE, NDO_LOADTILE, NDO_TILE, NDO_TILESIZE };
    CHECK(nt == sizeof want / sizeof want[0] && (c[nt].words.w0 >> 24) == NDO_END, "bind list %u units", nt);
    for (unsigned i = 0, j = 0; i < nt && j < nw; i++, j++) {
        while (j < nw && ((l[j].words.w0 >> 24) == G_RDPLOADSYNC || (l[j].words.w0 >> 24) == G_RDPPIPESYNC || (l[j].words.w0 >> 24) == G_RDPTILESYNC)) j++;
        CHECK((c[i].words.w0 >> 24) == want[i] && nd_op_n64[want[i] - NDO_BASE] == (l[j].words.w0 >> 24) && (c[i].words.w0 & 0xffffffu) == (l[j].words.w0 & 0xffffffu) &&
              c[i].words.w1 == l[j].words.w1, "bind op %u", i);
        CHECK(nd_op_tex(c[i].words.w0 >> 24), "bind op %u is a texture op", i);
    }
}

int main(int argc, char** argv) {
    op_test();
    if (argc < 2) { fprintf(stderr, "usage: nd_cmd_test <ndref.bin> ...\n"); return 2; }
    for (int a = 1; a < argc; a++) {
        FILE* f = fopen(argv[a], "rb");
        if (!f) { printf("FAIL: cannot open %s\n", argv[a]); return 1; }
        fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
        unsigned char* buf = malloc((size_t)sz);
        assert(buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz);
        fclose(f);
        assert(memcmp(buf, "NDR1", 4) == 0 && rd32(buf + 4) == 4);
        memset(rf, 0, sizeof rf); memset(rset, 0, sizeof rset);
        uint32_t tk[19] = { 0 }, d_decal = 0, d_atest = 0, d_cull = 0, d_graw = 0, tw0[4] = { 0 }, tw1[4] = { 0 };
        NdTmplKey key; int have_key = 0;
        for (size_t pos = 20, end = 20 + rd32(buf + 16); pos < end;) {
            uint32_t kind = rd32(buf + pos), size = rd32(buf + pos + 4);
            const unsigned char* b = buf + pos + 8;
            if (kind == 1) {
                for (int i = 0; i < 19; i++) tk[i] = rd32(b + 4 * i);
                d_graw = rd32(b + 80); d_cull = rd32(b + 88); d_decal = rd32(b + 92); d_atest = rd32(b + 100);
                for (int i = 0; i < 4; i++) { tw0[i] = rd32(b + 128 + 4 * i); tw1[i] = rd32(b + 144 + 4 * i); }
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
                if (n == 0) memset(rset, 0, sizeof rset); /* the command buffer was split since the draw before: the words of that part are not in the stream */
                apply((const uint32_t*)(const void*)(b + 4), n); /* little endian host: the words are in file order */
                n_draw++;
                /* texture unit 0 and 1 blocks: dim, param, lod, address (regs 0x82 to 0x85 and 0x92 to 0x95) */
                for (int u = 0; u < 2; u++) {
                    const uint32_t* tw = u ? tw1 : tw0;
                    unsigned base = u ? 0x91u : 0x81u;
                    if (!(tw[0] | tw[1]) || !rset[base + 1] || key.off_active) continue;
                    n_cmp++; n_tex++;
                    /* the ndref words are read at the draw call, the stream holds what citro3d wrote: a texture whose slot was reused without a rebind keeps its old unit words
                     * (one draw of 632 in the 8 files). Up to 1 percent is counted, not failed. */
                    if (!(rf[base + 1] == tw[0] && rf[base + 2] == tw[1] && rf[base + 3] == tw[2] && rf[base + 4] == tw[3])) n_texbad++;
                }
                int id = nd_tmpl_build(&key);
                if (id >= 0) {
                    const NdTmplWord* w; unsigned nw = nd_tmpl_words(id, &w);
                    for (unsigned i = 0; i < nw; i++) {
                        unsigned r = w[i].reg;
                        /* a template colour word (0 or 0xFFFFFFFF) and the stage words past the stages in use are skipped; buffer words too */
                        if (r >= 0x300 || !rset[r]) { n_skip++; continue; }
                        if ((r >= 0xC0 && r < 0x100 && (r & 7) == 3) || r == 0xFD || r == 0xE0 || r == 0x126 || r == 0x103 || (r == 0x106 && !(rf[0x105] & 1))) { n_skip++; continue; } /* 0x106: stale while the stencil test is off */
                        n_cmp++;
                        CHECK(rf[r] == w[i].val, "%s draw %u reg %03x template %08x stream %08x (pass %u raw %u z %u)", argv[a], n_draw, r, w[i].val, rf[r], (unsigned)key.pass, (unsigned)key.raw, (unsigned)key.z);
                    }
                }
            }
            pos += 8 + size;
        }
        free(buf);
    }
    CHECK(n_texbad * 100 <= n_tex, "texture unit words differ in %u of %u blocks", n_texbad, n_tex);
    printf("nd_cmd: %u draws, %u register compares (%u texture unit blocks, %u stale), %u skipped %s\n", n_draw, n_cmp, n_tex, n_texbad, n_skip, fails ? "FAIL" : "PASS");
    return fails != 0;
}
