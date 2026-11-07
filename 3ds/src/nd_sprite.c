/* S4-1: sprite record as native draw state (see nd_sprite.h). */
#include <stdint.h>
#include <string.h>
#include "nd_sprite.h"

void nd_sprite_decode(const uint32_t* w, NdSprRec* r) {
    r->flags = (w[0] >> 16) & 0xffu;
    r->alpha = (w[0] >> 8) & 0xffu;
    r->rm = w[1];
    r->raster = w[2];
    r->palette = w[3];
    r->w = w[6] >> 16;
    r->h = w[6] & 0xffffu;
    r->fin = w[7];
    r->quad = w[5];
}

int nd_sprite_draw(const uint32_t* rec, int emit, NdDraw* n, NdMState* st, float pal0[16], int* why) {
    NdSprRec r;
    nd_sprite_decode(rec, &r);
    if (!pm_gpu_spr_build(rec, n, st, pal0, why)) return 0;
    n->pal = (const float(*)[16])pal0;
    n->npal = 1;
    if (emit && !pm_gpu_spr_submit(n, (const void*)(uintptr_t)r.quad, n->u[0].dim != 0)) { *why = 20; return 0; }
    return 1;
}
