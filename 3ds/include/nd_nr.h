/* nd_nr.h: the native record of the frame stream (NSTREAM step 1, design_lookahead section 8). A setter builds its record from its native
 * fields (no gbi word, no ND_X0 translation): k = the typed id (ND_FS_*) << 24 | a (24 bits), v = b. The fields keep the layout of the HC4
 * units (nd_op.h), so every record is the bit pattern the gbi macro gave through ND_X0 and ND_X1. nd_nr writes it with nd_rec_nr. */
#ifndef ND_NR_H
#define ND_NR_H
#include <stdint.h>
#include "nd_op.h"
#include "nd_cmd.h"
#include "nd_fs.h"

#define nd_nr(k, v) nd_rec_nr((k), (v)) /* one native record into the open run (NSTREAM2 renames the recorder) */
#define ND_NR_AT(k, v) (nrp[0] = (k), nrp[1] = (v), nrp += 2) /* NSTREAM3: core 0, into the site's uint32_t* nrp */
static inline void nd_nr_o(uint32_t** o, uint32_t k, uint32_t v) { if (o) { uint32_t* q = *o; q[0] = k; q[1] = v; *o = q + 2; } else nd_rec_nr(k, v); }
#define ND_NR_OM(h, sft, len) ND_FS_W0((h) ? ND_FS_OMH : ND_FS_OML, ((uint32_t)(sft) & 0xffu) | ((uint32_t)(len) & 0xffu) << 8)
#define ND_NR_GEOM(clr) ND_FS_W0(ND_FS_GEOM, ~(uint32_t)(clr) & 0xffffffu)
#define ND_NR_TEXON(on) ND_FS_W0(ND_FS_TEXTURE, ((uint32_t)(on) & 0x7fu) != 0u)
#define ND_NR_HL(h, l) (((uint32_t)(h) & 0xffffu) << 16 | ((uint32_t)(l) & 0xffffu))
#define ND_NR_XY12(x, y) (((uint32_t)(x) & 0xfffu) << 12 | ((uint32_t)(y) & 0xfffu))
#define ND_NR_XY10(x, y) (((uint32_t)(x) & 0x3ffu) << 14 | ((uint32_t)(y) & 0x3ffu) << 2)
#define ND_NR_TXY(tile, x, y) (((uint32_t)(tile) & 7u) << 24 | ND_NR_XY12(x, y))
#define ND_NR_CIMG(siz, width) ND_FS_W0(ND_FS_MISC, (uint32_t)ND_FS_MISC_CIMG << 16 | ((uint32_t)(siz) & 3u) << 12 | (((uint32_t)(width) - 1u) & 0xfffu))
#define ND_NR_VTX(n, v0) ND_FS_W0(ND_FS_VTX, ((uint32_t)(n) & 0xffu) << 12 | (((uint32_t)(v0) + (uint32_t)(n)) & 0x7fu) << 1)
#define ND_NR_MW(kind, arg) ND_FS_W0(ND_FS_MW, (uint32_t)(kind) << 16 | (uint32_t)(arg))
#define ND_NR_MOVEMEM(len, idx, ofs) ND_FS_W0(ND_FS_MOVEMEM, ((((uint32_t)(len) - 1u) >> 3) & 0x1fu) << 19 | (((uint32_t)(ofs) >> 3) & 0xffu) << 8 | ((uint32_t)(idx) & 0xffu))
#define ND_NR_IMG(fmt, siz, width) (((uint32_t)(fmt) & 7u) << 21 | ((uint32_t)(siz) & 3u) << 19 | (((uint32_t)(width) - 1u) & 0xfffu))
#define ND_NR_TILE_A(fmt, siz, line, tmem) (((uint32_t)(fmt) & 7u) << 21 | ((uint32_t)(siz) & 3u) << 19 | ((uint32_t)(line) & 0x1ffu) << 9 | ((uint32_t)(tmem) & 0x1ffu))
#define ND_NR_TILE_B(tile, pal, cmt, maskt, shiftt, cms, masks, shifts) \
    (((uint32_t)(tile) & 7u) << 24 | ((uint32_t)(pal) & 0xfu) << 20 | ((uint32_t)(cmt) & 3u) << 18 | ((uint32_t)(maskt) & 0xfu) << 14 | \
     ((uint32_t)(shiftt) & 0xfu) << 10 | ((uint32_t)(cms) & 3u) << 8 | ((uint32_t)(masks) & 0xfu) << 4 | ((uint32_t)(shifts) & 0xfu))
#define ND_NR_TLUT(tile, count) (((uint32_t)(tile) & 7u) << 24 | ((uint32_t)(count) & 0x3ffu) << 14)
#define ND_NR_RGBA(r, g, b, a) (((uint32_t)(r) & 0xffu) << 24 | ((uint32_t)(g) & 0xffu) << 16 | ((uint32_t)(b) & 0xffu) << 8 | ((uint32_t)(a) & 0xffu))
#define ND_NR_KEYR(cR, sR, wR) (((uint32_t)(wR) & 0xfffu) << 16 | ((uint32_t)(cR) & 0xffu) << 8 | ((uint32_t)(sR) & 0xffu))
#define ND_NR_TR_W0(xh, yh) ND_FS_W0(ND_FS_RECT, ND_NR_XY12(xh, yh)) /* DEL1A: the rectangle unit form (nd_texrect reads the low 24 bits) */
#endif
