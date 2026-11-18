/* nd_snap_in.c: the combine inputs of an NdDs (NDR0A, snapchk). Pure, no globals: host-built (3ds/tools/nd_snap_test.c). The fields are the
 * ones comb_in_walker (nd_comb.c) reads from nd_ws and the setter globals; box_folded is 0 here (the core 0 fold state is not in NdDs). */
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include "nd.h"
#include "nd_ds.h"
#include "nd_snap.h"
#include "PR/gbi.h"

void nd_snap_in(const NdDs* s, int leg, int pat, NdCombIn* in) {
    const NdTst* t = &s->w.tst1;
    memset(in, 0, sizeof *in);
    in->w0 = s->w.comb_w0; in->w1 = s->w.comb_w1; in->omode_h = s->w.omode_h; in->prim = s->w.prim; in->env = s->w.envc;
    in->prim_lo8 = (uint8_t)s->w.prim_lod; in->key_c = s->keyc; in->key_s = s->keys; in->conv_k4 = s->conv_k4;
    in->tst[0] = t->comb_a; in->tst[1] = t->comb_b; in->tst[2] = t->comb_c; in->tst[3] = t->comb_d;
    in->tst[4] = t->comb_aa; in->tst[5] = t->comb_ab; in->tst[6] = t->comb_ac; in->tst[7] = t->comb_ad;
    in->tst[8] = t->comb2_a; in->tst[9] = t->comb2_b; in->tst[10] = t->comb2_c; in->tst[11] = t->comb2_d;
    in->tst[12] = t->comb2_aa; in->tst[13] = t->comb2_ab; in->tst[14] = t->comb2_ac; in->tst[15] = t->comb2_ad;
    in->leg_mode = leg;
    in->tex_on = s->w.tex_on; in->use_t1 = s->w.use_t1; in->t1_valid = s->w.td[1].valid; in->dual_on = s->dual_on;
    in->box_prod = s->box_prod; in->box_pat = pat; in->box_folded = 0; in->comb_set = s->comb_set; in->comb_alpha_shade = s->comb_alpha_shade;
    if (s->box_prod && s->w.td[1].valid && nd_td_white(&s->w.td[0])) { /* NDR0D: box_fold_sync nd_comb.c:377-381 */
        in->tst[0] = 3; in->tst[1] = 5; in->tst[2] = 1; in->tst[3] = 5; in->tst[4] = 7; in->tst[5] = 7; in->tst[6] = 7; in->tst[7] = 1;
        in->use_t1 = 1; in->box_folded = 1;
    }
}

/* NDR0D: tile0_white (nd_comb.c) on the tile d: 1 when all texels are 1.0 (I4 0xF or I8 0xFF). */
int nd_td_white(const NdTd* d) {
    const uint8_t* src = d->src;
    if (!d->valid || !src || d->fmt != G_IM_FMT_I || d->siz > 1) return 0;
    for (int y = 0; y < d->h; y++) {
        for (int x = 0; x < d->w; x++) {
            if (d->siz == 0) {
                unsigned b = src[(y * d->stride + x) >> 1];
                if (((x & 1) ? (b & 15u) : (b >> 4)) != 15u) return 0;
            } else if (src[y * d->stride + x] != 255) {
                return 0;
            }
        }
    }
    return 1;
}

/* NDR0C: the render inputs of the snapshot (the walker reads the same words: nd_ws.omode_l, nd_ws.geom, fogc, blendc). */
void nd_snap_rnd(const NdDs* s, NdSnapChk* r) {
    r->oml = s->w.omode_l; r->geom = s->w.geom; r->fogc = s->fogc; r->blendc = s->blendc;
}

/* NDR0G1: the bind block of tile n as ensure_tile and dual_bind read it (td zero when not valid). */
void nd_snap_bind(const NdTd* td, const NdTst* t, int n, NdSnapBind* o) {
    memset(o, 0, sizeof *o);
    if (td->valid) o->td = *td;
    memcpy(o->wrap, t->tile_wrap[n], sizeof o->wrap);
    memcpy(o->sz, t->tile_sz[n], sizeof o->sz);
    o->shift[0] = t->tile_shift[n][0]; o->shift[1] = t->tile_shift[n][1];
}

/* NDR0G2: the difference of two bind pairs: bit 1 << 2n for the td of tile n, 2 << 2n for its wrap, sz and shift words. */
unsigned nd_snap_bdiff(const NdSnapBind* a, const NdSnapBind* b) {
    unsigned m = 0u;
    for (int n = 0; n < 2; n++) {
        if (memcmp(&a[n].td, &b[n].td, sizeof a[n].td)) m |= 1u << (2 * n);
        if (memcmp(a[n].wrap, b[n].wrap, sizeof a[n] - offsetof(NdSnapBind, wrap))) m |= 2u << (2 * n);
    }
    return m;
}

/* NDR0F1: the mix of sync_render (nd_set.c) on the arguments. Returns 1 with col = the colour mix of nd_mix1 (nd_set.c), 2 with col = the
 * fog shade alpha mix (F08), 0 with col 0 (no mix). */
int nd_snap_mix(unsigned ol, unsigned fogc, unsigned blendc, int nofog, unsigned* col) {
    unsigned p = (ol >> 30) & 3u, a = (ol >> 26) & 3u, m = (ol >> 22) & 3u, b = (ol >> 18) & 3u;
    *col = 0;
    if ((p == 2u || p == 3u) && a == 1u && m == 0u && b == 0u && !(p == 3u && nofog)) {
        *col = ((p == 2u ? blendc : fogc) & 0xffffff00u) | (fogc & 0xffu);
        return 1;
    }
    if (!nofog && p == 3u && a == 2u && m == 0u && b == 0u) {
        *col = (fogc & 0xffffff00u) | 0xffu;
        return 2;
    }
    return 0;
}
