/* nd_val.h: the value calls of a call site. One typed unit per call into the open state run (nd_rec_state), as the macro's redirect.
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
#endif
