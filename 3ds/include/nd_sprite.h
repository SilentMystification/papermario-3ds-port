/* S4-1: the sprite record (G_PM_SPR, include/gbi_custom.h, sprite.c) as the walker-visible draw state of its final draw.
 * Pure: the record words and the walker-sticky values the caller passes. nd_sprite.c. */
#ifndef ND_SPRITE_H
#define ND_SPRITE_H
#include <stdint.h>
#include "nd.h"

/* record words (uint32, Gfx order w0 w1 per word): 0 header (flags << 16, alpha << 8), 1 render mode list, 2 raster, 3 palette,
 * 4 matrix, 5 vertices (Quad*), 6 width << 16 | height, 7 final state list (NULL without a viewport) */
typedef struct NdSprRec {
    uint32_t flags;          /* bit 0 shading, bit 1 set viewport */
    uint32_t alpha;          /* opacity byte */
    uint32_t rm;             /* render mode list (Gfx*) */
    uint32_t raster, palette;
    uint32_t w, h;
    uint32_t fin;            /* final state list (Gfx*), 0 when none */
} NdSprRec;

void nd_sprite_decode(const uint32_t* words, NdSprRec* r);
/* the render mode of the list then the final state list (merged as the other mode bits and the combine of the later op). 0: a list op the compile refuses */
int nd_sprite_mode(const NdSprRec* r, NdMode* m);
/* the env and prim words the final draw reads: the walker values unless the flat path sets them */
void nd_sprite_colors(const NdSprRec* r, uint32_t walk_env, uint32_t walk_prim, uint32_t* env, uint32_t* prim);
#endif
