/* nd_xf.h: the native transform calls (NXF1 row). One NDC_NAPI record per call, one typed matrix op unit (nd_op.h) as nd_op_xlate
 * gives it for the matching F3DEX2 word. Core 1 records, core 0 runs it through nd_walk_state, the walker's own matrix code.
 * gSPMatrix(gMainGfxPos++, m, p) -> nd_xf_push(m, p)
 * gSPPopMatrix(gMainGfxPos++, n) -> nd_xf_pop(n) (the n is not in the F3DEX2 words; the pop takes the top, as gSPPopMatrix does) */
#ifndef ND_XF_H
#define ND_XF_H
#include <stdint.h>

void nd_xf_push(const void* m, unsigned p);
void nd_xf_pop(unsigned which);
#endif
