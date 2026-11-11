/* nd_op.h: the typed op (HC4, plan_hotcode section 5.1). One 8-byte unit as the frame stream unit: w0 = id << 24 | a (24 bits), w1 = b.
 * The ids are 0x20 + k (k < 32), free of the F3DEX2 ops (0x01-0x07, 0xd7-0xff) and the G_PM_* ids (0x0d-0x13). The recorder (core 1) writes
 * typed ops into the NDC_STATE runs (nd_op_xlate); core 0 runs them by id (nd_set.c nd_op_same, nd_op_apply) and decodes no word for them.
 * HC4a: the scalar state ops. HC4b: the combine, the end of a typed table, the dropped-op mark of the t4chk check. HC4c: the texture image, tile
 * and load ops (a and b keep the F3DEX2 field layout: a = w0 low 24 bits, b = w1; core 0 reads the fields by id, nd_tex_set.c). Host-built (nd_cmd_test.c). */
#ifndef ND_OP_H
#define ND_OP_H
#include <stdint.h>
#include "ultra64.h" /* the F3DEX2 ids of the nd_op_n64 table */

enum {
    NDO_BASE = 0x20,
    NDO_OMH = 0x20,   /* G_SETOTHERMODE_H: a = shift | length << 8, b = value */
    NDO_OML,          /* G_SETOTHERMODE_L: as NDO_OMH */
    NDO_GEOM,         /* G_GEOMETRYMODE: a = the clear mask (w0 low 24 bits), b = the set bits */
    NDO_PRIM,         /* G_SETPRIMCOLOR: a = the LOD word (16 bits), b = colour */
    NDO_ENV,          /* G_SETENVCOLOR: b = colour */
    NDO_FOG,          /* G_SETFOGCOLOR: b = colour */
    NDO_BLEND,        /* G_SETBLENDCOLOR: b = colour */
    NDO_FILL,         /* G_SETFILLCOLOR: b = colour */
    NDO_PRIMZ,        /* G_SETPRIMDEPTH: b = z << 16 | dz */
    NDO_SCISSOR,      /* G_SETSCISSOR: a = ulx << 12 | uly, b = lrx << 12 | lry (10.2 fields; the wide sign rule runs on core 0) */
    NDO_TEXTURE,      /* G_TEXTURE: a = on (0 or 1), b = ss << 16 | ts */
    NDO_MW,           /* G_MOVEWORD: a = kind << 16 | arg (NDO_MW_*), b = value */
    NDO_RARE,         /* G_SETCONVERT, G_SETKEYR, G_SETKEYGB: a = NDO_RARE_*, b = w1 */
    NDO_COMB,         /* G_SETCOMBINE: a = w0 low 24 bits, b = w1 (the walker keeps the full word: NDO_COMB_W0) */
    NDO_END,          /* the end of a typed table (render mode table); never in a stream run */
    NDO_DROP,         /* t4chk (debug switch): the next op is one the recorder shadow dropped (nd_rec.c); core 0 checks it */
    NDO_TIMG,         /* G_SETTIMG: a = fmt << 21 | siz << 19 | (width - 1), b = image address (a segment address stays a value) */
    NDO_TILE,         /* G_SETTILE: a = fmt << 21 | siz << 19 | line << 9 | tmem, b = tile << 24 | palette << 20 | cmt, maskt, shiftt, cms, masks, shifts */
    NDO_TILESIZE,     /* G_SETTILESIZE: a = uls << 12 | ult, b = tile << 24 | lrs << 12 | lrt (10.2 fields) */
    NDO_LOADBLOCK,    /* G_LOADBLOCK: a = uls << 12 | ult, b = tile << 24 | texels - 1 << 12 | dxt */
    NDO_LOADTILE,     /* G_LOADTILE: as NDO_TILESIZE */
    NDO_LOADTLUT,     /* G_LOADTLUT: a = 0, b = tile << 24 | (count - 1) << 14 */
    NDO_VTX,          /* HC4d, from here the geometry and matrix ops (nd_op_geo): G_VTX: a = n << 12 | (v0 + n) << 1, b = vertex address */
    NDO_TRI1,         /* G_TRI1: a = v0 * 2 << 16 | v1 * 2 << 8 | v2 * 2 */
    NDO_TRI2,         /* G_TRI2: a = the first triangle, b = the second (as NDO_TRI1 a) */
    NDO_MTX,          /* G_MTX: a = the w0 low 24 bits (flags in the low byte), b = matrix address */
    NDO_POPMTX,       /* G_POPMTX: b = bytes (64 per matrix) */
    NDO_MOVEMEM,      /* G_MOVEMEM: a = the w0 low 24 bits (offset << 8 | index), b = address */
    NDO_CALL,         /* HC4d2, from here the control ops (nd_op_ctl): G_DL: a = 1 for the branch form (G_DL_NOPUSH), b = list address */
    NDO_RECT,         /* G_FILLRECT, G_TEXRECT(FLIP): a = lrx << 12 | lry, b = NDO_RECT_TEX | tile << 24 | ulx << 12 | uly (10.2 fields).
                       * A texture rectangle keeps its two RDPHALF units after it (their b: s, t and dsdx, dtdy) */
    NDO_MISC,         /* a = NDO_MISC_* << 16 | arg: the list start of an in-line state list (G_PM_LST), the native-only payload (G_PM_ND,
                       * arg = kind << 8 | on), G_MODIFYVTX (arg = where << 8 | vertex, vertex 255 when above), G_SETCIMG (arg = siz << 12 |
                       * width - 1, b = image address), no work (G_NOOP, G_RDPHALF_*, G_SETZIMG) */
    NDO_END_
};
enum { NDO_MW_NONE, NDO_MW_NUMLIGHT, NDO_MW_LIGHTCOL /* arg = light k */, NDO_MW_FOG, NDO_MW_SEG /* arg = segment */ };
enum { NDO_RARE_CONV, NDO_RARE_KEYR, NDO_RARE_KEYGB };
enum { NDO_MISC_NOP, NDO_MISC_LST, NDO_MISC_ND, NDO_MISC_MODVTX, NDO_MISC_CIMG };
#define NDO_RECT_TEX 0x80000000u /* NDO_RECT b: a texture rectangle (else a fill) */

#define NDO_W0(id, a) (((uint32_t)(id) << 24) | ((uint32_t)(a) & 0xffffffu))
#define NDO_COMB_W0(a) ((uint32_t)0xfcu << 24 | ((uint32_t)(a) & 0xffffffu)) /* the G_SETCOMBINE word of a typed combine (nd_ws.comb_w0, model keys) */
#define NDO_A_RENDERMODE (3u | 29u << 8) /* the a field of gDPSetRenderMode (G_MDSFT_RENDERMODE 3, 29 bits) */
/* HC4e1: the typed unit of an F3DEX2 word as an integer constant expression (static initializers, the gs* macros). ND_X0 is the a and id
 * word (w0), ND_X1 the b word (w1); both mirror nd_op_xlate (nd_op.c) arm for arm in the same order. A typed or unknown word stays as it is.
 * ND_X1 lists only the arms whose b is not w1. No division: MOVEWORD offsets by a constant chain or a shift, MODIFYVTX by a shift. */
#define ND_OPB(w0) (((uint32_t)(w0) >> 24) & 0xffu)
#define ND_OTH_LEN(w0) (((uint32_t)(w0) & 0xffu) + 1u)
#define ND_OTH_FLD(w0) (((uint32_t)(w0) >> 8) & 0xffu)
#define ND_OTH_A(w0) ((((32u - ND_OTH_FLD(w0) - ND_OTH_LEN(w0)) & 0xffu)) | (ND_OTH_LEN(w0) << 8))
#define ND_MW_IDX(w0) (((uint32_t)(w0) >> 16) & 0xffu)
#define ND_MW_OFF(w0) ((uint32_t)(w0) & 0xffffu)
#define ND_MW_LCOK(o) ((o) == 0u || (o) == 24u || (o) == 48u || (o) == 72u || (o) == 96u || (o) == 120u || (o) == 144u || (o) == 168u || (o) == 192u)
#define ND_MW_LCA(o) ((o) == 0u ? 0u : (o) == 24u ? 1u : (o) == 48u ? 2u : (o) == 72u ? 3u : (o) == 96u ? 4u : (o) == 120u ? 5u : \
    (o) == 144u ? 6u : (o) == 168u ? 7u : 8u)
#define ND_MW_IS_LC(w0) (ND_MW_IDX(w0) == G_MW_LIGHTCOL && ND_MW_LCOK(ND_MW_OFF(w0)))
#define ND_MW_IS_SEG(w0) (ND_MW_IDX(w0) == G_MW_SEGMENT && (ND_MW_OFF(w0) >> 2) < 16u)
#define ND_MW_KIND(w0) (ND_MW_IDX(w0) == G_MW_NUMLIGHT ? (unsigned)NDO_MW_NUMLIGHT : ND_MW_IS_LC(w0) ? (unsigned)NDO_MW_LIGHTCOL : \
    ND_MW_IDX(w0) == G_MW_FOG ? (unsigned)NDO_MW_FOG : ND_MW_IS_SEG(w0) ? (unsigned)NDO_MW_SEG : (unsigned)NDO_MW_NONE)
#define ND_MW_ARG(w0) (ND_MW_IS_LC(w0) ? ND_MW_LCA(ND_MW_OFF(w0)) : ND_MW_IS_SEG(w0) ? (ND_MW_OFF(w0) >> 2) : 0u)
#define ND_MW_A(w0) NDO_W0(NDO_MW, ((ND_MW_KIND(w0) << 16) | ND_MW_ARG(w0)))
#define ND_MV_VI(w0) (ND_MW_OFF(w0) >> 1)
#define ND_MV_A(w0) NDO_W0(NDO_MISC, (NDO_MISC_MODVTX << 16) | (((uint32_t)(w0) >> 8) & 0xff00u) | (ND_MV_VI(w0) < 255u ? ND_MV_VI(w0) : 255u))
#define ND_NOP_A NDO_W0(NDO_MISC, NDO_MISC_NOP << 16)
#define ND_IS_SYNC(w0) ((ND_OPB(w0) >= G_RDPLOADSYNC && ND_OPB(w0) <= G_RDPFULLSYNC) || ND_OPB(w0) == G_SPNOOP)
#define ND_X0(w0, w1) (ND_OPB(w0) == G_SETOTHERMODE_H ? NDO_W0(NDO_OMH, ND_OTH_A(w0)) : \
    ND_OPB(w0) == G_SETOTHERMODE_L ? NDO_W0(NDO_OML, ND_OTH_A(w0)) : \
    ND_OPB(w0) == G_GEOMETRYMODE ? NDO_W0(NDO_GEOM, w0) : \
    ND_OPB(w0) == G_SETPRIMCOLOR ? NDO_W0(NDO_PRIM, (uint32_t)(w0) & 0xffffu) : \
    ND_OPB(w0) == G_SETENVCOLOR ? NDO_W0(NDO_ENV, 0u) : \
    ND_OPB(w0) == G_SETFOGCOLOR ? NDO_W0(NDO_FOG, 0u) : \
    ND_OPB(w0) == G_SETBLENDCOLOR ? NDO_W0(NDO_BLEND, 0u) : \
    ND_OPB(w0) == G_SETFILLCOLOR ? NDO_W0(NDO_FILL, 0u) : \
    ND_OPB(w0) == G_SETPRIMDEPTH ? NDO_W0(NDO_PRIMZ, 0u) : \
    ND_OPB(w0) == G_SETSCISSOR ? NDO_W0(NDO_SCISSOR, w0) : \
    ND_OPB(w0) == G_TEXTURE ? NDO_W0(NDO_TEXTURE, ((uint32_t)(w0) & 0xffu) != 0u) : \
    ND_OPB(w0) == G_MOVEWORD ? ND_MW_A(w0) : \
    ND_OPB(w0) == G_SETCONVERT ? NDO_W0(NDO_RARE, NDO_RARE_CONV) : \
    ND_OPB(w0) == G_SETKEYR ? NDO_W0(NDO_RARE, NDO_RARE_KEYR) : \
    ND_OPB(w0) == G_SETKEYGB ? NDO_W0(NDO_RARE, NDO_RARE_KEYGB) : \
    ND_OPB(w0) == G_SETCOMBINE ? NDO_W0(NDO_COMB, w0) : \
    ND_OPB(w0) == G_SETTIMG ? NDO_W0(NDO_TIMG, w0) : \
    ND_OPB(w0) == G_SETTILE ? NDO_W0(NDO_TILE, w0) : \
    ND_OPB(w0) == G_SETTILESIZE ? NDO_W0(NDO_TILESIZE, w0) : \
    ND_OPB(w0) == G_LOADBLOCK ? NDO_W0(NDO_LOADBLOCK, w0) : \
    ND_OPB(w0) == G_LOADTILE ? NDO_W0(NDO_LOADTILE, w0) : \
    ND_OPB(w0) == G_LOADTLUT ? NDO_W0(NDO_LOADTLUT, w0) : \
    ND_OPB(w0) == G_VTX ? NDO_W0(NDO_VTX, w0) : \
    ND_OPB(w0) == G_TRI1 ? NDO_W0(NDO_TRI1, w0) : \
    ND_OPB(w0) == G_TRI2 ? NDO_W0(NDO_TRI2, w0) : \
    ND_OPB(w0) == G_MTX ? NDO_W0(NDO_MTX, w0) : \
    ND_OPB(w0) == G_POPMTX ? NDO_W0(NDO_POPMTX, w0) : \
    ND_OPB(w0) == G_MOVEMEM ? NDO_W0(NDO_MOVEMEM, w0) : \
    ND_OPB(w0) == G_DL ? NDO_W0(NDO_CALL, ((((uint32_t)(w0) >> 16) & 0xffu) == G_DL_NOPUSH)) : \
    ND_OPB(w0) == G_FILLRECT ? NDO_W0(NDO_RECT, w0) : \
    (ND_OPB(w0) == G_TEXRECT || ND_OPB(w0) == G_TEXRECTFLIP) ? NDO_W0(NDO_RECT, w0) : \
    ND_OPB(w0) == G_PM_LST ? NDO_W0(NDO_MISC, NDO_MISC_LST << 16) : \
    ND_OPB(w0) == G_PM_ND ? NDO_W0(NDO_MISC, NDO_MISC_ND << 16 | (((uint32_t)(w0) >> 8) & 0xff00u) | ((uint32_t)(w0) & 0xffu)) : \
    ND_OPB(w0) == G_MODIFYVTX ? ND_MV_A(w0) : \
    ND_OPB(w0) == G_SETCIMG ? NDO_W0(NDO_MISC, NDO_MISC_CIMG << 16 | (((uint32_t)(w0) >> 7) & 0x3000u) | ((uint32_t)(w0) & 0xfffu)) : \
    (ND_OPB(w0) == G_NOOP || ND_OPB(w0) == G_RDPHALF_1 || ND_OPB(w0) == G_RDPHALF_2 || ND_OPB(w0) == G_SETZIMG) ? ND_NOP_A : \
    ND_IS_SYNC(w0) ? ND_NOP_A : \
    ND_OPB(w0) == G_ENDDL ? NDO_W0(NDO_END, 0u) : \
    (uint32_t)(w0))
#define ND_X1(w0, w1) (ND_OPB(w0) == G_SETSCISSOR ? ((uint32_t)(w1) & 0xffffffu) : \
    ND_OPB(w0) == G_FILLRECT ? ((uint32_t)(w1) & 0x00ffffffu) : \
    (ND_OPB(w0) == G_TEXRECT || ND_OPB(w0) == G_TEXRECTFLIP) ? (NDO_RECT_TEX | ((uint32_t)(w1) & 0x07ffffffu)) : \
    (ND_OPB(w0) == G_PM_LST || ND_OPB(w0) == G_PM_ND || ND_OPB(w0) == G_ENDDL) ? 0u : \
    (uint32_t)(w1))
static inline int nd_op_typed(unsigned id) { return id - (unsigned)NDO_BASE < 32u; }
static inline int nd_op_geo(unsigned id) { return id - (unsigned)NDO_VTX <= (unsigned)(NDO_MOVEMEM - NDO_VTX); } /* HC4d: no skip test, no op switch of nd_set.c */
static inline int nd_op_ctl(unsigned id) { return id >= (unsigned)NDO_CALL && id < (unsigned)NDO_END_; } /* HC4d2: a list call, rectangle or misc op */
static inline int nd_op_tex(unsigned id) { return id - (unsigned)NDO_TIMG <= (unsigned)(NDO_LOADTLUT - NDO_TIMG); } /* HC4c: an image, tile or load op */
/* The mask of an othermode op from its a field (shift, length), as the walker computed it from the word. */
static inline uint32_t nd_op_om_mask(uint32_t a) {
    const unsigned sft = a & 0xffu, len = (a >> 8) & 0xffu;
    return len >= 32u ? 0xffffffffu : (((1u << len) - 1u) << sft);
}
/* HC4d2: typed units written by a producer (no word built). */
#define ND_OP_VTX_W0(n, v0) NDO_W0(NDO_VTX, (uint32_t)(n) << 12 | ((uint32_t)(v0) + (uint32_t)(n)) << 1)
#define ND_OP_TRI_A(v0, v1, v2) ((uint32_t)(v0) * 2u << 16 | (uint32_t)(v1) * 2u << 8 | (uint32_t)(v2) * 2u)
#define ND_OP_CALL_W0 NDO_W0(NDO_CALL, 0)
/* The F3DEX2 op of each typed id (pm_gbi_cur_op, census). */
static const uint8_t nd_op_n64[NDO_END_ - NDO_BASE] = {
    G_SETOTHERMODE_H, G_SETOTHERMODE_L, G_GEOMETRYMODE, G_SETPRIMCOLOR, G_SETENVCOLOR, G_SETFOGCOLOR, G_SETBLENDCOLOR,
    G_SETFILLCOLOR, G_SETPRIMDEPTH, G_SETSCISSOR, G_TEXTURE, G_MOVEWORD, 0 /* NDO_RARE: the sub-op names it */, G_SETCOMBINE,
    G_ENDDL /* NDO_END */, G_SPNOOP /* NDO_DROP */, G_SETTIMG, G_SETTILE, G_SETTILESIZE, G_LOADBLOCK, G_LOADTILE, G_LOADTLUT,
    G_VTX, G_TRI1, G_TRI2, G_MTX, G_POPMTX, G_MOVEMEM, G_DL, G_FILLRECT, 0 /* NDO_MISC: the sub-op names it */
};
/* Translate one F3DEX2 word into out[0], out[1]: the typed op (returns 1) or the word unchanged (returns 0). HC4e4: host only (nd_op.c:
 * nd_cmd_test, nd_model_test, nd_gv_test). */
int nd_op_xlate(uint32_t w0, uint32_t w1, uint32_t out[2]);
/* HC4b: the render mode table (W3): the typed ops of ModelRenderModes[i] and NDO_END. The recorder builds an entry at its first record
 * (nd_rec_model, core 1); the frame push publishes it; immutable after. ND_RM_N entries of ND_RM_OPS units. */
#define ND_RM_N 64u
#define ND_RM_OPS 8u
extern uint32_t nd_rm_tab[ND_RM_N][2u * ND_RM_OPS];
extern uint8_t nd_rm_n[ND_RM_N]; /* ops of entry i before its NDO_END */
void nd_rm_build(unsigned i);
#endif
