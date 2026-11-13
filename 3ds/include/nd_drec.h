/* nd_drec.h: draw record of the golden masters. One NdDrec per draw; nd_drec_write prints it as text.
   Addresses are never written: a blob's content is written as its FNV-1a 64 hash. */
#ifndef ND_DREC_H
#define ND_DREC_H

#include <stdint.h>
#include <stdio.h>

typedef struct NdDrecReg { uint16_t reg; uint8_t mask; uint32_t val; } NdDrecReg; /* a state register write; an address register carries 0 */
typedef struct NdDrecBlob { const void* p; uint32_t n; } NdDrecBlob;          /* bytes the draw reads (n 0: none) */
typedef struct NdDrec {
    uint32_t seq, kind, n;                 /* draw number from 0; 1 indexed triangles, 2 arrays; index or vertex count */
    const NdDrecReg* regs; uint32_t nregs; /* in emit order */
    NdDrecBlob tex[3]; uint32_t tex_type[3], tex_dim[3]; /* level 0 texels of units 0-2 */
    NdDrecBlob vtx, idx;
    const float (*mtx)[16]; uint32_t nmtx; /* palette matrices, memory order */
} NdDrec;
uint64_t nd_drec_fnv(const void* p, uint32_t n);
void nd_drec_write(FILE* f, const NdDrec* r);

#endif
