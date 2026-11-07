/* nd_state.c: the native state (NdWs, nd_ws). The walker ops in pm_gbi.c read and write nd_ws at walk time: it is the only state.
 * nd_words is the reference decode of F3DEX2 word pairs into nd_ws for the host tests (nd_gbi_test, nd_rec_test). The bridge
 * leaves (nd_gbi.h) no longer decode at macro time, so nd_words is never called during the walk.
 * Pure C: the host test builds it. */
#include <stdint.h>
#include <string.h>
#include "nd.h"
#include "nd_ops.h"

ND_HOTD NdWs nd_ws = { .geom = 0, .omode_h = 0, .omode_l = 0x00000030u, .cimg = 0, .cimg_siz = 2, .prim = 0xffffffffu, .envc = 0xffffffffu, .fill = 0xffffffffu, .tex_ss = 0xFFFF, .tex_ts = 0xFFFF };

#define MW_SEGMENT 0x06
#define MV_VIEWPORT 8

/* CORE-2a-3: the record-mode leaf (nd_rec.h). The record of a leaf in the main list is CORE-2c. Until then the leaf writes its words
 * outside the main list and nothing inside it. */
void nd_rec_leaf(void* at, const void* w, unsigned nops, unsigned nslots, int in_main) {
    (void)nops;
    if (!in_main) memcpy(at, w, (size_t)nslots * 8u);
}

void nd_words(const unsigned* w, unsigned n) {
    NdWs* s = &nd_ws;
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
        case OP_SETPRIMDEPTH: s->prim_z = w1 >> 16; break;
        case OP_TEXTURE: s->tex_on = (w0 & 0xFF) != 0; s->tex_ss = w1 >> 16; s->tex_ts = w1 & 0xFFFFu; break;
        case OP_SETCIMG: s->cimg_siz = (int)((w0 >> 19) & 3); break;
        case OP_SETTIMG: s->timg = (const void*)(uintptr_t)w1; break;
        case OP_MOVEWORD: if (((w0 >> 16) & 0xFF) == MW_SEGMENT && (w0 & 0xFFFF) / 4 < 16) s->segbase[(w0 & 0xFFFF) / 4] = w1; break;
        default: break;
        }
    }
}
