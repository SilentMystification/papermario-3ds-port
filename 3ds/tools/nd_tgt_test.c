/* nd_tgt_test.c: nd_tgt_role (nd_tgt.h) against a copy of the former img_role body (nd_target.c before NTGT). The copy takes the six address
 * tests as inputs. Every combination of the kind bits, siz 0-3, for_fill, cycle type, VISCVG or not, oml bits 0x30, zcol. */
#include <stdio.h>
#include <stdint.h>
#include "ultra64.h"
#include "nd_tgt.h"

NdWs nd_ws;
NdTgt nd_tgt;
int zcol;
void pm_zbuf_need(void) {}

static int ref_role(int img_null, int zero, int depthq, int cfb, int inz, int native, int siz, int for_fill, uint32_t oml, uint32_t omh, int zc) {
    if (img_null && !zero) return ROLE_MAIN;
    if (depthq) return ROLE_DEPTHQ;
    if (for_fill && cfb && (oml & ~7u) == ((G_RM_VISCVG | G_RM_VISCVG2) & ~7u)) return ROLE_CPUFILL;
    if (siz == 2 && (zero || inz) && (((omh >> G_MDSFT_CYCLETYPE) & 3) == 3 || (oml & 0x30u)) && !zc) return ROLE_DEPTH;
    if (img_null && !zc) return ROLE_MAIN;
    if (native && siz == 2) return ROLE_MAIN;
    return ROLE_SLOT;
}

int main(void) {
    unsigned n = 0, bad = 0;
    for (unsigned k = 0; k < 64; k++)
        for (unsigned siz = 0; siz < 4; siz++)
            for (int ff = 0; ff < 2; ff++)
                for (unsigned cyc = 0; cyc < 4; cyc++)
                    for (int vis = 0; vis < 2; vis++)
                        for (int b30 = 0; b30 < 2; b30++)
                            for (int zc = 0; zc < 2; zc++) {
                                uint32_t oml = (vis ? (uint32_t)(G_RM_VISCVG | G_RM_VISCVG2) : 0u) | (b30 ? 0x30u : 0u);
                                uint32_t omh = cyc << G_MDSFT_CYCLETYPE;
                                int want = ref_role(!!(k & NDK_NULL), !!(k & NDK_ZERO), !!(k & NDK_DEPTHQ), !!(k & NDK_CFB), !!(k & NDK_INZ), !!(k & NDK_NATIVE), (int)siz, ff, oml, omh, zc);
                                int got = nd_tgt_role(k, siz, ff, oml, omh, zc);
                                n++;
                                if (want != got) { if (bad < 5) printf("diff kind %u siz %u ff %d cyc %u vis %d b30 %d zc %d: want %d got %d\n", k, siz, ff, cyc, vis, b30, zc, want, got); bad++; }
                            }
    if (bad) { printf("ntgt FAIL %u of %u\n", bad, n); return 1; }
    printf("ntgt ok %u\n", n);
    return 0;
}
