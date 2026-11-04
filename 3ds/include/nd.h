/* nd.h: native draw record, frame driver, matrices (plan_native_system 2.2 and 2.5, S1-2 and S1-4). Only the LEGACY bridge kind is emitted so far. */
#ifndef ND_H
#define ND_H

#include <stdint.h>
#include "pm_gpu.h"

#ifdef __arm__
#define ND_HOTD __attribute__((section(".data.pmhot"))) /* same section as PM_HOTD in pm_port.h */
#else
#define ND_HOTD
#endif

enum {
    ND_DRAW_IDX = 0, /* later rows: indexed static mesh */
    ND_LEGACY = 7    /* bridge only: the walker runs the words of one list span (deleted in S10) */
};

/* 32 bytes, one cache line (plan 2.2). LEGACY: aux = list address (32-bit ARM11 pointer), aux2 = byte count. */
typedef struct NdRec {
    uint8_t kind;
    uint8_t tmpl;     /* template id */
    uint8_t tex0;     /* texture unit 0 block id (0 = none) */
    uint8_t tex1;     /* texture unit 1 block id (0 = none) */
    uint16_t mtx;     /* matrix slot in the frame matrix pool */
    uint16_t unif;    /* per-draw uniform block id (0 = the template default) */
    uint32_t idx_word; /* index buffer address, GPU register form */
    uint16_t nidx;    /* index count */
    uint16_t vbuf;    /* attribute buffer id (0 = dynamic ring, 1 = static pool) */
    uint32_t sort;    /* tmpl << 24, tex0 << 16, tex1 << 8, layer */
    uint32_t seq;     /* game order index. Bit 31 = ordered (blend, decal or depth off: keep game order) */
    uint32_t aux;     /* rectangle corners, or the LEGACY list address */
    uint32_t aux2;    /* second rectangle word, the light block id, or the LEGACY byte count */
} NdRec;
_Static_assert(sizeof(NdRec) == 32, "NdRec is one 32-byte line");
#define ND_SEQ_ORDERED 0x80000000u
#define ND_REC_MAX 2048u

void nd_frame_begin(void);                    /* empties the record array (once per gbi task) */
void nd_bridge_cut(void* list, unsigned nbytes); /* appends one LEGACY record for an N64 list span */
void nd_emit(void);                           /* runs the records in order; LEGACY calls the walker */

/* Draw order for n records into ord (indices). A record with ND_SEQ_ORDERED keeps its place and breaks runs. Inside each run of unordered records a stable
 * counting sort by tex0, then by tmpl (LSD, two passes). Returns n. n <= ND_REC_MAX. */
unsigned nd_order(const NdRec* rec, unsigned n, uint16_t* ord);

/* NDMTX (S1-4). Walker matrix stack (the RSP depth is 10), projection, N64 unpack, per-eye matrix pool. */
#define ND_MV_DEPTH 10
#define ND_MTX_MAX 512
#define ND_MTX_PUSH 1u /* values of G_MTX_PUSH, G_MTX_LOAD, G_MTX_PROJECTION */
#define ND_MTX_LOAD 2u
#define ND_MTX_PROJ 4u
extern float nd_mv[ND_MV_DEPTH][4][4];
extern int nd_mv_sp;
extern float nd_proj[4][4];
extern float nd_mtx_pool[2][ND_MTX_MAX][16]; /* matrix slot per eye, C3D_Mtx memory order, 64 bytes, aligned 32 */
void nd_mtx_ident(float m[4][4]);
void nd_mtx_mul(float c[4][4], const float a[4][4], const float b[4][4]);
void nd_mtx_unpack(const uint32_t* n64, float d[4][4]);      /* 16 words of an N64 Mtx */
void nd_mtx_reset(void);                                     /* projection and stack bottom to identity, stack empty */
void nd_mtx_load(const float in[4][4], unsigned flags);      /* push (when flagged), then load or premultiply into the top or the projection */
void nd_mtx_pop(unsigned count);                             /* count 0 pops one */
void nd_mtx_combine(float c[4][4], float e, float conv);     /* modelview x projection with the eye shift e (0 = no shift) */
void nd_mtx_cmul(float o[16], const float a[16], const float b[16]); /* citro3d Mtx_Multiply */
void nd_mtx_pcomp(float o[16], const float post[16], const float game[4][4]); /* pcomp = post x game */

/* S1-5 (fps60): during the t = 0.5 pass (pm_interp_repl) a G_MTX loads the interpolated replacement of the game matrix at src (pm_interp_find) instead of the N64 words.
 * Returns 1 and fills in[4][4] when a replacement exists, else 0 (the caller unpacks the N64 words). */
int nd_mtx_replace(const void* src, float in[4][4]);

/* Light block (the pm5 vertex shader layout, PmFx): lflag[0] = lit, lflag[1] = texgen, amb = slot n, col and dir of up to 7 lights (dir / 127), look-at vectors lk (/ 127).
 * lt: the RSP light table (slot n is the ambient light), n lights. Fog is not set here. */
typedef struct NdLight { uint8_t col[3]; int8_t dir[3]; } NdLight;
void nd_lights(PmFx* fx, int texgen, const NdLight* lt, unsigned n, const int8_t lk[2][3]);

/* Eye 2 without a second walk: copy the eye 1 command span, then write the eye 2 matrix words at each logged position. */
typedef struct NdMtxLog { uint32_t at; uint16_t slot; uint16_t pad; } NdMtxLog;
void nd_eye_copy(uint32_t* dst, const uint32_t* src, unsigned nw, const NdMtxLog* log, unsigned nlog, unsigned eye);

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
