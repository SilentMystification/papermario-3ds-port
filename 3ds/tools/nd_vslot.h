/* nd_vslot.h: N64 vertex slot resolver (plan_endgame v3 section 6, phase B). VTX and TRI ops to absolute vertex indices. */
#ifndef ND_VSLOT_H
#define ND_VSLOT_H

#include <stdint.h>

enum { ND_VS_VTX = 1, ND_VS_TRI1 = 2, ND_VS_TRI2 = 3 };

#define ND_VS_MIX 0xFFFFFFFFu /* VSLOT2: a draw over 2 or more arrays: idx indexes ref */
#define ND_VS_IN 0xFFFFFFFEu /* VSLOT2: idx = caller slots. VSLOT3: as the arr of a ref, index = a caller slot (a triangle that mixes caller and own slots) */

/* One vertex of a mixed draw: array id and index in it. */
typedef struct NdVsRef { uint32_t arr, index; } NdVsRef;

/* VTX: arr id, first index, n, v0. TRI1: s[0..2]. TRI2: s[0..5]. */
typedef struct NdVsOp { uint8_t kind, n, v0, s[6]; uint32_t arr, first; } NdVsOp;

/* One draw: array id, first entry in idx, entry count. */
typedef struct NdVsDraw { uint32_t arr, idx0, nidx; } NdVsDraw;

/* Returns 0 on success, or -(i + 1) for the op i that fails. *ndr and *nidx hold the counts on success. */
int nd_vslot_resolve(const NdVsOp* ops, unsigned nops, uint16_t* idx, unsigned idx_max, NdVsDraw* dr, unsigned dr_max,
                     unsigned* ndr, unsigned* nidx, NdVsRef* ref, unsigned ref_max, unsigned* nref);

#endif
