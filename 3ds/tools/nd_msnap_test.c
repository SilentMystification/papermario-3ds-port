/* nd_msnap_test.c: host check of nd_msnap_in (NREC1A). Case 1: nd_msnap_take of a fixed NdDs. Case 2: diff of a snap with itself (no field, no bind).
 * Case 3: a copy with blendc 8 and bind sz[2] 0 (fields blendc, bind tile 1: result 8). Prints `nrec1a cases 3 bad <b>`.
 * --fault: case 2 expects one differing field (bad must be above 0). Exit 1 when bad is above 0. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nd.h"
#include "nd_ds.h"
#include "nd_snap.h"
#include "nd_msnap.h"

static int nf, bad;
#define CHK(name, got, want) do { nf++; if ((unsigned long)(got) != (unsigned long)(want)) { bad++; printf("nrec1a BAD %s got %lx want %lx\n", name, (unsigned long)(got), (unsigned long)(want)); } } while (0)

static NdDs d;
int main(int argc, char** argv) {
    int fault = argc > 1 && !strcmp(argv[1], "--fault");
    NdMSnap m, m2;
    const char* nm[4] = { "", "", "", "" };
    int nb = 0;
    unsigned r;
    memset(&d, 0, sizeof d);
    d.w.prim = 0x11223344u;
    d.w.tex_ss = 2u;
    d.w.tex_ts = 0x10003u;
    d.blendc = 7u;
    d.w.td[0].valid = 1;
    d.w.td[0].w = 32;
    d.w.tst1.tile_sz[1][2] = 124;

    /* case 1: take */
    nd_msnap_take(&d, &m);
    CHK("magic", m.magic, ND_MSNAP_MAGIC);
    CHK("prim", m.prim, 0x11223344u);
    CHK("tss", m.tss, 0x00020003u);
    CHK("blendc", m.blendc, 7u);
    CHK("b0_w", m.b[0].td.w, 32);
    CHK("b1_sz2", m.b[1].sz[2], 124);
    CHK("b1_valid", m.b[1].td.valid, 0);

    /* case 2: diff of a snap with itself */
    m2 = m;
    r = nd_msnap_diff(&m, &m2, nm, 4, &nb);
    if (fault) CHK("same_nb_fault", nb, 1);
    else { CHK("same_nb", nb, 0); CHK("same_r", r, 0u); }

    /* case 3: one field and one bind word differ */
    m2 = m;
    m2.blendc = 8u;
    m2.b[1].sz[2] = 0;
    r = nd_msnap_diff(&m, &m2, nm, 4, &nb);
    CHK("diff_nb", nb, 1);
    CHK("diff_name", strcmp(nm[0], "blendc") == 0, 1);
    CHK("diff_r", r, 8u);

    printf("nrec1a cases 3 bad %d\n", bad);
    return bad > 0 ? 1 : 0;
}
