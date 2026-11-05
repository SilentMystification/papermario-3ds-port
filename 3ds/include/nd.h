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

/* NDTEX (S1-6): texture descriptor to a texture block id. cms, cmt: the G_SETTILE wrap flags (bit 0 mirror, bit 1 clamp), masks, maskt: the tile mask exponents.
 * The tile shift is not a texture unit word: it goes to the uv offset and scale of the draw. */
typedef struct NdTexDesc {
    const void* img;                 /* raster */
    const void* pal;                 /* palette (TLUT) or NULL */
    uint8_t fmt, siz, cms, cmt, masks, maskt;
    uint16_t w, h, pal_n, stride;
    uint8_t pad[2];
} NdTexDesc;
_Static_assert(sizeof(void*) != 4 || sizeof(NdTexDesc) == 24, "NdTexDesc has no padding on the 32 bit target");
uint32_t nd_tex_wrap_of(unsigned cm, unsigned mask);
uint32_t nd_tex_wrap_final(int shr, unsigned mirror, uint32_t wrap);
uint32_t nd_tex_param(uint32_t param, unsigned filter, uint32_t ws, uint32_t wt);
uint32_t nd_tex_dim(unsigned w, unsigned h);
void nd_tex_reset(void);
int nd_tex_get(const NdTexDesc* d);          /* block id, loading the image through pm_tex.c on a miss; -1 when it fails or the table is full */
const uint32_t* nd_tex_block(int id);        /* dim, param, lodParam, data address >> 3 */

/* NDST (S2-1): the native state, set by nd_words() from the words the bridge form of nd_gbi.h decodes. In the bridge nothing draws from it yet (S2-2 starts);
 * it is the state a native draw reads. nd_words never touches the walker state (nd_mv, nd_proj): the N64 word is written too and the walker runs it. */
typedef struct NdSt {
    uint32_t geom;
    uint32_t om_h, om_l;               /* other mode high and low */
    uint32_t comb_w0, comb_w1;         /* G_SETCOMBINE */
    uint32_t prim, env, fog, blend, fill; /* 0xRRGGBBAA */
    uint32_t prim_lod;                 /* prim LOD fraction (m) and min level (l), w0 bits 8 to 15 and 0 to 7 */
    uint16_t primz, primdz;
    uint16_t sc_ulx, sc_uly, sc_lrx, sc_lry; /* 10.2 fixed point as the word holds them */
    uint8_t sc_mode;
    uint8_t tex_on, tex_level, tex_tile;
    uint16_t tex_s, tex_t;
    uint32_t cimg_w0, cimg_w1, zimg_w1;      /* colour image word pair, depth image address */
    uint32_t timg_w0, timg_w1;               /* texture image */
    uint32_t tile_w0[8], tile_w1[8];         /* G_SETTILE and G_SETTILESIZE of the 8 tiles (size words in tsz_*) */
    uint32_t tsz_w0[8], tsz_w1[8];
    uint32_t vp_addr;                        /* G_MOVEMEM viewport source (game pointer value) */
    uint32_t segment[16];
    uint32_t mtx_addr, mtx_flags, n_mtx, n_pop;
    uint32_t n_rect, n_fill, n_tri, n_vtx, n_dl, n_enddl, n_load, n_unknown, last_unknown;
} NdSt;
extern NdSt nd_st;
void nd_st_reset(void);
void nd_words(const unsigned* w, unsigned n); /* n word pairs (Gfx), in list order */

/* NDDLC (S3-1): a static display list (a map shape list) compiled once into an NdObj. A group starts at a G_VTX at slot 0 after triangles, so a group reads only the vertices
 * it loads itself. The compiler keeps the box of the object coordinates per group, the geometry mode ops with the triangle index they apply before, the Vtx copies
 * (flag 0, a palette index 0) and the triangle indices into those copies. Moved from the DLC compile code (pm_gbi.c dlc_compile) and dlc_static_build. Returns a refusal reason, NDR_OK when compiled. */
enum { NDR_OK, NDR_OP, NDR_SLOT, NDR_VTX, NDR_LONG, NDR_FULL }; /* NDR_FULL: the vertex or index pool has no room (the list itself is fine) */
#define ND_DLC_MAXOPS 4096u
#define ND_DLC_MAXGRP 1024u
#define ND_DLC_MAXGOP 2048u
typedef struct NdGrp {
    int16_t lo[3], hi[3];       /* box of the object coordinates of the vertices the group loads */
    uint16_t nvtx, ntri;        /* vertices loaded, triangles kept (a triangle on a dead slot is dropped) */
    uint16_t ng;                /* geometry mode ops inside the group */
    uint32_t op0, op1;          /* op range of the group in the source list */
    uint32_t am, om;            /* geometry mode and-mask and or-value at the group start, relative to the list entry */
    uint32_t vbase, ibase, gbase; /* first vertex in obj->vtx, first index in obj->idx, first geometry op in obj->gop */
    uint8_t tsv_exit;           /* the last op of the group was a triangle */
} NdGrp;
typedef struct NdObj {
    uint32_t nops, exit_am, exit_om;
    uint16_t ngrp;
    NdGrp* grp;
    uint32_t (*gop)[2];         /* per geometry op: and-mask, or-value */
    uint16_t* gat;              /* per geometry op: the triangle index (inside its group) it applies before */
    uint32_t ngop;
    uint8_t* vtx;               /* 16 byte Vtx copies, flag word 0 */
    uint32_t nvtx;
    uint16_t* idx;              /* triangle corner indices (3 per triangle): vertex numbers of the pool, vfirst + the copy number */
    uint32_t nidx;
    unsigned vfirst;            /* vertex number of vtx[0] in the vertex pool (0 for a malloc object) */
    uint8_t pooled;             /* vtx and idx belong to the pools, nd_dlc_free does not free them */
} NdObj;
typedef struct NdDlcEnv {
    const void* (*vtx)(uint32_t w1, unsigned n); /* the n Vtx (16 bytes, host order) at the list address w1, or NULL when it is not shape data */
    uint8_t* (*valloc)(unsigned nvtx, unsigned* first); /* optional: room for nvtx Vtx copies in the vertex pool, first = its first vertex number; NULL return refuses */
    uint16_t* (*ialloc)(unsigned nidx);                 /* optional: room for nidx indices in the index pool; NULL return refuses (both NULL: malloc) */
} NdDlcEnv;
int nd_dlc_compile(NdObj* o, const uint32_t* list, const NdDlcEnv* env); /* list: word pairs in host order, up to G_ENDDL */
void nd_dlc_free(NdObj* o);

/* NDEMIT (S3 M3): the command words of one raw draw (pm4 program: static or ring Vtx with the palette matrix at entry 0), written from native data. The PICA command format is
 * [parameter][header][extra parameters][pad]: header bits 0-15 register, 16-19 byte mask, 20-27 extra count, bit 31 consecutive registers. Only the parts that differ from
 * the shadow (what the GPU holds after the last emitted draw) are written; a shadow reset writes everything. The vertex shader program, the attribute and buffer
 * configuration and the frame state stay with citro3d (set up once before the first native draw). Verified against the real command stream (nd_emit_test). */
typedef struct NdUnit { uint32_t dim, param, lod, addr, type, force; } NdUnit; /* dim 0: unit off. addr = physical address >> 3, type = the texture format word. force: write the block and clear the texture cache even when the words equal the shadow (another texture or new texels under the same address, as a citro3d rebind) */
typedef struct NdDraw {
    int tmpl;                 /* nd_tmpl id: effect, depth, blend and TEV words */
    uint32_t tev_col[6];      /* the colour word of each TEV stage that a colour source sets (the template holds 0 there), ABGR */
    uint32_t buf_col; uint8_t has_buf; /* texenv buffer colour (register 0xFD), written when has_buf */
    NdUnit u[2];              /* texture units 0 and 1 */
    const float (*pal)[16];   /* palette entries, pcomp each, C3D_Mtx memory order */
    unsigned npal;
    unsigned loc_pal, loc_var; /* vertex shader float uniform locations: palette entry 0 (entries 4 apart), then uvso, cmul, cadd, zmix (4 vec4 in a row) */
    float uvso[4], cmul[4], cadd[4], zmix[2]; /* game order x y z w, the words go out in memory order w z y x */
    uint32_t idx_word;        /* index buffer address word (register 0x227) */
    uint32_t nidx;            /* index count */
} NdDraw;
#define ND_NPAL 21
typedef struct NdShadow {
    int tmpl;                 /* -1 = unknown */
    NdUnit u[2]; uint8_t u_on[2], cfg_valid; uint32_t cfg;
    float pal[ND_NPAL][16]; uint8_t pal_valid[ND_NPAL];
    float uni[16]; uint8_t uni_valid;
    uint32_t tmpl_val[64]; /* ND_TMPL_WORDS max, register values of the template last written (for word level deltas) */
    uint32_t col[6], buf_col; uint8_t col_valid, has_buf;
} NdShadow;
void nd_shadow_reset(NdShadow* s);
unsigned nd_emit_draw(uint32_t* out, NdShadow* sh, const NdDraw* d); /* returns the number of words written (even) */

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
/* the most words nd_emit_draw writes for one draw: the template (2 words a register), 2 texture units, 21 palette entries (20 words each), the uniforms, the draw tail. nd_emit_test proves it. */
#define ND_EMIT_MAX 768
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
