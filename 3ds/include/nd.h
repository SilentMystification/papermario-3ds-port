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
 * counting sort by tex1, then tex0, then tmpl (LSD, three passes): the order by (tmpl, tex0, tex1). Returns n. n <= ND_REC_MAX. */
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

/* NDGV (S3N-1): the draw parameters of a raw triangle, moved from pm_gbi.c gv_params_calc. nd_gv_params is pure: the caller reads the walker values into
 * NdGvIn (the flags are the walker's own tests, written out by the caller), so the walker and the native model path call one function. */
typedef struct NdGvP { float uvso[4], vp[4], cm[4], ca[4], zm[2]; int boost; PmFx fx; int fxon; } NdGvP;
typedef struct NdGvIn {
    uint32_t tss;                       /* texture scale word pair (s in the high half) */
    int lighting, texgen, fog;          /* geometry mode bits G_LIGHTING, G_TEXTURE_GEN, G_FOG */
    int comb5, plan_active, plan_vtx_ok, alpha_shade; /* ENV rule, plan rule (vtxname not SHADE or NONE), alpha 255 rule */
    int boost;                          /* the second cycle boost (caller: dual on and the comb2 operands match) */
    int primz; unsigned prim_z;         /* depth source is PRIM (omode bit), the primitive depth word */
    uint32_t envc, plan_vtx;            /* ENV colour, plan vertex colour, RGBA8 words */
    float en[4], vc[4];                 /* the same two colours, unpacked 0..1 (the caller unpacks) */
    int tw, th;                         /* texture size (pm_tex_size, at least 1) */
    unsigned shs, sht;                  /* tile shift s and t of the tile in use */
    int tex_on;
    float vp[4];                        /* viewport scale x y, translate x y */
    int tile_x0, tile_y0; float tile_fx, tile_fy;
    int fogmix, zc_ok;                  /* fogmix: fog mix on and G_FOG; zc_ok: the caller's pm_gpu_gv_zcoef result (only read when fogmix) */
    float za, zb;
    int fog_fm, fog_fo;
    const NdLight* lt; unsigned lt_n; const int8_t (*lk)[3];
} NdGvIn;
void nd_gv_params(const NdGvIn* in, NdGvP* o);
float nd_shift_mul(unsigned n);     /* tile shift to scale: 1 for 0, 1 / 2^n for n up to 10 */
float nd_prim_ndc_z(unsigned prim_z);


/* S3N-1 (B5, B6): a render mode list (ModelRenderModes[i], Gfx pairs ending in G_ENDDL) as a native state. ok 1 only when the list has SETOTHERMODE_H/L ops
 * alone and writes every bit nd_render_state reads. Then r is the render state of the list (depth_img 0). nd_mode_compile returns m->ok. */

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
/* S3N-1 (B4): the texture descriptor of a map model handle, set once when load_texture_impl builds the handle (model.c, TARGET_3DS). S3L-2: the desc is the tile 0 facts for every
 * extra form (mipmaps, aux). Only a handle with no raster, zero size or a 32 bit format stays unset (nd_tex_handle_desc returns NULL: the walker draws them). masks and maskt are INTEGER_LOG2 of the size (the same macro as make_texture_gfx). */
void nd_tex_handle_set(int index, const void* gfx, const void* raster, const void* palette, unsigned w, unsigned h, unsigned fmt, unsigned siz,
                       unsigned wrap_w, unsigned wrap_h, unsigned masks, unsigned maskt, unsigned extra, int has_aux);
/* the descriptor of handle index when its list (gfx) is the one set; NULL otherwise */
const NdTexDesc* nd_tex_handle_desc(int index, const void* gfx);
int nd_tex_handle_miss(int index, const void* gfx); /* S3L-2: fallback code of a handle with no desc (9 form, 10 index or gfx) */

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
    uint32_t om_kh, om_kl;                   /* B8: other mode bits this state has a value for (set by SETOTHERMODE, all after RDPSETOTHERMODE) */
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
/* S6-2a: the texture state of an entity model display list, decoded from its words (pure, host safe). The walker's tile 0 state at the end of the list
 * (SETTIMG, SETTILE tile 0, SETTILESIZE tile 0, LOADBLOCK tile 0, and for a CI format the LOADTLUT of the palette bank on tile 7). Returns a code:
 * NDTX_NONE no texture op; NDTX_OK one bind, native-ready; NDTX_MULTI more binds (the end state is decoded, not native-ready); the others are fallbacks
 * (NDTX_TIMG, NDTX_TILE, NDTX_WIN, NDTX_PAL, NDTX_NOLB, NDTX_NOBIND). Fields are valid for NDTX_OK and NDTX_MULTI only. pal is NULL and pal_n 0 when not CI. */
enum { NDTX_NONE, NDTX_OK, NDTX_TIMG, NDTX_TILE, NDTX_MULTI, NDTX_WIN, NDTX_PAL, NDTX_NOLB, NDTX_NOBIND, NDTX_N };
typedef struct NdTexDec { const void* img; const void* pal; unsigned fmt, siz, w, h, stride, pal_n, cms, cmt, masks, maskt, bad_op; } NdTexDec; /* bad_op: op << 8 | tile of the first NDTX_TILE op (diagnostic) */
int nd_dlc_tex_decode(const uint32_t* l, NdTexDec* o);

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
typedef struct NdMode { uint8_t ok; uint32_t om_h, om_l; uint32_t wh, wl; NdRender r; uint8_t has_cb, has_tx; uint32_t cw0, cw1, tw1, gand, gor; uint32_t bad; uint8_t has_g; } NdMode;
/* B9: has_cb: the list sets the combine (cw0, cw1 = the last G_SETCOMBINE); has_tx: the list sets TEXTURE (tw1 = its w1: s << 16 | t, the walker tss); gand, gor: the geometry ops composed (geom = (geom & gand) | gor); bad: the op code of the first other op (ok 0) */ /* B8: om_h, om_l = the list bits, wh, wl = the bits it writes; r = the list alone (nd_model_state merges) */
int nd_aux_state(const uint32_t* aw, unsigned n, NdMode* ax); /* S3L-3: the R13 words (n Gfx pairs after the record) as a pre-list NdMode: SETOTHERMODE, SETCOMBINE, TEXTURE only, no geometry. 0: another op (fallback) */
int nd_mode_compile(NdMode* m, const unsigned* w);
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

/* S3N-3a: the state of one model record (G_PM_REC, 14 pairs of words, R0 to R13) as a native draw. Pure: the record words and NdMCtx only, no walker global.
 * The caller fills NdMCtx from the tables and from the values the record does not carry (the sticky walker flags, the texture unit, the gv params). Returns 1 when
 * the record draws natively (d and st written), 0 for a fallback (lit, custom gfx, aux words, a refused combiner, another render mode, no combine entry). */
/* S3N-3d: the unit 0 words of block id at draw time, read from its slot (the walker's residency step). 0: the slot no longer holds it (fallback). */
int nd_tex_unit(int id, NdUnit* u);
/* S3N-6: nd_tex_unit with a slot hint (the slot and its generation, pm_tex.c pm_tex_slot_gen): a valid hint gives the words without the content search. hs < 0: none. */
int nd_tex_unit_hint(int id, int* hs, unsigned* hg, NdUnit* u);
unsigned pm_tex_slot_gen(int i); /* S3N-6 (pm_tex.c): the generation of a slot */
typedef struct NdMCtx {
    int zmark; float zn;       /* S5-2 (a): walker r_zmark (the zmark pass: pass 1 model depth and stencil mark, pass 2 constant depth zn * w) and its depth zn */
    int aux_ok;                /* S3L-3: the R13 words are decoded into this context (nd_aux_state, base_oh, comb, tss); 0 with R13 words: fallback */
    const NdMode* mode;        /* compiled render mode list of the record (R0 bits 16 to 23) */
    const NdUnit* u0;          /* texture unit 0 of the draw (nd_tex_get + nd_tex_block), NULL: no texture */
    int i8_0;                  /* unit 0 raster is I8 */
    const NdGvP* gv;           /* uvso, cmul, cadd, zmix of the draw (nd_gv_params) */
    int has_comb; uint32_t comb_w0, comb_w1; /* R4 combine entry (words of the G_SETCOMBINE the record points to) */
    int tex_w, tex_h;          /* S3N-3c: the texture size of unit 0 (pot of the desc w, h, at least 8): the uv scale of the gv params */
    uint32_t tss;              /* S3N-3c: the TEXTURE scale word of the gv params (the list TEXTURE or the walker tss); gv is made per range */
    uint32_t geom_at;          /* S3N-3c: geometry mode of the draw (after the list ops and the group ops of its batch) */
    int geom_at_set;           /* S3N-3c: geom_at is used, else (geom_base & gand) | gor. Cull and z come from this geometry */
    int draw_3d, dview, rgb_scale, r_mix, r_memblend, r_replace; /* sticky walker flags */
    uint32_t prim, env, cconst, mixcol; /* used when the record does not set PRIM (R3 bit 2) */
    uint32_t prim_lod;         /* S3N-3d: nd_st.prim_lod (G_SETPRIMCOLOR w0 low 16 bits; aud_prim_lod is the low byte) */
    uint32_t dbias;            /* decal bias, float bits */
    uint32_t base_oh, base_ol, known_h, known_l; /* B8: the native other mode state (nd_st) and the bits it has a value for */
    uint32_t geom_base;        /* B9: the geometry mode before the list (nd_st.geom) */
    int wind;                  /* walker: the sign of the matrix determinant (gv_wind, until the matrix is native in 3c): cull sense */
} NdMCtx;
typedef struct NdMState { NdTmplKey k; NdTevOut to; int tmpl; int why; } NdMState; /* why: fallback code 1 mode or combine entry, 2 aux words, 3 custom gfx, 4 lit or fog, 5 combiner refused, 6 tex1 or noise, 7 template full, 8 combine needs tile 1, the box product or a key value */
int nd_model_state(const uint32_t* rec, const NdMCtx* c, NdDraw* d, NdMState* st); /* 1 native, 0 fallback, -1 nost (B8: a read other mode bit has no value) */
/* S3N-3c: the static ranges of a shape list (pm_gbi.c pm_dlc_native_ranges): index pool pointer, index count, geometry mode of the batch. */
typedef struct NdRng { const uint16_t* sd; unsigned nidx; uint32_t geom; } NdRng;
int pm_dlc_native_ranges(const void* list, uint32_t entry, NdRng* out, unsigned max); /* -1 the walker would not replay the list statically, -2 out full */
/* S3N-3c: a model record as native draws. c: the record state (geom_at is set per range). list: the geometry list (R12). emit 0: build only (ndmcheck);
 * emit 1: flush, then submit each draw (S3N-4). Returns the draw count, -1 fallback (nd_nat_why), -2 nost. Draws are read with nd_model_native. */
#define ND_NAT_MAX 64
typedef struct NdNat { NdDraw d; NdMState s; float pal[16]; const uint16_t* sd; unsigned nidx; uint8_t zp; float zn; } NdNat; /* S5-2: zp 1 = zmark pass 2, pal row 2 := zn * row 3 (rebuilt on a hit) */
extern int nd_nat_why;
int nd_model_draw(const uint32_t* rec, const NdMCtx* c, const void* list, int emit);
const NdNat* nd_model_native(unsigned j);
/* S3N-6: the native model cache. The key: the 14 record words (R0 to R13) and the walker state the draw reads before the record is applied (pm_gpu.c
 * pm_gpu_model_key). One entry per key: the combined matrix the ranges were cut with (pm_gbi.c combined), the unit 0 slot hint, and the draws (nd_mc_nat, n of them).
 * Open hash, 1024 entries, no division. Full table (3/4) or a full draw pool: clear all (counted by nd_mc_clears). */
#define NDMC_RW 28
#define NDMC_SW 64
typedef struct NdMKey { uint32_t w[NDMC_RW + NDMC_SW]; uint32_t h; uint32_t rp; } NdMKey; /* S3N-7: h = hash of w (nd_mc_hash), rp = the record address (not compared) */
#define ND_RAW_MAX 16
typedef struct __attribute__((aligned(32))) NdMEnt { NdMKey k; NdRng raw[ND_RAW_MAX]; uint32_t geom; int hs; unsigned hg; uint16_t off, n, nraw; uint8_t used, tex, neg, why; uint32_t ep; } NdMEnt; /* raw: the static ranges the draws were cut from (the hit compares them with the fresh ones) */
unsigned nd_handle_gen(void);                                   /* bumped by each texture handle write (nd_tex_handle_set) */
NdMEnt* nd_mc_find(const NdMKey* k);                           /* the entry of key k, NULL when none */
NdMEnt* nd_mc_store(const NdMKey* k, int tex, int hs, unsigned hg); /* the draws and ranges of the last nd_model_draw (it returned >= 0); NULL when more than ND_RAW_MAX ranges */
NdNat* nd_mc_nat(NdMEnt* e, unsigned j);                       /* draw j of an entry (j < e->n) */
NdMEnt* nd_mc_store_neg(const NdMKey* k, unsigned ep, unsigned why); /* S3N-7: a stored fallback (any reason): the record skips the model draw while the texture epoch ep holds. S3L-1: why = the fallback code (NdMState.why) */
uint32_t nd_mc_hash(const NdMKey* k);                          /* S3N-7: the hash of the key words, set in pm_gpu_model_key */
void nd_mc_miss(const NdMKey* k);                              /* S3N-7: the class of one miss (diagnostic counters) */
unsigned nd_mc_size(void);                                     /* S3N-7 diagnostic: table size */
const NdMEnt* nd_mc_ent(unsigned i);                           /* S3N-7 diagnostic: entry i */
void nd_mc_diag_take(unsigned o[15]);                          /* S3N-7: the miss classes of the window (0 new, 1 clr, 2 rec, 3 st, 4 hd, 5 gv, 6 same, 7 rcap, 8 ncap: stores refused), then reset them; table use */
unsigned nd_mc_clears(void);
/* S3N-7: exclusive tick timers of the native model path (pm_gpu.c). Sampled frames only (pm_ph_on). A region's ticks less the regions timed inside it. */
typedef struct NdtT { uint64_t t, all; int on; } NdtT;
enum { NDT_KEYB, NDT_FIND, NDT_RNG, NDT_APPLY, NDT_FB, NDT_UPD, NDT_EMIT, NDT_ADD, NDT_MTX, NDT_N }; /* keyb: record key build; find: cache lookup; apply: the non-hit path (rec_apply, the fallback or the model call) */
unsigned pm_ph_smp_count(void); /* S3N-7: pm_phase.c, sampled frames so far */
NdtT pm_gpu_ndt_begin(void);
void pm_gpu_ndt_end(NdtT s, int k);
unsigned pm_gbi_gvkey(uint32_t* o, unsigned max); /* S3N-6 (pm_gbi.c): the walker gv, tile, vp and plan state (pre-apply) */
void pm_gpu_model_key(const uint32_t* rec, NdMKey* k); /* S3N-6 (pm_gpu.c) */
/* S3N-3d: the combine choice of one draw (pm_gbi.c nd_comb_decide). In: the combine words, the walker flags and the 16 combiner fields of tst1 (comb_a..d, comb_aa..ad,
 * comb2_a..d, comb2_aa..ad). Out: leg (legacy mode, or 8 with plan), use and plan (general plan), cc (constant colour of mode 4), and the walker state (mode5, pat, why, cyc, dual). */
typedef struct NdCombIn {
    uint32_t w0, w1, omode_h, prim, env, aud_prim_lod, key_c, key_s, conv_k4;
    uint32_t tst[16];
    int leg_mode, tex_on, use_t1, t1_valid, dual_on, box_prod, box_pat, box_folded, comb_set, comb_alpha_shade;
} NdCombIn;
typedef struct NdCombOut {
    int leg, mode5, pat, use, why, cyc, dual, plan_on, cc_on, env_on;
    uint32_t cc;
    PmPlan plan;
} NdCombOut;
void nd_comb_decide(const NdCombIn* in, NdCombOut* out);

/* S6-2c-1: an entity list cut at its state ops (segment compile, row S6-2c). A segment is the geometry ops [op0, op1) (VTX, TRI1, TRI2; its triangles are
 * tri0 to tri0 + ntri - 1 in list order) and the state ops [sop0, op0) that come before it, decoded into d (the delta, the nd_mode_compile rules).
 * tex is the tile 0 texture state at op0 (nd_dlc_tex_decode of the list prefix). Sync ops are neutral. A geometry op after a state op starts a segment.
 * Any op outside the known set refuses the list: NDR_OP and *bad = the op code. Slots are tracked over the whole list (the walker keeps them). */
typedef struct NdEntDelta {
    uint32_t wh, wl, vh, vl;    /* SETOTHERMODE_H/L of the delta: written bits and their values */
    uint32_t gand, gor;         /* GEOMETRYMODE ops composed: (geom & gand) | gor */
    uint32_t cw0, cw1;          /* SETCOMBINE (last of the delta) */
    uint32_t prim, prim_lod, env; /* SETPRIMCOLOR, SETENVCOLOR (last of the delta) */
    uint32_t tw0, tw1;          /* TEXTURE (last of the delta) */
    uint8_t has_mode, has_geom, has_comb, has_prim, has_env, has_tex;
    uint8_t n_tex;              /* texture state ops in the delta: SETTIMG, SETTILE, SETTILESIZE, LOADBLOCK, LOADTLUT, LOADTILE */
} NdEntDelta;
typedef struct NdEntSeg {
    NdEntDelta d;
    NdTexDec tex;
    uint32_t sop0, op0, op1;
    uint32_t tri0, ntri;
} NdEntSeg;
#define ND_ENT_MAXSEG 512u
int nd_ent_segments(const uint32_t* l, NdEntSeg* seg, unsigned maxseg, unsigned* nseg, uint32_t* bad);
/* S6-2c-2: the geometry ops (VTX, TRI1, TRI2) of an entity list copied into out (word pairs, G_ENDDL last): the list the static compile takes. Returns the op count of l (the index of its G_ENDDL). */
unsigned nd_ent_strip(const uint32_t* l, uint32_t* out);

#endif
