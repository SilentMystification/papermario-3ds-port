/* nd_geo.h: the target, depth, vertex, triangle and light calls of a call site (nd_geo.c, design_lookahead.md section 1). Each setter
 * records the typed unit(s) the nd_gbi.h redirect records for the macro (nd_rec_nr), so the frame stream is the one the macro gave.
 * gDPSetColorImage(P, f, s, w, i)        -> nd_target_begin(i, s, w) (an image other than nuGfxCfb_ptr), nd_target_end() (nuGfxCfb_ptr, 16 bit, 320 wide)
 * gDPSetDepthImage(P, i)                 -> removed (G_SETZIMG is a NOP unit, nd_op.h)
 * gDPSetDepthSource(P, src)              -> nd_pso_zsrc(src)
 * gDPSetPrimDepth(P, z, dz)              -> nd_val_primz(z, dz)
 * gSPVertex(P, v, n, v0)                 -> nd_mesh_vtx(v, n, v0)
 * gSP1Triangle(P, a, b, c, f)            -> nd_mesh_tri1(a, b, c, f)
 * gSP2Triangles(P, a, b, c, f, d, e, g, h) -> nd_mesh_tri2(a, b, c, f, d, e, g, h)
 * gSPSetLights1(P, nm) / gSPSetLights2(P, nm) -> nd_lit_set1(nm) / nd_lit_set2(nm)
 * gSPLookAt(P, la)                       -> nd_lit_lookat(la)
 * gSPPerspNormalize(P, s)                -> nd_xf_pnorm(s)
 * gSPClipRatio(P, r)                     -> nd_xf_clip(r) (needs gbi.h for FR_NEG_* / FR_POS_*) */
#ifndef ND_GEO_H
#define ND_GEO_H
#include <stdint.h>

/* NTGT: one NDC_NAPI record with the runner nd_tgt_run (nd_walk.c): the walker's G_SETCIMG unit, then set_cimg (nd_target.c) decides the address class once. */
void nd_target_beginw(uint32_t img, uint32_t siz, uint32_t width);
#define nd_target_begin(i, s, w) nd_target_beginw((uint32_t)(uintptr_t)(i), s, w)
void nd_target_end(void);
void nd_tgt_run(const uint32_t* rec);

void nd_pso_zsrc(uint32_t src);
void nd_val_primz(uint32_t z, uint32_t dz);

void nd_mesh_vtxw(uint32_t v, unsigned n, unsigned v0);
#define nd_mesh_vtx(v, n, v0) nd_mesh_vtxw((uint32_t)(uintptr_t)(v), n, v0)
void nd_mesh_tri1(unsigned a, unsigned b, unsigned c, unsigned f);
void nd_mesh_tri2(unsigned a, unsigned b, unsigned c, unsigned f, unsigned d, unsigned e, unsigned g, unsigned h);

void nd_lit_1(uint32_t l0, uint32_t a);
void nd_lit_2(uint32_t l0, uint32_t l1, uint32_t a);
#define nd_lit_set1(nm) nd_lit_1((uint32_t)(uintptr_t)&(nm).l[0], (uint32_t)(uintptr_t)&(nm).a)
#define nd_lit_set2(nm) nd_lit_2((uint32_t)(uintptr_t)&(nm).l[0], (uint32_t)(uintptr_t)&(nm).l[1], (uint32_t)(uintptr_t)&(nm).a)
void nd_lit_lookatw(uint32_t la);
#define nd_lit_lookat(la) nd_lit_lookatw((uint32_t)(uintptr_t)(la))

void nd_xf_pnorm(unsigned s);
void nd_xf_clipw(uint32_t neg, uint32_t pos);
#define nd_xf_clip(r) nd_xf_clipw(FR_NEG_##r, FR_POS_##r)
#endif
