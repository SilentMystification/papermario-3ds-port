/* nd_val.c: the value calls of a call site (nd_val.h). Each call records its typed unit into the open native record run (nd_rec_nr), the
 * unit the nd_gbi.h redirect records for the macro, so the frame stream is the one the macro gave (CODEMODW1: custom gfx ranges read by
 * nd_r11_walk). */
#include <stdint.h>
#include "ultra64.h"
#include "nd_op.h"
#include "nd_api.h"
#include "nd_cmd.h"
#include "nd_nr.h"
#include "nd_at.h"
#include "nd_val.h"
#include "nd_fs.h"

#define VAL_COL(r, g, b, a) (((uint32_t)(r) & 0xffu) << 24 | ((uint32_t)(g) & 0xffu) << 16 | ((uint32_t)(b) & 0xffu) << 8 | ((uint32_t)(a) & 0xffu))

static void val_rec(uint32_t id, uint32_t a, uint32_t b) {
    nd_nr(ND_FS_W0(id, a), b);
}

void nd_val_prim(unsigned minlod, unsigned lodfrac, unsigned r, unsigned g, unsigned b, unsigned a) {
    NDVAL_PRIM(nd_nr, minlod, lodfrac, r, g, b, a);
}
void nd_val_env(unsigned r, unsigned g, unsigned b, unsigned a) {
    NDVAL_ENV(nd_nr, r, g, b, a);
}
void nd_val_seg(unsigned seg, uint32_t base) {
    /* the typed form of G_MOVEWORD G_MW_SEGMENT at offset seg * 4 (ND_MW_A); a segment above 15 is not a segment op and is dropped like the macro's */
    if (seg < 16u) val_rec(ND_FS_MW, ((uint32_t)ND_FS_MW_SEG << 16) | seg, base);
}
void nd_val_tile_size(unsigned tile, unsigned uls, unsigned ult, unsigned lrs, unsigned lrt) {
    val_rec(ND_FS_TILESIZE, ((uls & 0xfffu) << 12) | (ult & 0xfffu), ((tile & 7u) << 24) | ((lrs & 0xfffu) << 12) | (lrt & 0xfffu));
}
