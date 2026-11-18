/* nd_dschk_test.c: host check of nd_ds_cmp (NST3, design_nstate.md section 5). Three cases: two equal NdDs give 0; a copy with prim
 * changed gives 1 and the name prim (the seeded fault); a copy with only td[0] changed gives 0 (td is a draw-time field).
 * Prints `nst3 cmp ok 3/3` when all three pass. */
#include <stdio.h>
#include <string.h>
#include "nd_ds.h"
#include "nd_gtab.h"

int pm_wide;
int is_host(u32 p) { return p >= 0x00100000u; }
void tex_win_s25(int* a, int* b, int* c, int* d, unsigned e, unsigned f, unsigned g, unsigned h) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; }
/* NST2A: nd_ds.c calls the table lookups (no table in this test). */
const NdNDev* nd_ntab_find(const void* list) { (void)list; return NULL; }
const NdGTab* nd_gtab_find(const void* list) { (void)list; return NULL; }

int main(void) {
    const char* bad[4];
    int ok = 0, n;
    NdDs a = nd_ds, b = nd_ds;
    n = nd_ds_cmp(&a, &b, bad, 4);
    if (n == 0) ok++; else printf("nst3 BAD equal copies gave %d\n", n);
    b.w.prim ^= 0x100u;
    n = nd_ds_cmp(&a, &b, bad, 4);
    if (n == 1 && !strcmp(bad[0], "prim")) ok++; else printf("nst3 BAD prim case gave %d first %s\n", n, n > 0 ? bad[0] : "-");
    b = nd_ds;
    b.w.td[0].w = 7;
    n = nd_ds_cmp(&a, &b, bad, 4);
    if (n == 0) ok++; else printf("nst3 BAD td case gave %d first %s\n", n, n > 0 ? bad[0] : "-");
    printf("nst3 cmp ok %d/3\n", ok);
    return ok == 3 ? 0 : 1;
}
