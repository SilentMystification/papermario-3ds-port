/* nd_state.c: the native state half (plan_native_slices S2-1). nd_words decodes F3DEX2 word pairs, written by the bridge form of nd_gbi.h, into nd_st.
 * The decode rules are the ones the walker uses (pm_gbi.c: other mode field mask, geometry mode, scissor, prim depth, texture and tile words).
 * Pure C: the host test builds it. The walker state is not touched. */
#include <stdint.h>
#include <string.h>
#include "nd.h"
#include "nd_ops.h"

NdSt nd_st;

#define MW_SEGMENT 0x06
#define MV_VIEWPORT 8

void nd_st_reset(void) { memset(&nd_st, 0, sizeof nd_st); }

void nd_words(const unsigned* w, unsigned n) {
    NdSt* s = &nd_st;
    for (unsigned i = 0; i < n; i++) {
        uint32_t w0 = w[2 * i], w1 = w[2 * i + 1];
        switch (w0 >> 24) {
        case OP_NOOP: case OP_PIPESYNC: case OP_LOADSYNC: case OP_TILESYNC: case OP_FULLSYNC: case OP_RDPHALF_1: case OP_RDPHALF_2: break;
        case OP_GEOMETRYMODE: s->geom = (s->geom & (w0 & 0xFFFFFFu)) | w1; break;
        case OP_SETOTHERMODE_L: case OP_SETOTHERMODE_H: {
            unsigned len = (w0 & 0xFF) + 1, field = (w0 >> 8) & 0xFF, sft = 32 - field - len;
            uint32_t mask = len >= 32 ? 0xFFFFFFFFu : ((1u << len) - 1u) << sft;
            uint32_t* om = (w0 >> 24) == OP_SETOTHERMODE_H ? &s->om_h : &s->om_l;
            *om = (*om & ~mask) | (w1 & mask);
            if ((w0 >> 24) == OP_SETOTHERMODE_H) s->om_kh |= mask; else s->om_kl |= mask; /* B8: the bits this state has a value for */
            break;
        }
        case OP_RDPSETOTHERMODE: s->om_h = w0 & 0xFFFFFFu; s->om_l = w1; s->om_kh = 0xFFFFFFu; s->om_kl = 0xFFFFFFFFu; break;
        case OP_SETCOMBINE: s->comb_w0 = w0; s->comb_w1 = w1; break;
        case OP_SETPRIMCOLOR: s->prim = w1; s->prim_lod = w0 & 0xFFFFu; break;
        case OP_SETENVCOLOR: s->env = w1; break;
        case OP_SETFOGCOLOR: s->fog = w1; break;
        case OP_SETBLENDCOLOR: s->blend = w1; break;
        case OP_SETFILLCOLOR: s->fill = w1; break;
        case OP_SETPRIMDEPTH: s->primz = (uint16_t)(w1 >> 16); s->primdz = (uint16_t)w1; break;
        case OP_SETSCISSOR:
            s->sc_ulx = (uint16_t)((w0 >> 12) & 0xFFF); s->sc_uly = (uint16_t)(w0 & 0xFFF);
            s->sc_mode = (uint8_t)((w1 >> 24) & 3); s->sc_lrx = (uint16_t)((w1 >> 12) & 0xFFF); s->sc_lry = (uint16_t)(w1 & 0xFFF);
            break;
        case OP_TEXTURE:
            s->tex_level = (uint8_t)((w0 >> 11) & 7); s->tex_tile = (uint8_t)((w0 >> 8) & 7); s->tex_on = (uint8_t)((w0 >> 1) & 0x7F);
            s->tex_s = (uint16_t)(w1 >> 16); s->tex_t = (uint16_t)w1;
            break;
        case OP_SETCIMG: s->cimg_w0 = w0; s->cimg_w1 = w1; break;
        case OP_SETZIMG: s->zimg_w1 = w1; break;
        case OP_SETTIMG: s->timg_w0 = w0; s->timg_w1 = w1; break;
        case OP_SETTILE: s->tile_w0[(w1 >> 24) & 7] = w0; s->tile_w1[(w1 >> 24) & 7] = w1; break;
        case OP_SETTILESIZE: s->tsz_w0[(w1 >> 24) & 7] = w0; s->tsz_w1[(w1 >> 24) & 7] = w1; break;
        case OP_LOADTILE: case OP_LOADBLOCK: case OP_LOADTLUT: s->n_load++; break;
        case OP_MOVEMEM: if ((w0 & 0xFF) == MV_VIEWPORT) s->vp_addr = w1; break;
        case OP_MOVEWORD: if (((w0 >> 16) & 0xFF) == MW_SEGMENT) s->segment[((w0 & 0xFFFF) / 4) & 15] = w1; break;
        case OP_MTX: s->mtx_flags = (w0 & 0xFF) ^ 1u /* G_MTX_PUSH is inverted in F3DEX2 */; s->mtx_addr = w1; s->n_mtx++; break;
        case OP_POPMTX: s->n_pop++; break;
        case OP_TEXRECT: case OP_TEXRECTFLIP: s->n_rect++; break;
        case OP_FILLRECT: s->n_fill++; break;
        case OP_VTX: s->n_vtx++; break;
        case OP_TRI1: s->n_tri++; break;
        case OP_TRI2: case OP_QUAD: s->n_tri += 2; break;
        case OP_DL: s->n_dl++; break;
        case OP_ENDDL: s->n_enddl++; break;
        default: s->n_unknown++; s->last_unknown = w0 >> 24; break;
        }
    }
}
