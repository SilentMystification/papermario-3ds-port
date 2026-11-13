/* nd_val.h: the value calls of a call site (CODEMOD row, plan section 2). One NDC_NAPI record per call; the core 0 runner applies the typed
 * op through nd_walk_state, the same executor and skip test the walker uses for the matching G_ op, so the state at the next draw is the one
 * the display list macro gave. Core 1 records, core 0 runs.
 * gDPSetPrimColor(gMainGfxPos++, m, l, r, g, b, a) -> nd_val_prim(m, l, r, g, b, a)
 * gDPSetEnvColor(gMainGfxPos++, r, g, b, a)         -> nd_val_env(r, g, b, a)
 * gSPSegment(gMainGfxPos++, seg, base)              -> nd_val_seg(seg, base)
 * gDPSetTileSize(gMainGfxPos++, t, uls, ult, lrs, lrt) -> nd_val_tile_size(t, uls, ult, lrs, lrt) (10.2 fields, as the macro) */
#ifndef ND_VAL_H
#define ND_VAL_H
#include <stdint.h>

void nd_val_prim(unsigned minlod, unsigned lodfrac, unsigned r, unsigned g, unsigned b, unsigned a);
void nd_val_env(unsigned r, unsigned g, unsigned b, unsigned a);
void nd_val_seg(unsigned seg, uint32_t base);
void nd_val_tile_size(unsigned tile, unsigned uls, unsigned ult, unsigned lrs, unsigned lrt);
void nd_val_run(const uint32_t* rec); /* core 0: the NDC_NAPI runner of every record above (one typed unit) */
#endif
