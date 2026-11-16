/* nd_nmdl0_test.c: host test of NMDL0. (1) The O forms added for src/model.c (nd_at.h, nd_tx.h) against the gbi macros they replace, unit for unit
 * (the shapes of nd_at_test.c, whose stubs and runner this file includes). (2) make_texture_gfx and SolidCombineModes as extracted from src/model.c
 * against the frozen gbi-macro form (3ds/tests/nmdl0/old_mtg.c): every format, size, palette, aux and combine case gives the same units,
 * the sync units apart (a sync is a no-op unit that the bind list reader and the record writer drop). Prints "nmdl0 ok <n>" or "nmdl0 FAIL <what>". */
#define main ns3_main
#include "nd_at_test.c"
#undef main

void old_make_texture_gfx(TextureHeader* header, Gfx** gfxPos, IMG_PTR raster, PAL_PTR palette, IMG_PTR auxRaster, PAL_PTR auxPalette, u8 auxShiftS, u8 auxShiftT, u16 auxOffsetS, u16 auxOffsetT);
void make_texture_gfx(TextureHeader* header, uint32_t** nrpp, IMG_PTR raster, PAL_PTR palette, IMG_PTR auxRaster, PAL_PTR auxPalette, u8 auxShiftS, u8 auxShiftT, u16 auxOffsetS, u16 auxOffsetT);
extern NdCmbW SolidCombineModes[][5];
extern Gfx old_SolidCombineModes[][5];

/* the new shapes: each gbi macro (A) against the O form (B) */
static void a_tex_on(void) { gSPTexture(p++, 0xFFFF, 0xFFFF, 3, G_TX_RENDERTILE, G_ON); }
static void b_tex_on(void) { NDST_TEX(ND_NR_AT, 0xFFFF, 0xFFFF, 3, G_TX_RENDERTILE, G_ON); }
static void a_tex_off(void) { gSPTexture(p++, 0, 0, 0, G_TX_RENDERTILE, G_OFF); }
static void b_tex_off(void) { NDST_TEX(ND_NR_AT, 0, 0, 0, G_TX_RENDERTILE, G_OFF); }
static void a_adither(void) { gDPSetAlphaDither(p++, G_AD_PATTERN); }
static void b_adither(void) { NDST_ADITHER(ND_NR_AT, G_AD_PATTERN); }
static void a_omh(void) { gSPSetOtherMode(p++, G_SETOTHERMODE_H, 4, 16, G_TF_BILERP | G_TC_FILT | G_TT_RGBA16 | G_TL_LOD | G_TP_PERSP); }
static void b_omh(void) { NDST_OMH(ND_NR_AT, 4, 16, G_TF_BILERP | G_TC_FILT | G_TT_RGBA16 | G_TL_LOD | G_TP_PERSP); }
static void a_end(void) { gSPEndDisplayList(p++); }
static void b_end(void) { NDST_END(ND_NR_AT); }
static void a_fogc(void) { gDPSetFogColor(p++, 11, 22, 33, 44); }
static void b_fogc(void) { NDVAL_FOG(ND_NR_AT, 11, 22, 33, 44); }
static void a_fogp(void) { gSPFogPosition(p++, 900, 1000); }
static void b_fogp(void) { NDVAL_FOGPOS(ND_NR_AT, 900, 1000); }
/* the b field values of the record (model.c): the gbi unit's w1 against the value macro */
static uint32_t w1_of(void (*a)(void)) { memset(pk, 0, sizeof pk); p = pk; a(); return pk[0].words.w1; }
static void a_blend(void) { gDPSetBlendColor(p++, 11, 22, 33, 255); }
static void a_prim4(void) { gDPSetPrimColor(p++, 0, 0, 11, 22, 33, 255); }
static void a_env4(void) { gDPSetEnvColor(p++, 11, 22, 33, 255); }
static void a_fogc4(void) { gDPSetFogColor(p++, 11, 22, 33, 0); }
static void a_fogp4(void) { gSPFogPosition(p++, 970, 1000); }
static void a_tsz(void) { gDPSetTileSize(p++, G_TX_RENDERTILE + 1, 7, 9, 301, 205); }
static void b_tsz(void) { NDTX_TSZ(ND_NR_AT, G_TX_RENDERTILE + 1, 7, 9, 301, 205); }
static void a_pushv(void) { s32 m = G_MTX_MUL | G_MTX_PUSH; gSPMatrix(p++, &gMtxAt, m | G_MTX_MODELVIEW); }
static void b_pushv(void) { s32 m = G_MTX_MUL | G_MTX_PUSH; NDXF_PUSH(ND_NR_AT, &gMtxAt, m | G_MTX_MODELVIEW); }
static void a_cc2(void) { gDPSetCombineMode(p++, G_CC_SHADE, G_CC_SHADE); }
static void b_cc2(void) { NDST_CC(ND_NR_AT, G_CC_SHADE, G_CC_SHADE); }
static void a_tlut256(void) { gDPLoadTLUT_pal256(p++, gTimg); }
static void b_tlut256(void) { NDTX_LOAD_TLUT256(ND_NR_AT, gTimg); }
static void a_tlut16b(void) { gDPLoadTLUT_pal16(p++, 0, gTimg); }
static void b_tlut16b(void) { NDTX_LOAD_TLUT16(ND_NR_AT, 0, gTimg); }
static void a_mtile(void) { gDPLoadMultiTile(p++, gTimg, 5, 2, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 16, 0, 0, 31, 15, 0, G_TX_WRAP, G_TX_MIRROR, 4, 3, 2, 2); }
static void b_mtile(void) { NDTX_LOAD_MTILE(ND_NR_AT, gTimg, 5, 2, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 16, 0, 0, 31, 15, 0, G_TX_WRAP, G_TX_MIRROR, 4, 3, 2, 2); }
static void a_mtile8(void) { gDPLoadMultiTile(p++, gTimg, 5, 1, G_IM_FMT_CI, G_IM_SIZ_8b, 64, 8, 0, 0, 63, 7, 0, G_TX_WRAP, G_TX_MIRROR, 6, 3, 1, 1); }
static void b_mtile8(void) { NDTX_LOAD_MTILE(ND_NR_AT, gTimg, 5, 1, G_IM_FMT_CI, G_IM_SIZ_8b, 64, 8, 0, 0, 63, 7, 0, G_TX_WRAP, G_TX_MIRROR, 6, 3, 1, 1); }
static void a_mtile32(void) { gDPLoadMultiTile(p++, gTimg, 5, 1, G_IM_FMT_RGBA, G_IM_SIZ_32b, 16, 8, 0, 0, 15, 7, 0, G_TX_WRAP, G_TX_MIRROR, 4, 3, 1, 1); }
static void b_mtile32(void) { NDTX_LOAD_MTILE(ND_NR_AT, gTimg, 5, 1, G_IM_FMT_RGBA, G_IM_SIZ_32b, 16, 8, 0, 0, 15, 7, 0, G_TX_WRAP, G_TX_MIRROR, 4, 3, 1, 1); }
static void a_mtile4(void) { gDPLoadMultiTile_4b(p++, gTimg, 5, 3, G_IM_FMT_CI, 64, 32, 0, 0, 63, 31, 1, G_TX_WRAP, G_TX_MIRROR, 6, 5, 3, 3); }
static void b_mtile4(void) { NDTX_LOAD_MTILE_4B(ND_NR_AT, gTimg, 5, 3, G_IM_FMT_CI, 64, 32, 0, 0, 63, 31, 1, G_TX_WRAP, G_TX_MIRROR, 6, 5, 3, 3); }
static void a_smtile(void) { gDPScrollMultiTile(p++, gTimg, 17, 1, G_IM_FMT_IA, G_IM_SIZ_8b, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_MIRROR, 5, 5, 1, 2, 9, 13); }
static void b_smtile(void) { NDTX_SCROLL_MTILE(ND_NR_AT, gTimg, 17, 1, G_IM_FMT_IA, G_IM_SIZ_8b, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_MIRROR, 5, 5, 1, 2, 9, 13); }
static void a_smtile16(void) { gDPScrollMultiTile(p++, gTimg, 17, 1, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, 0, 31, 31, 0, G_TX_WRAP, G_TX_MIRROR, 5, 5, 1, 2, 9, 13); }
static void b_smtile16(void) { NDTX_SCROLL_MTILE(ND_NR_AT, gTimg, 17, 1, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, 0, 31, 31, 0, G_TX_WRAP, G_TX_MIRROR, 5, 5, 1, 2, 9, 13); }
static void a_smtile4(void) { gDPScrollMultiTile_4b(p++, gTimg, 17, 1, G_IM_FMT_CI, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_MIRROR, 5, 5, 1, 2, 9, 13); }
static void b_smtile4(void) { NDTX_SCROLL_MTILE_4B(ND_NR_AT, gTimg, 17, 1, G_IM_FMT_CI, 32, 32, 0, 0, 31, 31, 1, G_TX_WRAP, G_TX_MIRROR, 5, 5, 1, 2, 9, 13); }
static void a_hh(void) { gDPScrollTextureBlockHalfHeight(p++, gTimg, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_MIRROR, 5, 5, 1, 2, 9, 13, 3, 4); }
static void b_hh(void) { NDTX_SCROLL_HH(ND_NR_AT, gTimg, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_MIRROR, 5, 5, 1, 2, 9, 13, 3, 4); }
static void a_hh8(void) { gDPScrollTextureBlockHalfHeight(p++, gTimg, G_IM_FMT_IA, G_IM_SIZ_8b, 32, 32, 0, G_TX_WRAP, G_TX_MIRROR, 5, 5, 1, 2, 9, 13, 3, 4); }
static void b_hh8(void) { NDTX_SCROLL_HH(ND_NR_AT, gTimg, G_IM_FMT_IA, G_IM_SIZ_8b, 32, 32, 0, G_TX_WRAP, G_TX_MIRROR, 5, 5, 1, 2, 9, 13, 3, 4); }
static void a_hh32(void) { gDPScrollTextureBlockHalfHeight(p++, gTimg, G_IM_FMT_RGBA, G_IM_SIZ_32b, 16, 16, 0, G_TX_WRAP, G_TX_MIRROR, 4, 4, 1, 2, 9, 13, 3, 4); }
static void b_hh32(void) { NDTX_SCROLL_HH(ND_NR_AT, gTimg, G_IM_FMT_RGBA, G_IM_SIZ_32b, 16, 16, 0, G_TX_WRAP, G_TX_MIRROR, 4, 4, 1, 2, 9, 13, 3, 4); }
static void a_hh4(void) { gDPScrollTextureBlockHalfHeight_4b(p++, gTimg, G_IM_FMT_CI, 32, 32, 1, G_TX_WRAP, G_TX_MIRROR, 5, 5, 1, 2, 9, 13, 3, 4); }
static void b_hh4(void) { NDTX_SCROLL_HH_4B(ND_NR_AT, gTimg, G_IM_FMT_CI, 32, 32, 1, G_TX_WRAP, G_TX_MIRROR, 5, 5, 1, 2, 9, 13, 3, 4); }
static void a_ltile8(void) { gDPLoadTextureTile(p++, gTimg, G_IM_FMT_CI, G_IM_SIZ_8b, 32, 16, 0, 0, 31, 15, 0, G_TX_WRAP, G_TX_MIRROR, 5, 4, G_TX_NOLOD, G_TX_NOLOD); }
static void b_ltile8(void) { NDTX_LOAD_TILE(ND_NR_AT, gTimg, G_IM_FMT_CI, G_IM_SIZ_8b, 32, 16, 0, 0, 31, 15, 0, G_TX_WRAP, G_TX_MIRROR, 5, 4, G_TX_NOLOD, G_TX_NOLOD); }
static void a_lblk32(void) { gDPLoadTextureBlock(p++, gTimg, G_IM_FMT_RGBA, G_IM_SIZ_32b, 16, 16, 0, G_TX_WRAP, G_TX_WRAP, 4, 4, G_TX_NOLOD, G_TX_NOLOD); }
static void b_lblk32(void) { NDTX_LOAD_BLOCK(ND_NR_AT, gTimg, G_IM_FMT_RGBA, G_IM_SIZ_32b, 16, 16, 0, G_TX_WRAP, G_TX_WRAP, 4, 4, G_TX_NOLOD, G_TX_NOLOD); }
static const Shape nshapes[] = {
    { "tex_on", a_tex_on, b_tex_on }, { "tex_off", a_tex_off, b_tex_off }, { "adither", a_adither, b_adither }, { "omh", a_omh, b_omh },
    { "end", a_end, b_end }, { "fogc", a_fogc, b_fogc }, { "fogp", a_fogp, b_fogp }, { "tsz", a_tsz, b_tsz },
    { "pushv", a_pushv, b_pushv }, { "cc2", a_cc2, b_cc2 }, { "tlut256", a_tlut256, b_tlut256 }, { "tlut16b", a_tlut16b, b_tlut16b },
    { "mtile", a_mtile, b_mtile }, { "mtile8", a_mtile8, b_mtile8 }, { "mtile32", a_mtile32, b_mtile32 }, { "mtile4", a_mtile4, b_mtile4 },
    { "smtile", a_smtile, b_smtile }, { "smtile16", a_smtile16, b_smtile16 }, { "smtile4", a_smtile4, b_smtile4 },
    { "hh", a_hh, b_hh }, { "hh8", a_hh8, b_hh8 }, { "hh32", a_hh32, b_hh32 }, { "hh4", a_hh4, b_hh4 },
    { "ltile8", a_ltile8, b_ltile8 }, { "lblk32", a_lblk32, b_lblk32 },
};

/* make_texture_gfx: old list (Gfx words, sync units dropped) against the new list (uint32_t pairs), over every case */
static Gfx lo[1024];
static uint32_t ln[2048];
static uint8_t gRas[1 << 16], gAux[1 << 16], gPal[512], gAuxPal[512];
static int mtg_case(const TextureHeader* h, int pal, int apal, unsigned sS, unsigned sT, unsigned oS, unsigned oT) {
    static uint32_t ua[2048];
    TextureHeader h1 = *h, h2 = *h;
    Gfx* op = lo;
    uint32_t* np = ln;
    unsigned na = 0, nb;
    memset(lo, 0, sizeof lo); memset(ln, 0, sizeof ln);
    old_make_texture_gfx(&h1, &op, (IMG_PTR)gRas, pal ? (PAL_PTR)gPal : NULL, (IMG_PTR)gAux, apal ? (PAL_PTR)gAuxPal : NULL, (u8)sS, (u8)sT, (u16)oS, (u16)oT);
    make_texture_gfx(&h2, &np, (IMG_PTR)gRas, pal ? (PAL_PTR)gPal : NULL, (IMG_PTR)gAux, apal ? (PAL_PTR)gAuxPal : NULL, (u8)sS, (u8)sT, (u16)oS, (u16)oT);
    for (Gfx* g = lo; g < op; g++) { if (g->words.w0 == ND_NOP_A) continue; ua[2u * na] = g->words.w0; ua[2u * na + 1u] = g->words.w1; na++; }
    nb = (unsigned)(np - ln) / 2u;
    return na == nb && memcmp(ua, ln, na * 8u) == 0 && na > 0;
}

int main(void) {
    static Gfx dummy[4];
    unsigned n = (unsigned)(sizeof nshapes / sizeof nshapes[0]), ok = 0, ncase = 0;
    gMainGfxPos = dummy + 1;
    for (unsigned i = 0; i < n; i++) {
        if (!run_shape(&nshapes[i])) { printf("nmdl0 FAIL %s\n", nshapes[i].name); return 1; }
        ok++;
    }
    /* the b fields of the record: the value macros against the gbi unit's w1 */
    if (w1_of(a_blend) != ND_NR_RGBA(11, 22, 33, 255)) { printf("nmdl0 FAIL blend value\n"); return 1; }
    if (w1_of(a_prim4) != ND_NR_RGBA(11, 22, 33, 255)) { printf("nmdl0 FAIL prim value\n"); return 1; }
    if (w1_of(a_env4) != ND_NR_RGBA(11, 22, 33, 255)) { printf("nmdl0 FAIL env value\n"); return 1; }
    if (w1_of(a_fogc4) != ND_NR_RGBA(11, 22, 33, 0)) { printf("nmdl0 FAIL fog colour value\n"); return 1; }
    if (w1_of(a_fogp4) != ND_NR_FOGPOS(970, 1000)) { printf("nmdl0 FAIL fog position value\n"); return 1; }
    ok += 5;
    /* the tables: every entry, 2 words */
    for (unsigned a = 0; a < 21u; a++) for (unsigned b = 0; b < 5u; b++) {
        const Gfx* o = &old_SolidCombineModes[a][b];
        if (SolidCombineModes[a][b].w0 != o->words.w0 || SolidCombineModes[a][b].w1 != o->words.w1) { printf("nmdl0 FAIL table %u %u\n", a, b); return 1; }
        ncase++;
    }
    static const unsigned dims[][2] = { { 64, 64 }, { 32, 16 }, { 128, 8 }, { 16, 16 }, { 8, 4 } };
    static const unsigned act[][2] = { { 0, 0 }, { 0, 1 }, { 0, 2 }, { 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 }, { 10, 1 } };
    for (unsigned et = 0; et <= 3; et++) for (unsigned mb = 0; mb < 4; mb++) for (unsigned ab = 0; ab < 4; ab++)
    for (unsigned pal = 0; pal < 2; pal++) for (unsigned apal = 0; apal < 2; apal++)
    for (unsigned d = 0; d < 5; d++) for (unsigned c = 0; c < 8; c++) for (unsigned f = 0; f < 3; f++) {
        TextureHeader h;
        memset(&h, 0, sizeof h);
        h.mainW = (u16)dims[d][0]; h.mainH = (u16)dims[d][1];
        h.auxW = (u16)dims[(d + 1) % 5][0]; h.auxH = (u16)dims[(d + 1) % 5][1];
        h.extraTiles = (u8)et;
        h.mainFmt = (u8)(mb == 0 ? G_IM_FMT_CI : (mb == 3 ? G_IM_FMT_RGBA : G_IM_FMT_IA));
        h.auxFmt = (u8)((ab + 1) & 3);
        h.mainBitDepth = (u8)mb; h.auxBitDepth = (u8)ab;
        h.mainWrapW = (u8)(f & 1); h.mainWrapH = (u8)(2 - (f & 1)); h.auxWrapW = (u8)(f ? 1 : 2); h.auxWrapH = (u8)(f ? 2 : 1);
        h.auxCombineType = (u8)act[c][0]; h.auxCombineSubType = (u8)act[c][1];
        h.filtering = (u8)f;
        if (!mtg_case(&h, (int)pal, (int)apal, 3u + f, 5u + f, 17u + c, 0x203u + d)) {
            printf("nmdl0 FAIL mtg et %u mb %u ab %u pal %u apal %u dim %u c %u f %u\n", et, mb, ab, pal, apal, d, c, f);
            return 1;
        }
        ncase++;
    }
    printf("nmdl0 ok %u\n", ok + ncase);
    return 0;
}
