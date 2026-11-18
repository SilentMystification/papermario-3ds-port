/* nd_fs_test.c: the field-set ids (nd_fs.h) equal the unit ids (nd_op.h). Compile check: one _Static_assert per id and sub id.
 * Run time: the typed word of a field set equals the typed word of the unit id. Prints `ncv1a ids <n> bad <b>`.
 * --fault compares ND_FS_TILE with NDO_TILESIZE: bad must be above 0. */
#include <stdio.h>
#include <string.h>
#include "nd_op.h"
#include "nd_fs.h"

#define ND_FS_UNIT_IDS(X) \
    X(BASE) X(OMH) X(OML) X(GEOM) X(PRIM) X(ENV) X(FOG) X(BLEND) X(FILL) X(PRIMZ) X(SCISSOR) X(TEXTURE) X(MW) X(RARE) \
    X(COMB) X(END) X(DROP) X(TIMG) X(TILE) X(TILESIZE) X(LOADBLOCK) X(LOADTILE) X(LOADTLUT) X(VTX) X(TRI1) X(TRI2) \
    X(MTX) X(POPMTX) X(MOVEMEM) X(CALL) X(RECT) X(MISC) X(END_)

#define ND_FS_SUB_IDS(X) \
    X(MW_NONE) X(MW_NUMLIGHT) X(MW_LIGHTCOL) X(MW_FOG) X(MW_SEG) \
    X(RARE_CONV) X(RARE_KEYR) X(RARE_KEYGB) \
    X(MISC_NOP) X(MISC_LST) X(MISC_ND) X(MISC_MODVTX) X(MISC_CIMG)

#define SA_UNIT(x) _Static_assert((int)ND_FS_##x == (int)NDO_##x, #x);
#define SA_SUB(x) _Static_assert((int)ND_FS_##x == (int)NDO_##x, #x);
ND_FS_UNIT_IDS(SA_UNIT)
ND_FS_SUB_IDS(SA_SUB)
_Static_assert(ND_FS_RECT_TEX == NDO_RECT_TEX, "RECT_TEX");
_Static_assert(ND_FS_A_RENDERMODE == NDO_A_RENDERMODE, "A_RENDERMODE");

static int n_cmp;
static int n_bad;

static void cmp(const char *name, uint32_t fs, uint32_t ndo)
{
    n_cmp++;
    if (fs != ndo) {
        n_bad++;
        printf("bad %s fs %08x ndo %08x\n", name, (unsigned)fs, (unsigned)ndo);
    }
}

#define CMP_UNIT(x) cmp(#x, (uint32_t)ND_FS_##x, (uint32_t)NDO_##x);
#define CMP_SUB(x) cmp(#x, (uint32_t)ND_FS_##x, (uint32_t)NDO_##x);

int main(int argc, char **argv)
{
    int fault = argc > 1 && strcmp(argv[1], "--fault") == 0;
    ND_FS_UNIT_IDS(CMP_UNIT)
    ND_FS_SUB_IDS(CMP_SUB)
    cmp("RECT_TEX", ND_FS_RECT_TEX, NDO_RECT_TEX);
    cmp("A_RENDERMODE", ND_FS_A_RENDERMODE, NDO_A_RENDERMODE);
    cmp("W0 TILE", ND_FS_W0(ND_FS_TILE, 0x123456), NDO_W0(NDO_TILE, 0x123456));
    cmp("COMB_W0", ND_FS_COMB_W0(0x0abcdef), NDO_COMB_W0(0x0abcdef));
    if (fault) cmp("FAULT TILE", (uint32_t)ND_FS_TILE, (uint32_t)NDO_TILESIZE);
    printf("ncv1a ids %d bad %d\n", n_cmp, n_bad);
    return n_bad == 0 ? 0 : 1;
}
