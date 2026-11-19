/* IMGFX2: nd_x.h, the F3DEX2 word to typed unit translator of the host tools and tests (ND_X0, ND_X1).
 * No device file includes it: the device keeps the typed units of nd_op.h. */
#ifndef ND_X_H
#define ND_X_H
#include <stdint.h>
#include "ultra64.h"
#include "nd_op.h"
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
#endif
