/* nd_op.h: the typed op helpers of the host compiles. The unit ids are ND_FS_ (nd_fs.h, HC4); core 0 runs them by id (nd_set.c nd_unit_same, nd_unit_apply).
 * This file keeps the unit helpers (ND_FS_W0, ND_OP_*, the field macros) and the word translator is 3ds/tools/nd_x.h (ND_X0, ND_X1; IMGFX2, host only). */
#ifndef ND_OP_H
#define ND_OP_H
#include <stdint.h>
#include "ultra64.h" /* the F3DEX2 op values of ND_X0 */
#include "nd_fs.h"

#define ND_NOP_A ND_FS_W0(ND_FS_MISC, ND_FS_MISC_NOP << 16)
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
