/* nd_ds.h: the core 1 draw state NdDs (design_nstate.md section 3, row NST1). NdDs = the walker state NdWs plus the globals of the core 0
 * setters (same names and types). nd_ds_apply copies the state writes of nd_op_apply (nd_set.c, nd_comb.c, nd_tex_set.c, nd_mtx_set.c)
 * without the GPU calls. Core 1 owns nd_ds; core 0 keeps nd_ws and the globals. Host-built (nd_ds_test.c). */
#ifndef ND_DS_H
#define ND_DS_H
#include <stdint.h>
#include "nd.h"
#include "nd_int.h"

typedef struct NdDs {
    NdWs w;
    unsigned conv_k4;              /* nd_comb.c */
    unsigned keyc, keys;
    int dual_on, box_prod, comb_set, comb_alpha_shade;
    int sci_x0, sci_y0, sci_x1, sci_y1; /* nd_set.c */
    int fog_fm, fog_fo, fogmix_on;
    unsigned fogc, blendc;
    NdLight lt[9];                 /* nd_mtx_set.c */
    unsigned lt_n;
    int8_t lk[2][3];               /* look-at vectors */
    float vp_sx, vp_sy, vp_sz, vp_tx, vp_ty, vp_tz;
    NdTexM tm;                     /* nd_tex_set.c nd_tm: the load side of the texture model */
} NdDs;
extern NdDs nd_ds;
/* Applies one typed unit (w0 = id << 24 | a, w1 = b). Bit 0 of the result: a field value changed. Other ids change nothing. */
unsigned nd_ds_apply(NdDs* s, uint32_t k, uint32_t v);
/* NST2A: applies the state units of a static list (depth 0 from nd_rec_dl, 1 from an NDO_CALL unit of nd_rec_nr); see nd_ds.c. */
void nd_ds_list(NdDs* s, const void* l, int depth);
/* NST3 dschk: the count of differing fields of two NdDs copies (draw-time fields zeroed first); the first max names into bad. */
int nd_ds_cmp(const NdDs* a, const NdDs* b, const char** bad, int max);
#endif
