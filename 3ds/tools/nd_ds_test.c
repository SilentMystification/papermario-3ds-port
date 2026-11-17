/* nd_ds_test.c: host check of nd_ds_apply (NST1). 12 fixed units, every changed field against a table. Prints `nst1 fields <n> bad <b>`.
 * --fault changes one expected value (bad must be above 0). */
#include <stdio.h>
#include <string.h>
#include "nd_ds.h"
#include "nd_op.h"

int pm_wide;
int is_host(u32 p) { return p >= 0x00100000u; }
void tex_win_s25(int* a, int* b, int* c, int* d, unsigned e, unsigned f, unsigned g, unsigned h) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; }

static int nf, bad;
#define CHK(name, got, want) do { nf++; if ((unsigned long)(got) != (unsigned long)(want)) { bad++; printf("nst1 BAD %s got %lx want %lx\n", name, (unsigned long)(got), (unsigned long)(want)); } } while (0)
#define U(id, a, b) nd_ds_apply(&nd_ds, NDO_W0(id, a), (b))

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
    printf("nst1 fields %d bad %d\n", nf, bad);
    return bad ? 1 : 0;
}
