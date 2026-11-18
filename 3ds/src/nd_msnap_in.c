/* nd_msnap_in.c: the model snap of an NdDs and its compare (NREC1A). Pure, no globals: host-built (3ds/tools/nd_msnap_test.c). */
#include <string.h>
#include <stdint.h>
#include "nd.h"
#include "nd_ds.h"
#include "nd_snap.h"
#include "nd_msnap.h"

void nd_msnap_take(const NdDs* s, NdMSnap* m) {
    memset(m, 0, sizeof *m);
    m->magic = ND_MSNAP_MAGIC;
    m->prim = s->w.prim; m->prim_lod = s->w.prim_lod; m->omh = s->w.omode_h; m->oml = s->w.omode_l; m->kh = s->w.om_kh; m->kl = s->w.om_kl;
    m->tss = (s->w.tex_ss << 16) | (s->w.tex_ts & 0xffffu); m->geom = s->w.geom; m->cw0 = s->w.comb_w0; m->cw1 = s->w.comb_w1; m->fogc = s->fogc; m->blendc = s->blendc; m->envc = s->w.envc;
    nd_snap_bind(&s->w.td[0], &s->w.tst1, 0, &m->b[0]); nd_snap_bind(&s->w.td[1], &s->w.tst1, 1, &m->b[1]);
}

#define MS_F(f) do { if (a->f != b->f) { if (*nb < max) nm[*nb] = #f; (*nb)++; } } while (0)
unsigned nd_msnap_diff(const NdMSnap* a, const NdMSnap* b, const char** nm, int max, int* nb) {
    *nb = 0;
    MS_F(prim); MS_F(prim_lod); MS_F(omh); MS_F(oml); MS_F(kh); MS_F(kl); MS_F(tss); MS_F(geom); MS_F(cw0); MS_F(cw1); MS_F(fogc); MS_F(blendc); MS_F(envc);
    return nd_snap_bdiff(a->b, b->b);
}
