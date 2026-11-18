/* nd_ds_test.c: host check of nd_ds_apply (NST1). 12 fixed units, every changed field against a table. Prints `nst1 fields <n> bad <b>`.
 * --fault changes one expected value (bad must be above 0). */
#include <stdio.h>
#include <string.h>
#include "nd_ds.h"
#include "nd_op.h"
#include "nd_gtab.h"
#include <stdint.h>
#include <stdlib.h>

int pm_wide;
unsigned short* nuGfxZBuffer; /* NDR0B: the z-buffer address of the window rule in ds_td0 */
int is_host(u32 p) { return p >= 0x00100000u; }
void tex_win_s25(int* a, int* b, int* c, int* d, unsigned e, unsigned f, unsigned g, unsigned h) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; }
/* NST2A: no device table and no v1 table in the host test (the list case below is a raw typed list). */
const NdNDev* nd_ntab_find(const void* list) { (void)list; return NULL; }
const NdGTab* nd_gtab_find(const void* list) { (void)list; return NULL; }

static int nf, bad;
/* NST2b: the unit of nd_ds_model (the test form of ds_unit: the state write only) */
static void nd_model_unit(NdDs* s, uint32_t w0, uint32_t w1) { nd_ds_apply(s, w0, w1); }
#define CHK(name, got, want) do { nf++; if ((unsigned long)(got) != (unsigned long)(want)) { bad++; printf("nst1 BAD %s got %lx want %lx\n", name, (unsigned long)(got), (unsigned long)(want)); } } while (0)
#define U(id, a, b) nd_ds_apply(&nd_ds, NDO_W0(id, a), (b))
/* NST2F: the shaded sprite units on a zeroed NdDs. Tile 1 CI 4b pal 1 loaded (LOADTILE), TILESIZE tile 1 records m1; tile 0 CI 4b with pal0,
 * COMB comb_a 1 and comb_c 2, TILESIZE tile 0 sets dual_on when pal0 differs from pal 1 (tex_shaded0 :497). */
static void shaded_run(NdDs* s, unsigned pal0) {
    memset(s, 0, sizeof *s);
    nd_ds_apply(s, NDO_W0(NDO_TIMG, 0), 0x00200000u);
    nd_ds_apply(s, NDO_W0(NDO_TILE, 2u << 21 | 0x40u), 1u << 24 | 1u << 20);
    nd_ds_apply(s, NDO_W0(NDO_LOADTILE, 0), 1u << 24 | 124u << 12 | 124u);
    nd_ds_apply(s, NDO_W0(NDO_TILESIZE, 0), 1u << 24 | 124u << 12 | 124u);
    nd_ds_apply(s, NDO_W0(NDO_TILE, 2u << 21), pal0 << 20);
    nd_ds_apply(s, NDO_W0(NDO_COMB, 1u << 20 | 2u << 15), 0);
    nd_ds_apply(s, NDO_W0(NDO_TILESIZE, 0), 124u << 12 | 124u);
}
int main(int argc, char** argv) {
    int fault = argc > 1 && !strcmp(argv[1], "--fault");
    NdDs* s = &nd_ds;
    NdTst* t = &s->w.tst1;
    unsigned ch;
    /* 1 OMH: cycle type 2 (shift 20, length 2) */
    ch = U(NDO_OMH, 20u | 2u << 8, 1u << 20);
    CHK("omh ch", ch, 1); CHK("omode_h", s->w.omode_h, 1u << 20); CHK("om_kh", s->w.om_kh, 3u << 20);
    /* 2 OML: render mode (shift 3, length 29) */
    U(NDO_OML, NDO_A_RENDERMODE, 0x0C184240u);
    CHK("omode_l", s->w.omode_l, 0x0C184240u); CHK("om_kl", s->w.om_kl, 0xFFFFFFF8u);
    /* 3 GEOM: clear G_SHADE, set z-buffer and lighting */
    U(NDO_GEOM, 0xffffffu & ~(unsigned)G_SHADE, G_ZBUFFER | G_LIGHTING);
    CHK("geom", s->w.geom, G_SHADING_SMOOTH | G_ZBUFFER | G_LIGHTING);
    /* 4 PRIM */
    U(NDO_PRIM, 0x1234u, 0x11223344u);
    CHK("prim", s->w.prim, 0x11223344u); CHK("prim_lod", s->w.prim_lod, 0x1234u);
    /* 5 ENV */
    U(NDO_ENV, 0, 0x55667788u);
    CHK("envc", s->w.envc, 0x55667788u);
    /* 6 COMB 2 cycles, fields by the N64 layout */
    {
        unsigned a0 = 1, c0 = 4, aa0 = 1, ac0 = 3, a1 = 3, c1 = 5;
        unsigned b0 = 7, b1 = 2, aa1 = 7, ac1 = 0, d0 = 3, ab0 = 5, ad0 = 0, ab1 = 3, d1 = 6, ad1 = 4;
        unsigned w0 = a0 << 20 | c0 << 15 | aa0 << 12 | ac0 << 9 | a1 << 5 | c1;
        unsigned w1 = b0 << 28 | b1 << 24 | aa1 << 21 | ac1 << 18 | d0 << 15 | ab0 << 12 | ad0 << 9 | ab1 << 3 | d1 << 6 | ad1;
        U(NDO_COMB, w0, w1);
        CHK("comb_w0", s->w.comb_w0, 0xfc000000u | w0); CHK("comb_w1", s->w.comb_w1, w1);
        CHK("comb_a", t->comb_a, a0); CHK("comb_b", t->comb_b, b0); CHK("comb_c", t->comb_c, c0); CHK("comb_d", t->comb_d, d0);
        CHK("comb_aa", t->comb_aa, aa0); CHK("comb_ab", t->comb_ab, ab0); CHK("comb_ac", t->comb_ac, ac0); CHK("comb_ad", t->comb_ad, ad0);
        CHK("comb2_a", t->comb2_a, a1); CHK("comb2_b", t->comb2_b, b1); CHK("comb2_c", t->comb2_c, c1); CHK("comb2_d", t->comb2_d, d1);
        CHK("comb2_aa", t->comb2_aa, aa1); CHK("comb2_ab", t->comb2_ab, ab1); CHK("comb2_ac", t->comb2_ac, ac1); CHK("comb2_ad", t->comb2_ad, ad1);
        CHK("comb_set", s->comb_set, 1); CHK("use_t1", s->w.use_t1, 0); CHK("box_prod", s->box_prod, 0);
        CHK("comb_alpha_shade", s->comb_alpha_shade, 0);
    }
    /* 7 TIMG: fmt 0, siz 2, width 32 */
    U(NDO_TIMG, 2u << 19 | 31u, 0x00200000u);
    CHK("timg", (uintptr_t)s->w.timg, 0x00200000u); CHK("timg_siz", t->timg_siz, 2); CHK("timg_fmt", t->timg_fmt, 0); CHK("timg_stride", t->timg_stride, 32);
    /* 8 TILE 1: tmem 0x40, pal 3, cms 1, cmt 2, masks 5, maskt 6, shifts 7 and 8 */
    U(NDO_TILE, 2u << 19 | 0x40u, 1u << 24 | 3u << 20 | 2u << 18 | 6u << 14 | 8u << 10 | 1u << 8 | 5u << 4 | 7u);
    CHK("tile_tmem", t->tile_tmem[1], 0x40); CHK("tile_pal", t->tile_pal[1], 3); CHK("tile_siz", t->tile_siz[1], 2);
    CHK("wrap0", t->tile_wrap[1][0], 1); CHK("wrap1", t->tile_wrap[1][1], 2); CHK("wrap2", t->tile_wrap[1][2], 5); CHK("wrap3", t->tile_wrap[1][3], 6);
    CHK("shift0", t->tile_shift[1][0], 7); CHK("shift1", t->tile_shift[1][1], 8);
    /* 9 LOADBLOCK tile 1, 256 texels */
    U(NDO_LOADBLOCK, 0, 1u << 24 | 255u << 12);
    CHK("blk_tmem", t->blk[0].tmem, 0x40); CHK("blk_img", (uintptr_t)t->blk[0].img, 0x00200000u); CHK("blk_bytes", t->blk[0].bytes, 512); CHK("blk_n", s->tm.blk_n, 1);
    /* 10 TILESIZE tile 1: 0,0,124,124 */
    U(NDO_TILESIZE, 0, 1u << 24 | 124u << 12 | 124u);
    CHK("sz0", t->tile_sz[1][0], 0); CHK("sz2", t->tile_sz[1][2], 124); CHK("sz3", t->tile_sz[1][3], 124);
    /* 11 NUMLIGHT: 2 lights (w1 = 24 * n) */
    U(NDO_MW, NDO_MW_NUMLIGHT << 16, 48u);
    CHK("lt_n", s->lt_n, 2);
    /* 12 SEG 6 */
    U(NDO_MW, NDO_MW_SEG << 16 | 6u, 0x00300000u);
    CHK("segbase6", s->w.segbase[6], fault ? 0x00300004u : 0x00300000u);
    /* a repeat changes nothing */
    CHK("repeat ch", U(NDO_ENV, 0, 0x55667788u), 0);
    /* 13 NST2A list: a raw typed list {PRIM, CALL B, END}, B = {ENV, END} (fault: B without its ENV) */
    {
        static uint32_t lb[4], la[8];
        lb[0] = NDO_W0(NDO_ENV, 0); lb[1] = 0xCAFE0001u; lb[2] = NDO_W0(NDO_END, 0); lb[3] = 0;
        if (fault) lb[0] = NDO_W0(NDO_END, 0);
        la[0] = NDO_W0(NDO_PRIM, 5); la[1] = 0x11223344u;
        la[2] = NDO_W0(NDO_CALL, 0); la[3] = (uint32_t)(uintptr_t)lb;
        la[4] = NDO_W0(NDO_END, 0); la[5] = 0; la[6] = 0; la[7] = 0;
        nd_ds_list(&nd_ds, la, 0);
        CHK("list prim", s->w.prim, 0x11223344u); CHK("list prim_lod", s->w.prim_lod, 5);
        CHK("list env", s->w.envc, 0xCAFE0001u);
    }
    /* NST2b: one model record through nd_ds_model (q = the payload words, pair Rk = q[2k], q[2k+1]) into a zeroed NdDs. fault: f without bit 8. */
    {
        static NdDs m;
        static uint32_t q[32];
        static Lightsn lsn;
        static uint32_t bind[6], cb[2], rm[2];
        static const uint32_t w0c = 1u << 20 | 4u << 15;
        unsigned mf = 0, mb = 0;
#define C2(name, got, want) do { mf++; if ((unsigned long)(got) != (unsigned long)(want)) { mb++; printf("nst2b BAD %s got %lx want %lx\n", name, (unsigned long)(got), (unsigned long)(want)); } } while (0)
        ((uint8_t*)&lsn.l[0])[0] = 10; ((uint8_t*)&lsn.l[0])[1] = 20; ((uint8_t*)&lsn.l[0])[2] = 30;
        bind[0] = NDO_W0(NDO_TIMG, 2u << 19 | 31u); bind[1] = 0x00200000u;
        bind[2] = NDO_W0(NDO_TILE, 2u << 21 | 2u << 19 | 0x40u); bind[3] = 0;
        bind[4] = NDO_W0(NDO_END, 0); bind[5] = 0;
        cb[0] = NDO_W0(NDO_COMB, w0c); cb[1] = 0x12345678u;
        rm[0] = NDO_W0(NDO_OML, NDO_A_RENDERMODE); rm[1] = 0x0C184240u;
        memset(q, 0, sizeof q);
        q[6] = fault ? (4u | 64u | 2u) : (4u | 8u | 64u | 2u);
        q[12] = 0x11223344u; q[13] = 0x55667788u;
        q[8] = 0x0A0B0C0Du; q[11] = 0x01020304u;
        q[4] = 1u; q[7] = (uint32_t)(uintptr_t)&lsn;
        q[1] = (uint32_t)(uintptr_t)bind;
        q[9] = (uint32_t)(uintptr_t)cb;
        q[18] = NDO_W0(NDO_TILESIZE, 0); q[19] = 124u << 12 | 124u;
        nd_ds_model(&m, q, rm, 1, nd_model_unit);
        C2("prim", m.w.prim, 0x11223344u); C2("envc", m.w.envc, 0x55667788u);
        C2("fogc", m.fogc, 0x0A0B0C0Du); C2("fog_fm", m.fog_fm, 0x0102); C2("fog_fo", m.fog_fo, 0x0304);
        C2("lt_n", m.lt_n, 1); C2("lt0 col0", m.lt[0].col[0], 10); C2("lt0 col1", m.lt[0].col[1], 20); C2("lt0 col2", m.lt[0].col[2], 30);
        C2("timg", (uintptr_t)m.w.timg, 0x00200000u); C2("timg_siz", m.w.tst1.timg_siz, 2); C2("timg_stride", m.w.tst1.timg_stride, 32);
        C2("tile_fmt0", m.w.tst1.tile_fmt[0], 2); C2("tile_siz0", m.w.tst1.tile_siz[0], 2); C2("tile_tmem0", m.w.tst1.tile_tmem[0], 0x40);
        C2("comb_w0", m.w.comb_w0, NDO_COMB_W0(w0c)); C2("comb_w1", m.w.comb_w1, 0x12345678u);
        C2("omode_l", m.w.omode_l, 0x0C184240u);
        C2("tile_sz0", m.w.tst1.tile_sz[0][2], 124); C2("tile_sz3", m.w.tst1.tile_sz[0][3], 124);
        printf("nst2b model fields %u bad %u\n", mf, mb);
        bad += (int)mb;
#undef C2
    }
    /* NST2C (a): a parts record of 2 instances, m 3: {PRIM, CALL (index pre 1), ENV}. Instance 1 PRIM 0x1111 ENV 0x2222, instance 2 PRIM 0x3333 ENV 0x4444.
     * fault: the pre field is 2, so the ENV post unit is the skipped index and envc stays 0. */
    {
        static NdDs pz;
        static uint32_t pu[2 + 2 * 6];
        unsigned xf = 0, xb = 0;
#define C3(name, got, want) do { xf++; if ((unsigned long)(got) != (unsigned long)(want)) { xb++; printf("nst2c BAD %s got %lx want %lx\n", name, (unsigned long)(got), (unsigned long)(want)); } } while (0)
        memset(pu, 0, sizeof pu);
        pu[0] = 2u | (fault ? 2u : 1u) << 8 | 3u << 16;
        pu[2] = NDO_W0(NDO_PRIM, 5); pu[3] = 0x1111u;
        pu[4] = NDO_W0(NDO_CALL, 0); pu[5] = 0xDEADu;
        pu[6] = NDO_W0(NDO_ENV, 0); pu[7] = 0x2222u;
        pu[8] = NDO_W0(NDO_PRIM, 7); pu[9] = 0x3333u;
        pu[10] = NDO_W0(NDO_CALL, 0); pu[11] = 0xBEEFu;
        pu[12] = NDO_W0(NDO_ENV, 0); pu[13] = 0x4444u;
        nd_ds_parts(&pz, pu, nd_model_unit);
        C3("parts prim", pz.w.prim, 0x3333u); C3("parts prim_lod", pz.w.prim_lod, 7);
        C3("parts envc", pz.w.envc, 0x4444u);
        printf("nst2c parts fields %u bad %u\n", xf, xb);
        bad += (int)xb; nf += (int)xf;
#undef C3
    }
    /* NST2C (b): a compiled def {PRIM value slot, ENV}, vmask 1, one value PRIM (0x5, 0xABCD): prim from the value, envc from the static unit. */
    {
        static NdDs dz;
        static uint32_t du[4], dv[2];
        unsigned yf = 0, yb = 0;
#define C4(name, got, want) do { yf++; if ((unsigned long)(got) != (unsigned long)(want)) { yb++; printf("nst2c BAD %s got %lx want %lx\n", name, (unsigned long)(got), (unsigned long)(want)); } } while (0)
        du[0] = NDO_W0(NDO_PRIM, 0); du[1] = 0;
        du[2] = NDO_W0(NDO_ENV, 0); du[3] = 0x7777u;
        dv[0] = NDO_W0(NDO_PRIM, 5); dv[1] = 0xABCDu;
        nd_ds_scomp(&dz, du, 2, 1u, dv, 1, nd_model_unit);
        C4("scomp prim", dz.w.prim, 0xABCDu); C4("scomp prim_lod", dz.w.prim_lod, 5);
        C4("scomp envc", dz.w.envc, 0x7777u);
        printf("nst2c def fields %u bad %u\n", yf, yb);
        bad += (int)yb; nf += (int)yf;
#undef C4
    }
    /* NST2D: the unit log. Case 1: range A..B replays the PRIM X logged between its marks (PRIM Y after the close is not logged).
     * Case 2: a sub save, ENV E, restore: envc back. fault: the range runs B..A (no replay). */
    {
        static uint32_t mA, mB;
        unsigned rf = 0, rb = 0;
#define C5(name, got, want) do { rf++; if ((unsigned long)(got) != (unsigned long)(want)) { rb++; printf("nst2d BAD %s got %lx want %lx\n", name, (unsigned long)(got), (unsigned long)(want)); } } while (0)
        nd_ds_lg_reset();
        nd_ds_mark(&mA);
        nd_ds_lg_open();
        nd_ds_apply(&nd_ds, NDO_W0(NDO_PRIM, 5), 0xAAAAu);
        nd_ds_mark(&mB);
        nd_ds_lg_close();
        nd_ds_apply(&nd_ds, NDO_W0(NDO_PRIM, 9), 0xBBBBu);
        if (fault) nd_ds_range(&nd_ds, &mB, &mA);
        else nd_ds_range(&nd_ds, &mA, &mB);
        C5("range prim", s->w.prim, 0xAAAAu);
        nd_ds_apply(&nd_ds, NDO_W0(NDO_ENV, 0), 0x5555u);
        nd_ds_sub_save();
        nd_ds_apply(&nd_ds, NDO_W0(NDO_ENV, 0), 0x1234u);
        C5("sub env", s->w.envc, 0x1234u);
        nd_ds_sub_restore();
        C5("restore env", s->w.envc, 0x5555u);
        printf("nst2d cases 2 bad %u\n", rb);
        bad += (int)rb; nf += (int)rf;
#undef C5
    }
    /* NST2F: pal0 0 (differs from tile 1 pal 1): dual_on 1 and m1 taken. pal0 1 (same pal): dual_on 0. */
    {
        static NdDs sh;
        unsigned hf = 0, hb = 0;
#define C6(name, got, want) do { hf++; if ((unsigned long)(got) != (unsigned long)(want)) { hb++; printf("nst2f BAD %s got %lx want %lx\n", name, (unsigned long)(got), (unsigned long)(want)); } } while (0)
        shaded_run(&sh, 0);
        C6("shaded dual_on", sh.dual_on, 1); C6("shaded m1v", sh.m1v, 0);
        shaded_run(&sh, 1);
        C6("same pal dual_on", sh.dual_on, 0); C6("same pal m1v", sh.m1v, 0);
        printf("nst2f shaded bad %u\n", hb);
        bad += (int)hb; nf += (int)hf;
#undef C6
    }
    /* NDR0B: the texture descriptors. Case tlut: td[0] valid, fmt 2, pal A, tile_pal[0] 0, tmem 256; TIMG B; LOADTLUT moves td[0].pal to B
     * (fault: tile_pal[0] 1, the slot differs, td[0].pal stays A). Case tile 1: td[1] equals nd_texw1 on the same state. */
    {
        static NdDs tl, t1s;
        static unsigned char palA[32];
        unsigned rf = 0, rb = 0;
#define C7(name, got, want) do { rf++; if ((uintptr_t)(got) != (uintptr_t)(want)) { rb++; printf("ndr0b BAD %s got %lx want %lx\n", name, (unsigned long)(uintptr_t)(got), (unsigned long)(uintptr_t)(want)); } } while (0)
        memset(&tl, 0, sizeof tl);
        nd_ds_apply(&tl, NDO_W0(NDO_TILE, 2u << 21 | 0x100u), (fault ? 1u : 0u) << 20);
        tl.w.td[0].valid = 1; tl.w.td[0].fmt = 2; tl.w.td[0].pal = palA;
        nd_ds_apply(&tl, NDO_W0(NDO_TIMG, 2u << 19 | 31u), 0x00300000u);
        nd_ds_apply(&tl, NDO_W0(NDO_LOADTLUT, 0), 15u << 14);
        C7("tlut td0 pal", tl.w.td[0].pal, 0x00300000u);
        memset(&t1s, 0, sizeof t1s);
        nd_ds_apply(&t1s, NDO_W0(NDO_TIMG, 2u << 19 | 31u), 0x00200000u);
        nd_ds_apply(&t1s, NDO_W0(NDO_TILE, 2u << 21 | 0x40u), 1u << 24 | 1u << 20);
        nd_ds_apply(&t1s, NDO_W0(NDO_LOADTILE, 0), 1u << 24 | 124u << 12 | 124u);
        nd_ds_apply(&t1s, NDO_W0(NDO_TILESIZE, 0), 1u << 24 | 124u << 12 | 124u);
        {
            NdWin w1;
            nd_texw1(&t1s.w.tst1, &t1s.tm, t1s.w.timg, 0, 0, 124, 124, &w1);
            C7("t1 valid", t1s.w.td[1].valid, !!(w1.src && w1.w > 0 && w1.h > 0));
            C7("t1 src", t1s.w.td[1].src, w1.src);
            C7("t1 w", t1s.w.td[1].w, w1.w); C7("t1 h", t1s.w.td[1].h, w1.h);
            C7("t1 stride", t1s.w.td[1].stride, w1.stride); C7("t1 x0", t1s.w.td[1].x0, w1.x0);
        }
        printf("ndr0b cases 2 bad %u\n", rb);
        bad += (int)rb; nf += (int)rf;
#undef C7
    }
    printf("nst2c fields %d bad %d\n", nf, bad);
    printf("nst1 fields %d bad %d\n", nf, bad);
    return bad ? 1 : 0;
}
