/* nd.h: native draw record and frame driver (plan_native_system 2.2, S1-2). Only the LEGACY bridge kind exists so far. */
#ifndef ND_H
#define ND_H

#include <stdint.h>
#include "pm_gpu.h"

enum {
    ND_DRAW_IDX = 0, /* later rows: indexed static mesh */
    ND_LEGACY = 7    /* bridge only: the walker runs the words of one list span (deleted in S10) */
};

/* 32 bytes, one cache line. The LEGACY fields use the first 8 bytes; the rest is reserved for the draw fields. */
typedef struct NdRec {
    uint8_t kind;
    uint8_t pad[3];
    uint32_t a; /* LEGACY: list address (32-bit ARM11 pointer) */
    uint32_t b; /* LEGACY: byte count */
    uint32_t rsv[5];
} NdRec;
_Static_assert(sizeof(NdRec) == 32, "NdRec is one 32-byte line");

void nd_frame_begin(void);                    /* empties the record array (once per gbi task) */
void nd_bridge_cut(void* list, unsigned nbytes); /* appends one LEGACY record for an N64 list span */
void nd_emit(void);                           /* runs the records in order; LEGACY calls the walker */

/* NDTMPL (S1-3): render state and TEV template per key. The key holds the inputs of flush_draw's state setters and of the TEV builder.
 * Construct it with memset 0 and set the fields; the struct has no padding. pass: 0 main draw, 1 decal pass A, 2 decal pass B, 3 zmark pass 2.
 * The words are what C3Di_UpdateContext writes for the draw (register id, value). Colour words that depend on the PRIM, ENV or plan colours
 * hold 0 in the template (the per-draw uniform block patches them): a stage colour that no colour source sets holds 0xFFFFFFFF. */
typedef struct NdTmplKey {
    uint8_t pass, raw, cull;                          /* raw: graw batch; cull: GPU_CULLMODE (0 when not raw) */
    uint8_t z, zmark, zcmp, zupd, nocol, decal;       /* flush_draw flags */
    uint8_t atest_on, aref;                           /* alpha test: on and ref */
    uint8_t combine, tex, tex1, i8_0, i8_1, rgb_scale, r_mix, draw_3d, rect_2d, dview; /* TevKey fields of pm_gpu.c */
    uint8_t off_active, off_i8, r_memblend, r_replace, r_blend, noise;
    uint8_t two, t1only;                              /* combine 8: pm_comb_build inputs (with w0, w1) */
    uint8_t pad[3];
    uint32_t dbias;                                   /* decal bias, float bits (used when decal is set) */
    uint32_t w0, w1;                                  /* combine 8: the combiner words */
} NdTmplKey;
_Static_assert(sizeof(NdTmplKey) == 44, "NdTmplKey has no padding");
typedef struct NdTmplWord { uint16_t reg; uint32_t val; } NdTmplWord;
#define ND_TMPL_WORDS 64
#define ND_DECAL_EPS 0.003f /* decal pass A depth map offset */

/* returns the template id (0..255) of a key, building it on a miss; -1 when the table is full or the combiner words are refused */
int nd_tmpl_build(const NdTmplKey* k);
/* the register writes of template id; returns the count (ND_TMPL_WORDS max) */
unsigned nd_tmpl_words(int id, const NdTmplWord** out);

/* Render state rules of the walker (pm_gbi.c sync_render): other mode words to the draw flags. depth_img: the colour image is the z buffer image (a model drawn into
 * it is a depth write). set_memb 0: the memory blend flag is not changed by this state. atest -1 = off. */
typedef struct NdRender { uint8_t zcmp, zupd, blend, decal, nocol, memb, set_memb, depth; int16_t atest; } NdRender;
void nd_render_state(uint32_t omode_h, uint32_t omode_l, int depth_img, NdRender* o);

/* TEV stage builder, shared by the template and by pm_gpu.c flush_tev_rebuild. st[i] holds the 5 words of C3D_TexEnv stage i
 * (source, operand, combiner, colour, scale), all 6 stages (unused = C3D_TexEnvInit words). n = stages in use. */
typedef struct NdTevIn {
    const NdTmplKey* k;           /* TEV fields of the key */
    const PmPlan* plan;           /* combine 8 */
    uint32_t prim, cconst, mixcol; /* 0xRRGGBBAA, 0 for a template */
} NdTevIn;
typedef struct NdTevOut {
    uint32_t st[6][5];
    uint32_t buf_color;           /* texenv buffer colour (ABGR), used when buf_set */
    int n, memb, buf_set;
} NdTevOut;
void nd_tev_build(const NdTevIn* in, NdTevOut* out);
static inline uint32_t nd_abgr(uint32_t c) { return ((c & 255u) << 24) | (((c >> 8) & 255u) << 16) | (((c >> 16) & 255u) << 8) | (c >> 24); }

#endif
