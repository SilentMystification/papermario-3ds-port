/* nd_ds.h: the core 1 draw state NdDs (design_nstate.md section 3, row NST1). NdDs = the walker state NdWs plus the globals of the core 0
 * setters (same names and types). nd_ds_apply copies the state writes of nd_op_apply (nd_set.c, nd_comb.c, nd_tex_set.c, nd_mtx_set.c)
 * without the GPU calls. Core 1 owns nd_ds; core 0 keeps nd_ws and the globals. Host-built (nd_ds_test.c). */
#ifndef ND_DS_H
#define ND_DS_H
#include <stdint.h>
#include "nd.h"
#include "nd_int.h"
#include "nd_fs.h"

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
    int m1v, m1w, m1h;             /* NST2F: the shaded sprite record of tile 1 (m1.valid, m1.w, m1.h) */
    uint8_t sc_lost, sc_dirty;     /* NST2J: core 1 view of !sc_ok and sc_sig != sc_sig_now() (nd_comb.c:342); not in ds_fields */
} NdDs;
extern NdDs nd_ds;
/* Applies one typed unit (w0 = id << 24 | a, w1 = b). Bit 0 of the result: a field value changed. Other ids change nothing. */
unsigned nd_ds_apply(NdDs* s, uint32_t k, uint32_t v);
/* NST2A: applies the state units of a static list (depth 0 from nd_rec_dl, 1 from an ND_FS_CALL unit of nd_rec_nr); see nd_ds.c. */
void nd_ds_list(NdDs* s, const void* l, int depth);
/* NST2b: applies one typed unit of a record (the ds_unit of nd_rec.c: nd_ds_apply, the ND_FS_CALL list, the ep rule). */
typedef void (*NdDsUnitFn)(NdDs* s, uint32_t w0, uint32_t w1);
/* NST2b: the state units of one model record q (the payload, pair Rk = q[2k], q[2k+1]), its R13 units and the render mode pairs rm (nrm). */
void nd_ds_model(NdDs* s, const uint32_t* q, const uint32_t* rm, unsigned nrm, NdDsUnitFn unit);
/* NST2C: one typed unit into the core 1 state (ds_unit of nd_rec.c: nd_ds_apply, the ND_FS_CALL list, the ep rule). */
void nd_ds_unit(NdDs* s, uint32_t w0, uint32_t w1);
unsigned nd_ds_ep_rule(uint32_t w0, unsigned ch, int lost, int dirty); void nd_lsc_ep(void); /* NCONV2C: nd_ds.c; nd_snap.c */
/* NST2C: the state units of a record fed to core 1 (nd_rec.c: each unit by nd_ds_unit; syncs skipped). */
void nd_ds_feed(const uint32_t* u, unsigned n);
/* NST2C: the units of a parts record (nd_parts_end layout: u[0] = n | pre << 8 | m << 16); the list call at index pre has no state. */
void nd_ds_parts(NdDs* s, const uint32_t* u, NdDsUnitFn unit);
/* NST2C: the units of a compiled def (rec_ops order: value units from v, syncs skipped). */
void nd_ds_scomp(NdDs* s, const uint32_t* du, unsigned n, uint32_t vmask, const uint32_t* v, unsigned nv, NdDsUnitFn unit);
/* NST2D: the unit log (nd_ds.c). lg_reset at the frame start; open/close bracket a range; mark records a range bound; sub_save and
 * sub_restore hold the state across a sub; range replays the units between two marks into s (b NULL: the next mark). */
void nd_ds_lg_reset(void);
void nd_ds_lg_open(void);
void nd_ds_lg_close(void);
void nd_ds_mark(const uint32_t* pos);
void nd_ds_sub_save(void);
void nd_ds_sub_restore(void);
void nd_ds_range(NdDs* s, const uint32_t* a, const uint32_t* b);
extern unsigned nd_ds_lg_over, nd_ds_lg_hw; /* NST2D: units dropped (log full, bad bound), log high water */
extern unsigned nd_ds_rpl;                  /* NST2D: range replay depth */
/* NST3 dschk: the count of differing fields of two NdDs copies (draw-time fields zeroed first); the first max names into bad. */
int nd_ds_cmp(const NdDs* a, const NdDs* b, const char** bad, int max);
#endif
