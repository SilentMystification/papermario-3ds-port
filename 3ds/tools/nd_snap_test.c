/* nd_snap_test.c: host check of nd_snap_in (NDR0A). A fixed NdDs gives five fields that must reach NdCombIn: comb_w0, keys, tst[15]
 * (comb2_ad), dual_on and td[1].valid. Also tst[14] (comb2_ac) is checked. Prints `ndr0a in bad <b>`. --fault sets the expected tst[14] to the
 * value of tst[15] (bad must be above 0). Exit 1 when bad is above 0. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nd.h"
#include "nd_ds.h"
#include "nd_snap.h"

static int nf, bad;
#define CHK(name, got, want) do { nf++; if ((unsigned long)(got) != (unsigned long)(want)) { bad++; printf("ndr0a BAD %s got %lx want %lx\n", name, (unsigned long)(got), (unsigned long)(want)); } } while (0)

static NdDs ds;
int main(int argc, char** argv) {
    int fault = argc > 1 && !strcmp(argv[1], "--fault");
    NdCombIn in;
    memset(&ds, 0, sizeof ds);
    ds.w.comb_w0 = 0x11223344u;
    ds.keys = 5u;
    ds.w.tst1.comb2_ad = 9u;
    ds.w.tst1.comb2_ac = 0u;
    ds.dual_on = 1;
    ds.w.td[1].valid = 1;
    nd_snap_in(&ds, 1, 2, &in);
    {
        NdSnapChk r;
        int rb = 0;
        memset(&r, 0, sizeof r);
        ds.w.omode_l = 0x1234u; ds.w.geom = 0x5678u; ds.fogc = 0x9abcu; ds.blendc = fault ? 0x9abcu : 0xdef0u;
        nd_snap_rnd(&ds, &r);
        nf += 4;
        if (r.oml != 0x1234u) rb++;
        if (r.geom != 0x5678u) rb++;
        if (r.fogc != 0x9abcu) rb++;
        if (r.blendc != 0xdef0u) rb++;
        bad += rb;
        printf("ndr0c rnd bad %d\n", rb);
    }
    CHK("w0", in.w0, 0x11223344u);
    CHK("key_s", in.key_s, 5u);
    CHK("tst15", in.tst[15], 9u);
    CHK("dual_on", in.dual_on, 1);
    CHK("t1_valid", in.t1_valid, 1);
    CHK("tst14", in.tst[14], fault ? 9u : 0u);
    printf("ndr0a in bad %d\n", bad);
    { /* NDR0D: the box fold of nd_snap_in (box_fold_sync): a white tile 0 folds, a tile with one texel not 1.0 does not */
        static const uint8_t full[2] = { 0xff, 0xff }, nearly[2] = { 0xff, 0xfe };
        static NdDs f;
        NdCombIn fi;
        int before = bad;
        for (int k = 0; k < 2; k++) {
            memset(&f, 0, sizeof f);
            f.box_prod = 1;
            f.w.td[0].valid = 1; f.w.td[0].fmt = 4; f.w.td[0].siz = 0; f.w.td[0].w = 2; f.w.td[0].h = 2; f.w.td[0].stride = 2;
            f.w.td[0].src = k == 0 ? full : nearly;
            f.w.td[1].valid = 1;
            nd_snap_in(&f, 0, 0, &fi);
            CHK("fold_box", fi.box_folded, k == 0);
            if (k == 0) {
                CHK("fold_tst0", fi.tst[0], 3u);
                CHK("fold_tst2", fi.tst[2], fault ? 2u : 1u);
                CHK("fold_tst7", fi.tst[7], 1u);
                CHK("fold_use_t1", fi.use_t1, 1);
            }
        }
        printf("ndr0d fold bad %d\n", bad - before);
    }
    { /* NDR0F1: the mix of sync_render (nd_snap_mix) on fixed colours: fogc 0x11223344, blendc 0x55667788 */
        static const struct { unsigned ol; int nofog, kind; unsigned col; } mc[] = {
            { 0x84000000u, 0, 1, 0x55667744u }, { 0xC4000000u, 0, 1, 0x11223344u }, { 0xC4000000u, 1, 0, 0u },
            { 0xC8000000u, 0, 2, 0x112233ffu }, { 0xC8000000u, 1, 0, 0u }, { 0x00552078u, 0, 0, 0u } };
        int mb = 0;
        for (int i = 0; i < 6; i++) {
            unsigned col = 99u;
            int k = nd_snap_mix(mc[i].ol, 0x11223344u, 0x55667788u, mc[i].nofog, &col);
            int want = (fault && i == 2) ? 1 : mc[i].kind;
            if (k != want || col != mc[i].col) mb++;
        }
        bad += mb;
        printf("ndr0f mix bad %d\n", mb);
    }
    (void)nf;
    return bad ? 1 : 0;
}
