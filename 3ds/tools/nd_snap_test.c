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
    CHK("w0", in.w0, 0x11223344u);
    CHK("key_s", in.key_s, 5u);
    CHK("tst15", in.tst[15], 9u);
    CHK("dual_on", in.dual_on, 1);
    CHK("t1_valid", in.t1_valid, 1);
    CHK("tst14", in.tst[14], fault ? 9u : 0u);
    printf("ndr0a in bad %d\n", bad);
    (void)nf;
    return bad ? 1 : 0;
}
