/* nd_vslot.c: N64 vertex slot resolver (plan_endgame v3 section 6, phase B). Pure C, no game headers. */
#include <string.h>
#include "nd_vslot.h"

#define VS_SLOTS 32

typedef struct VsSlot { unsigned loaded; uint32_t arr; uint32_t index; } VsSlot;

int nd_vslot_resolve(const NdVsOp* ops, unsigned nops, uint16_t* idx, unsigned idx_max, NdVsDraw* dr, unsigned dr_max,
                     unsigned* ndr, unsigned* nidx, NdVsRef* ref, unsigned ref_max, unsigned* nref)
{
    VsSlot slot[VS_SLOTS];
    unsigned i, j, t, r, nd = 0, ni = 0, nr = 0;

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
                unsigned nl = 0;
                for (j = 0; j < 3; j++) {
                    if (s[j] >= VS_SLOTS) return -(int)(i + 1);
                    nl += slot[s[j]].loaded;
                }
                if (nl == 1 || nl == 2) return -(int)(i + 1);
                if (nl == 0) {
                    tarr = ND_VS_IN;
                    for (j = 0; j < 3; j++) tix[j] = s[j];
                } else {
                    tarr = slot[s[0]].arr;
                    for (j = 1; j < 3; j++)
                        if (slot[s[j]].arr != tarr) tarr = ND_VS_MIX;
                    if (tarr != ND_VS_MIX) {
                        for (j = 0; j < 3; j++) tix[j] = slot[s[j]].index;
                    } else {
                        for (j = 0; j < 3; j++) {
                            for (r = 0; r < nr; r++)
                                if (ref[r].arr == slot[s[j]].arr && ref[r].index == slot[s[j]].index) break;
                            if (r == nr) {
                                if (nr >= ref_max) return -(int)(i + 1);
                                ref[nr].arr = slot[s[j]].arr;
                                ref[nr].index = slot[s[j]].index;
                                nr++;
                            }
                            tix[j] = r;
                        }
                    }
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
    *nref = nr;
    *ndr = nd;
    *nidx = ni;
    return 0;
}
