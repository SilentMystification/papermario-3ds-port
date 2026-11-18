/* MODELRM host test: the device render mode entries (nd_rm_dev, ndsd_<list>: made by gen_gfx_tab.sh) against the frozen old lists
 * (3ds/tests/modelrm/old_lists.c, old_ symbol prefix). Each entry word for word; each def equal after nd_scomp_analyse. */
#include <stdio.h>
#include <string.h>
#include "ultra64.h"
#include "nd_op.h"
#include "nd_gtab.h"
#include "nd_rmdef.h"
extern Gfx* old_ModelRenderModes[];
#define OLD_EXT(n) extern NdSDef old_##n##_def;
ND_RM_LIST(OLD_EXT)
static int bad, nent, ndef;
static void chk_def(const char* nm, NdSDef* o, NdSDef* n) {
    if (o->n != n->n || memcmp(o->u, n->u, 8u * o->n)) { printf("modelrm FAIL def %s: units differ (old n %u, new n %u)\n", nm, o->n, n->n); bad++; return; }
    const int ro = nd_scomp_analyse(o), rn = nd_scomp_analyse(n);
    n->u = o->u; o->id = n->id = 0; /* id: a run counter (a hint), not a property of the def */
    if (ro != rn || memcmp(o, n, sizeof *o)) { printf("modelrm FAIL def %s: analysis differs\n", nm); bad++; return; }
    ndef++;
}
int main(void) {
#define CHK(n) chk_def(#n, &old_##n##_def, &ndsd_##n);
    ND_RM_LIST(CHK)
    for (unsigned i = 0; i < ND_RM_LISTS; i++) {
        const uint32_t* o = (const uint32_t*)old_ModelRenderModes[i];
        const NdNDev* e = nd_rm_dev[i];
        unsigned k = 0;
        while ((o[2u * k] >> 24) != ND_FS_END) k++;
        if (!e || e->nu != k || memcmp(o, e->u, 8u * k)) { printf("modelrm FAIL entry %u: %u old units, %u new\n", i, k, e ? e->nu : 0u); bad++; continue; }
        nent++;
    }
    if (bad || nent != ND_RM_LISTS) { printf("modelrm FAIL %d\n", bad); return 1; }
    printf("modelrm ok %d (defs %d)\n", nent, ndef);
    return 0;
}
