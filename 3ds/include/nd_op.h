/* nd_op.h: the typed op (HC4, plan_hotcode section 5.1). One 8-byte unit as the frame stream unit: w0 = id << 24 | a (24 bits), w1 = b.
 * The ids are 0x20 + k (k < 32), free of the F3DEX2 ops (0x01-0x07, 0xd7-0xff) and the G_PM_* ids (0x0d-0x13). The recorder (core 1) writes
 * typed ops into the NDC_STATE runs (nd_op_xlate); core 0 runs them by id (nd_set.c nd_op_same, nd_op_apply) and decodes no word for them.
 * HC4a: the scalar state ops. HC4b: the combine, the end of a typed table, the dropped-op mark of the t4chk check. Host-built (nd_cmd_test.c). */
#ifndef ND_OP_H
#define ND_OP_H
#include <stdint.h>

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
    NDO_END_
};
enum { NDO_MW_NONE, NDO_MW_NUMLIGHT, NDO_MW_LIGHTCOL /* arg = light k */, NDO_MW_FOG, NDO_MW_SEG /* arg = segment */ };
enum { NDO_RARE_CONV, NDO_RARE_KEYR, NDO_RARE_KEYGB };

#define NDO_W0(id, a) (((uint32_t)(id) << 24) | ((uint32_t)(a) & 0xffffffu))
#define NDO_COMB_W0(a) ((uint32_t)0xfcu << 24 | ((uint32_t)(a) & 0xffffffu)) /* the G_SETCOMBINE word of a typed combine (nd_ws.comb_w0, model keys) */
#define NDO_A_RENDERMODE (3u | 29u << 8) /* the a field of gDPSetRenderMode (G_MDSFT_RENDERMODE 3, 29 bits) */
static inline int nd_op_typed(unsigned id) { return id - (unsigned)NDO_BASE < 32u; }
/* The mask of an othermode op from its a field (shift, length), as the walker computed it from the word. */
static inline uint32_t nd_op_om_mask(uint32_t a) {
    const unsigned sft = a & 0xffu, len = (a >> 8) & 0xffu;
    return len >= 32u ? 0xffffffffu : (((1u << len) - 1u) << sft);
}
extern const uint8_t nd_op_n64[NDO_END_ - NDO_BASE]; /* the F3DEX2 op of each id (pm_gbi_cur_op, census) */
/* Translate one F3DEX2 word into out[0], out[1]: the typed op (returns 1) or the word unchanged (returns 0). */
int nd_op_xlate(uint32_t w0, uint32_t w1, uint32_t out[2]);
/* HC4b: the render mode table (W3): the typed ops of ModelRenderModes[i] and NDO_END. The recorder builds an entry at its first record
 * (nd_rec_model, core 1); the frame push publishes it; immutable after. ND_RM_N entries of ND_RM_OPS units. */
#define ND_RM_N 64u
#define ND_RM_OPS 8u
extern uint32_t nd_rm_tab[ND_RM_N][2u * ND_RM_OPS];
extern uint8_t nd_rm_n[ND_RM_N]; /* ops of entry i before its NDO_END */
void nd_rm_build(unsigned i);
#endif
