/* nd_vslot.c: N64 vertex slot resolver (plan_endgame v3 section 6, phase B). Pure C, no game headers. */
#include <string.h>
#include "nd_vslot.h"

#define VS_SLOTS 32

typedef struct VsSlot { unsigned loaded; uint32_t arr; uint32_t index; } VsSlot;

int nd_vslot_resolve(const NdVsOp* ops, unsigned nops, uint16_t* idx, unsigned idx_max, NdVsDraw* dr, unsigned dr_max,
                     unsigned* ndr, unsigned* nidx)
{
    VsSlot slot[VS_SLOTS];
    unsigned i, j, t, nd = 0, ni = 0;

    memset(slot, 0, sizeof slot);
    for (i = 0; i < nops; i++) {
        const NdVsOp* op = &ops[i];
        if (op->kind == ND_VS_VTX) {
            if ((unsigned)op->v0 + op->n > VS_SLOTS) return -(int)(i + 1);
            for (j = 0; j < op->n; j++) {
                slot[op->v0 + j].loaded = 1;
                slot[op->v0 + j].arr = op->arr;
                slot[op->v0 + j].index = op->first + j;
            }
        } else if (op->kind == ND_VS_TRI1 || op->kind == ND_VS_TRI2) {
            unsigned ntri = (op->kind == ND_VS_TRI1) ? 1u : 2u;
            for (t = 0; t < ntri; t++) {
                const uint8_t* s = &op->s[3 * t];
                uint32_t tix[3];
                uint32_t tarr = 0;
                for (j = 0; j < 3; j++) {
                    if (s[j] >= VS_SLOTS || !slot[s[j]].loaded) return -(int)(i + 1);
                    if (j == 0) {
                        tarr = slot[s[j]].arr;
                    } else if (slot[s[j]].arr != tarr) {
                        return -(int)(i + 1);
                    }
                    tix[j] = slot[s[j]].index;
                }
                if (nd == 0 || dr[nd - 1].arr != tarr) {
                    if (nd >= dr_max) return -(int)(i + 1);
                    dr[nd].arr = tarr;
                    dr[nd].idx0 = ni;
                    dr[nd].nidx = 0;
                    nd++;
                }
                if (ni + 3 > idx_max) return -(int)(i + 1);
                for (j = 0; j < 3; j++) idx[ni + j] = (uint16_t)tix[j];
                ni += 3;
                dr[nd - 1].nidx += 3;
            }
        }
    }
    *ndr = nd;
    *nidx = ni;
    return 0;
}
