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

int nd_sprite_mode(const NdSprRec* r, NdMode* m) {
    NdMode f;
    if (!r->rm || !nd_mode_compile(m, (const unsigned*)(uintptr_t)r->rm)) return 0;
    if (r->fin) {
        if (!nd_mode_compile(&f, (const unsigned*)(uintptr_t)r->fin)) return 0;
        m->om_h = (m->om_h & ~f.wh) | f.om_h; m->wh |= f.wh;
        m->om_l = (m->om_l & ~f.wl) | f.om_l; m->wl |= f.wl;
        if (f.has_cb) { m->has_cb = 1; m->cw0 = f.cw0; m->cw1 = f.cw1; }
    }
    m->ok = 1;
    return 1;
}

void nd_sprite_colors(const NdSprRec* r, uint32_t walk_env, uint32_t walk_prim, uint32_t* env, uint32_t* prim) {
    const int shaded = (r->flags & 1u) != 0, vp = (r->flags & 2u) != 0, opaque = r->alpha == 255u;
    *env = walk_env; *prim = walk_prim;
    if (!shaded && !opaque) *prim = r->alpha; /* sprite.c: the translucent prim colour before the list */
    if (vp) {
        if (shaded) *env = r->alpha;           /* (0, 0, 0, alpha) */
        else { *env = 0x646464ffu; *prim = r->alpha; } /* (100, 100, 100, 255) after (0, 0, 0, alpha); prim (0, 0, 0, 0, 0, alpha) */
    }
}
