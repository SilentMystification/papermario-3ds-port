/* nd_op.h: the typed op helpers of the host compiles. The unit ids are ND_FS_ (nd_fs.h, HC4); core 0 runs them by id (nd_set.c nd_unit_same, nd_unit_apply).
 * This file keeps the unit helpers (ND_FS_W0, ND_OP_*, the field macros) and the word translator ND_X0, ND_X1 (IMGFX2 moves them to 3ds/tools/nd_x.h). */
#ifndef ND_OP_H
#define ND_OP_H
#include <stdint.h>
#include "ultra64.h" /* the F3DEX2 op values of ND_X0 */
#include "nd_fs.h"

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
#define ND_MW_KIND(w0) (ND_MW_IDX(w0) == G_MW_NUMLIGHT ? (unsigned)ND_FS_MW_NUMLIGHT : ND_MW_IS_LC(w0) ? (unsigned)ND_FS_MW_LIGHTCOL : \
    ND_MW_IDX(w0) == G_MW_FOG ? (unsigned)ND_FS_MW_FOG : ND_MW_IS_SEG(w0) ? (unsigned)ND_FS_MW_SEG : (unsigned)ND_FS_MW_NONE)
#define ND_MW_ARG(w0) (ND_MW_IS_LC(w0) ? ND_MW_LCA(ND_MW_OFF(w0)) : ND_MW_IS_SEG(w0) ? (ND_MW_OFF(w0) >> 2) : 0u)
#define ND_MW_A(w0) ND_FS_W0(ND_FS_MW, ((ND_MW_KIND(w0) << 16) | ND_MW_ARG(w0)))
#define ND_MV_VI(w0) (ND_MW_OFF(w0) >> 1)
#define ND_MV_A(w0) ND_FS_W0(ND_FS_MISC, (ND_FS_MISC_MODVTX << 16) | (((uint32_t)(w0) >> 8) & 0xff00u) | (ND_MV_VI(w0) < 255u ? ND_MV_VI(w0) : 255u))
#define ND_NOP_A ND_FS_W0(ND_FS_MISC, ND_FS_MISC_NOP << 16)
#define ND_IS_SYNC(w0) ((ND_OPB(w0) >= G_RDPLOADSYNC && ND_OPB(w0) <= G_RDPFULLSYNC) || ND_OPB(w0) == G_SPNOOP)
#define ND_X0(w0, w1) (ND_OPB(w0) == G_SETOTHERMODE_H ? ND_FS_W0(ND_FS_OMH, ND_OTH_A(w0)) : \
    ND_OPB(w0) == G_SETOTHERMODE_L ? ND_FS_W0(ND_FS_OML, ND_OTH_A(w0)) : \
    ND_OPB(w0) == G_GEOMETRYMODE ? ND_FS_W0(ND_FS_GEOM, w0) : \
    ND_OPB(w0) == G_SETPRIMCOLOR ? ND_FS_W0(ND_FS_PRIM, (uint32_t)(w0) & 0xffffu) : \
    ND_OPB(w0) == G_SETENVCOLOR ? ND_FS_W0(ND_FS_ENV, 0u) : \
    ND_OPB(w0) == G_SETFOGCOLOR ? ND_FS_W0(ND_FS_FOG, 0u) : \
    ND_OPB(w0) == G_SETBLENDCOLOR ? ND_FS_W0(ND_FS_BLEND, 0u) : \
    ND_OPB(w0) == G_SETFILLCOLOR ? ND_FS_W0(ND_FS_FILL, 0u) : \
    ND_OPB(w0) == G_SETPRIMDEPTH ? ND_FS_W0(ND_FS_PRIMZ, 0u) : \
    ND_OPB(w0) == G_SETSCISSOR ? ND_FS_W0(ND_FS_SCISSOR, w0) : \
    ND_OPB(w0) == G_TEXTURE ? ND_FS_W0(ND_FS_TEXTURE, ((uint32_t)(w0) & 0xffu) != 0u) : \
    ND_OPB(w0) == G_MOVEWORD ? ND_MW_A(w0) : \
    ND_OPB(w0) == G_SETCONVERT ? ND_FS_W0(ND_FS_RARE, ND_FS_RARE_CONV) : \
    ND_OPB(w0) == G_SETKEYR ? ND_FS_W0(ND_FS_RARE, ND_FS_RARE_KEYR) : \
    ND_OPB(w0) == G_SETKEYGB ? ND_FS_W0(ND_FS_RARE, ND_FS_RARE_KEYGB) : \
    ND_OPB(w0) == G_SETCOMBINE ? ND_FS_W0(ND_FS_COMB, w0) : \
    ND_OPB(w0) == G_SETTIMG ? ND_FS_W0(ND_FS_TIMG, w0) : \
    ND_OPB(w0) == G_SETTILE ? ND_FS_W0(ND_FS_TILE, w0) : \
    ND_OPB(w0) == G_SETTILESIZE ? ND_FS_W0(ND_FS_TILESIZE, w0) : \
    ND_OPB(w0) == G_LOADBLOCK ? ND_FS_W0(ND_FS_LOADBLOCK, w0) : \
    ND_OPB(w0) == G_LOADTILE ? ND_FS_W0(ND_FS_LOADTILE, w0) : \
    ND_OPB(w0) == G_LOADTLUT ? ND_FS_W0(ND_FS_LOADTLUT, w0) : \
    ND_OPB(w0) == G_VTX ? ND_FS_W0(ND_FS_VTX, w0) : \
    ND_OPB(w0) == G_TRI1 ? ND_FS_W0(ND_FS_TRI1, w0) : \
    ND_OPB(w0) == G_TRI2 ? ND_FS_W0(ND_FS_TRI2, w0) : \
    ND_OPB(w0) == G_MTX ? ND_FS_W0(ND_FS_MTX, w0) : \
    ND_OPB(w0) == G_POPMTX ? ND_FS_W0(ND_FS_POPMTX, w0) : \
    ND_OPB(w0) == G_MOVEMEM ? ND_FS_W0(ND_FS_MOVEMEM, w0) : \
    ND_OPB(w0) == G_DL ? ND_FS_W0(ND_FS_CALL, ((((uint32_t)(w0) >> 16) & 0xffu) == G_DL_NOPUSH)) : \
    ND_OPB(w0) == G_FILLRECT ? ND_FS_W0(ND_FS_RECT, w0) : \
    (ND_OPB(w0) == G_TEXRECT || ND_OPB(w0) == G_TEXRECTFLIP) ? ND_FS_W0(ND_FS_RECT, w0) : \
    ND_OPB(w0) == G_PM_LST ? ND_FS_W0(ND_FS_MISC, ND_FS_MISC_LST << 16) : \
    ND_OPB(w0) == G_PM_ND ? ND_FS_W0(ND_FS_MISC, ND_FS_MISC_ND << 16 | (((uint32_t)(w0) >> 8) & 0xff00u) | ((uint32_t)(w0) & 0xffu)) : \
    ND_OPB(w0) == G_MODIFYVTX ? ND_MV_A(w0) : \
    ND_OPB(w0) == G_SETCIMG ? ND_FS_W0(ND_FS_MISC, ND_FS_MISC_CIMG << 16 | (((uint32_t)(w0) >> 7) & 0x3000u) | ((uint32_t)(w0) & 0xfffu)) : \
    (ND_OPB(w0) == G_NOOP || ND_OPB(w0) == G_RDPHALF_1 || ND_OPB(w0) == G_RDPHALF_2 || ND_OPB(w0) == G_SETZIMG) ? ND_NOP_A : \
    ND_IS_SYNC(w0) ? ND_NOP_A : \
    ND_OPB(w0) == G_ENDDL ? ND_FS_W0(ND_FS_END, 0u) : \
    (uint32_t)(w0))
#define ND_X1(w0, w1) (ND_OPB(w0) == G_SETSCISSOR ? ((uint32_t)(w1) & 0xffffffu) : \
    ND_OPB(w0) == G_FILLRECT ? ((uint32_t)(w1) & 0x00ffffffu) : \
    (ND_OPB(w0) == G_TEXRECT || ND_OPB(w0) == G_TEXRECTFLIP) ? (ND_FS_RECT_TEX | ((uint32_t)(w1) & 0x07ffffffu)) : \
    (ND_OPB(w0) == G_PM_LST || ND_OPB(w0) == G_PM_ND || ND_OPB(w0) == G_ENDDL) ? 0u : \
    (uint32_t)(w1))
static inline int nd_op_typed(unsigned id) { return id - (unsigned)ND_FS_BASE < 32u; }
static inline int nd_op_geo(unsigned id) { return id - (unsigned)ND_FS_VTX <= (unsigned)(ND_FS_MOVEMEM - ND_FS_VTX); } /* HC4d: no skip test, no op switch of nd_set.c */
static inline int nd_op_ctl(unsigned id) { return id >= (unsigned)ND_FS_CALL && id < (unsigned)ND_FS_END_; } /* HC4d2: a list call, rectangle or misc op */
static inline int nd_op_tex(unsigned id) { return id - (unsigned)ND_FS_TIMG <= (unsigned)(ND_FS_LOADTLUT - ND_FS_TIMG); } /* HC4c: an image, tile or load op */
/* The mask of an othermode op from its a field (shift, length), as the walker computed it from the word. */
static inline uint32_t nd_op_om_mask(uint32_t a) {
    const unsigned sft = a & 0xffu, len = (a >> 8) & 0xffu;
    return len >= 32u ? 0xffffffffu : (((1u << len) - 1u) << sft);
}
/* HC4d2: typed units written by a producer (no word built). */
#define ND_OP_VTX_W0(n, v0) ND_FS_W0(ND_FS_VTX, (uint32_t)(n) << 12 | ((uint32_t)(v0) + (uint32_t)(n)) << 1)
#define ND_OP_TRI_A(v0, v1, v2) ((uint32_t)(v0) * 2u << 16 | (uint32_t)(v1) * 2u << 8 | (uint32_t)(v2) * 2u)
#define ND_OP_CALL_W0 ND_FS_W0(ND_FS_CALL, 0)
/* Translate one F3DEX2 word into out[0], out[1]: the typed op (returns 1) or the word unchanged (returns 0). HC4e4: host only (nd_op.c:
 * nd_cmd_test, nd_model_test, nd_gv_test). */
int nd_op_xlate(uint32_t w0, uint32_t w1, uint32_t out[2]);
/* HC4b: the render mode table (W3): the typed ops of ModelRenderModes[i] and ND_FS_END. The recorder builds an entry at its first record
 * (nd_rec_model, core 1); the frame push publishes it; immutable after. ND_RM_N entries of ND_RM_OPS units. */
#define ND_RM_N 64u
#define ND_RM_OPS 8u
extern uint32_t nd_rm_tab[ND_RM_N][2u * ND_RM_OPS];
extern uint8_t nd_rm_n[ND_RM_N]; /* ops of entry i before its ND_FS_END */
void nd_rm_build(unsigned i);
#endif
