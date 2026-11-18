/* nd_snap_in.c: the combine inputs of an NdDs (NDR0A, snapchk). Pure, no globals: host-built (3ds/tools/nd_snap_test.c). The fields are the
 * ones comb_in_walker (nd_comb.c) reads from nd_ws and the setter globals; box_folded is 0 here (the core 0 fold state is not in NdDs). */
#include <string.h>
#include <stdint.h>
#include "nd.h"
#include "nd_ds.h"
#include "nd_snap.h"

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
}

/* NDR0C: the render inputs of the snapshot (the walker reads the same words: nd_ws.omode_l, nd_ws.geom, fogc, blendc). */
void nd_snap_rnd(const NdDs* s, NdSnapChk* r) {
    r->oml = s->w.omode_l; r->geom = s->w.geom; r->fogc = s->fogc; r->blendc = s->blendc;
}
