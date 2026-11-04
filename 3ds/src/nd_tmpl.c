/* nd_tmpl.c: NDTMPL render state template per key (plan_native_slices S1-3).
 * The words are the register writes of the flush_draw setters in pm_gpu.c (sh_cull, sh_depth_map, sh_alpha_test,
 * sh_blend, sh_depth_test). Table: 256 entries, open hash with a 512 slot index. Not used by a draw yet (S3). */
#include <stdint.h>
#include <string.h>
#include "nd.h"

uint32_t f32tof24(float f); /* libctru gpu.h */

/* register numbers (libctru gpu/registers.h) */
#define R_CULL 0x040
#define R_DM_SCALE 0x04D
#define R_DM_OFFSET 0x04E
#define R_DM_ENABLE 0x06D
#define R_BLEND_FUNC 0x101
#define R_ALPHA_TEST 0x104
#define R_DEPTH_COLOR_MASK 0x107

/* GPU enums (libctru gpu/enums.h) */
#define G_ALWAYS 1u
#define G_GREATER 6u
#define G_GEQUAL 7u
#define G_WRITE_DEPTH 0x10u
#define G_WRITE_COLOR 0x0Fu
#define G_WRITE_ALL 0x1Fu
#define G_ZERO 0u
#define G_ONE 1u
#define G_SRC_ALPHA 6u
#define G_ONE_MINUS_SRC_ALPHA 7u

#define ND_TMPL_N 256u
#define ND_TMPL_IDX 512u /* open hash index, power of two */

typedef struct { NdTmplKey key; unsigned n; NdTmplWord w[ND_TMPL_WORDS]; } NdTmplEnt;
static NdTmplEnt tmpl_ent[ND_TMPL_N];
static uint16_t tmpl_idx[ND_TMPL_IDX]; /* entry id + 1, 0 = empty */
static unsigned tmpl_n;

static unsigned key_hash(const NdTmplKey* k) {
    unsigned h = 2166136261u;
    unsigned char b[10] = { k->cull, k->z, k->zmark, k->zcmp, k->zupd, k->nocol, k->decal, k->atest_on, k->aref, k->blend };
    for (unsigned i = 0; i < 10; i++) h = (h ^ b[i]) * 16777619u;
    return (h ^ k->dbias) * 16777619u;
}

/* compares the fields only: the struct has padding */
static int key_eq(const NdTmplKey* a, const NdTmplKey* b) {
    return a->cull == b->cull && a->z == b->z && a->zmark == b->zmark && a->zcmp == b->zcmp && a->zupd == b->zupd &&
           a->nocol == b->nocol && a->decal == b->decal && a->atest_on == b->atest_on && a->aref == b->aref &&
           a->blend == b->blend && a->dbias == b->dbias;
}

/* the depth test that flush_draw sets for these flags (stencil pass of zmark not included) */
static void depth_of(const NdTmplKey* k, unsigned* en, unsigned* func, unsigned* mask) {
    if (k->z && k->zmark) { *en = 1; *func = G_GREATER; *mask = G_WRITE_DEPTH; }
    else if (k->z && k->zcmp) { *en = 1; *func = k->decal ? G_GEQUAL : G_GREATER; *mask = k->nocol ? G_WRITE_DEPTH : k->zupd ? G_WRITE_ALL : G_WRITE_COLOR; }
    else if (k->z && k->zupd) { *en = 1; *func = G_ALWAYS; *mask = k->nocol ? G_WRITE_DEPTH : G_WRITE_ALL; }
    else { *en = 0; *func = G_GREATER; *mask = G_WRITE_COLOR; }
}

static void build_words(NdTmplEnt* e) {
    const NdTmplKey* k = &e->key;
    unsigned en, func, mask, n = 0;
    depth_of(k, &en, &func, &mask);
    e->w[n].reg = R_CULL; e->w[n].val = k->cull; n++;
    e->w[n].reg = R_DM_SCALE; e->w[n].val = f32tof24(-1.0f); n++;
    float bias = 0.0f;
    if (k->decal) memcpy(&bias, &k->dbias, 4);
    e->w[n].reg = R_DM_OFFSET; e->w[n].val = f32tof24(bias); n++;
    e->w[n].reg = R_DM_ENABLE; e->w[n].val = 1; n++;
    e->w[n].reg = R_BLEND_FUNC; e->w[n].val = k->blend ? (G_SRC_ALPHA << 16 | G_ONE_MINUS_SRC_ALPHA << 20 | G_SRC_ALPHA << 24 | G_ONE_MINUS_SRC_ALPHA << 28)
                                                       : (G_ONE << 16 | G_ZERO << 20 | G_ONE << 24 | G_ZERO << 28); n++;
    e->w[n].reg = R_ALPHA_TEST; e->w[n].val = k->atest_on ? (1u | G_GREATER << 4 | (uint32_t)k->aref << 8) : (G_ALWAYS << 4); n++;
    e->w[n].reg = R_DEPTH_COLOR_MASK; e->w[n].val = en | func << 4 | mask << 8; n++;
    e->n = n;
}

int nd_tmpl_build(const NdTmplKey* in) {
    NdTmplKey kk = *in;
    if (!kk.decal) kk.dbias = 0; /* the bias is not used without decal: one key for both */
    unsigned h = key_hash(&kk) & (ND_TMPL_IDX - 1u);
    for (;;) {
        uint16_t s = tmpl_idx[h];
        if (s == 0) break;
        if (key_eq(&tmpl_ent[s - 1u].key, &kk)) return s - 1;
        h = (h + 1u) & (ND_TMPL_IDX - 1u);
    }
    /* ponytail: table full returns -1, the caller falls back to the walker. Add eviction when S3 fills more than 256 keys. */
    if (tmpl_n >= ND_TMPL_N) return -1;
    NdTmplEnt* e = &tmpl_ent[tmpl_n];
    e->key = kk;
    build_words(e);
    tmpl_idx[h] = (uint16_t)(tmpl_n + 1u);
    return (int)tmpl_n++;
}

unsigned nd_tmpl_words(int id, const NdTmplWord** out) {
    *out = tmpl_ent[id].w;
    return tmpl_ent[id].n;
}
