/* nd_state.c: the native state (NdC0 nd_c0). The walker ops in the former pm_gbi.c read and write nd_c0 at walk time: it is the only state.
 * nd_words is the reference decode of F3DEX2 word pairs into nd_c0 for the host tests (nd_gbi_test, nd_rec_test). The bridge
 * the list words run in the walker only (no macro-time decode), so nd_words is never called during the walk.
 * Pure C: the host test builds it. */
#include <stdint.h>
#include <string.h>
#include "nd.h"
#include "nd_ops.h"
#include "nd_tgt.h"

ND_HOTD NdC0 nd_c0 = { .geom = 0, .omode_h = 0, .omode_l = 0x00000030u, .prim = 0xffffffffu, .envc = 0xffffffffu, .fill = 0xffffffffu, .tex_ss = 0xFFFF, .tex_ts = 0xFFFF, .sci_x1 = 320, .sci_y1 = 240 };
ND_HOTD NdTgt nd_tgt = { .img = 0, .stride = 320, .siz = 2, .kind = NDK_NULL }; /* NTGT: the target record; the initial target is the main image (no address, not the zero word) */

#define MW_SEGMENT 0x06
#define MV_VIEWPORT 8

void nd_words(NdWs* s, const unsigned* w, unsigned n) {
    for (unsigned i = 0; i < n; i++) {
        uint32_t w0 = w[2 * i], w1 = w[2 * i + 1];
        switch (w0 >> 24) {
        case OP_NOOP: case OP_PIPESYNC: case OP_LOADSYNC: case OP_TILESYNC: case OP_FULLSYNC: case OP_RDPHALF_1: case OP_RDPHALF_2: break;
        case OP_GEOMETRYMODE: s->geom = (s->geom & (w0 & 0xFFFFFFu)) | w1; break;
        case OP_SETOTHERMODE_L: case OP_SETOTHERMODE_H: {
            unsigned len = (w0 & 0xFF) + 1, field = (w0 >> 8) & 0xFF, sft = 32 - field - len;
            uint32_t mask = len >= 32 ? 0xFFFFFFFFu : ((1u << len) - 1u) << sft;
            unsigned* om = (w0 >> 24) == OP_SETOTHERMODE_H ? &s->omode_h : &s->omode_l;
            *om = (*om & ~mask) | (w1 & mask);
            if ((w0 >> 24) == OP_SETOTHERMODE_H) s->om_kh |= mask; else s->om_kl |= mask; /* B8: the bits this state has a value for */
            break;
        }
        case OP_RDPSETOTHERMODE: s->omode_h = w0 & 0xFFFFFFu; s->omode_l = w1; s->om_kh = 0xFFFFFFu; s->om_kl = 0xFFFFFFFFu; break;
        case OP_SETCOMBINE: s->comb_w0 = w0; s->comb_w1 = w1; break;
        case OP_SETPRIMCOLOR: s->prim = w1; s->prim_lod = w0 & 0xFFFFu; break;
        case OP_SETENVCOLOR: s->envc = w1; break;
        case OP_SETFILLCOLOR: s->fill = w1; break;
        case OP_SETPRIMDEPTH: s->prim_z = (w1 >> 16) & 0x7FFFu; break; /* as the walker (nd_set.c set_primdepth) */
        case OP_TEXTURE: s->tex_on = (w0 & 0xFF) != 0; s->tex_ss = w1 >> 16; s->tex_ts = w1 & 0xFFFFu; break;
        case OP_SETCIMG: nd_tgt.siz = (uint8_t)((w0 >> 19) & 3); break;
        case OP_SETTIMG: s->timg = (const void*)(uintptr_t)w1; break;
        case OP_MOVEWORD: if (((w0 >> 16) & 0xFF) == MW_SEGMENT && (w0 & 0xFFFF) / 4 < 16) s->segbase[(w0 & 0xFFFF) / 4] = w1; break;
        default: break;
        }
    }
}

void nd_c0_to_ws(NdWs* d) { /* NWS3: the core 1 NdWs gets the fields of the core 0 state NdC0 */
    d->geom = nd_c0.geom; d->omode_h = nd_c0.omode_h; d->omode_l = nd_c0.omode_l; d->prim = nd_c0.prim; d->envc = nd_c0.envc; d->fill = nd_c0.fill; d->prim_z = nd_c0.prim_z;
    d->om_kh = nd_c0.om_kh; d->om_kl = nd_c0.om_kl; d->prim_lod = nd_c0.prim_lod; d->tex_on = nd_c0.tex_on; d->timg = nd_c0.timg; d->tlut = nd_c0.tlut; d->tlut_n = nd_c0.tlut_n;
    d->tile_x0 = nd_c0.tile_x0; d->tile_y0 = nd_c0.tile_y0; d->tile_fx = nd_c0.tile_fx; d->tile_fy = nd_c0.tile_fy; d->cur_tile = nd_c0.cur_tile; d->use_t1 = nd_c0.use_t1;
    d->tex_ss = nd_c0.tex_ss; d->tex_ts = nd_c0.tex_ts; memcpy(d->segbase, nd_c0.segbase, sizeof d->segbase); d->comb_w0 = nd_c0.comb_w0; d->comb_w1 = nd_c0.comb_w1;
    d->tst1 = nd_c0.tst1; memcpy(d->td, nd_c0.td, sizeof d->td);
}
