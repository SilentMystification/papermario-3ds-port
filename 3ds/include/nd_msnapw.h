/* nd_msnapw.h: the model snap words that model_draw reads (NREC1B). model_draw takes its walker inputs from the snap of the record that runs
 * (nd_msnap_use, nd_recs.c), not from nd_ws. The indices are the word indices of NdMSnap (nd_msnap.h). */
#ifndef ND_MSNAPW_H
#define ND_MSNAPW_H
#include <stdint.h>
enum { MSW_PRIM = 1, MSW_PLOD, MSW_OMH, MSW_OML, MSW_KH, MSW_KL, MSW_TSS, MSW_GEOM, MSW_CW0, MSW_CW1, MSW_FOGC, MSW_BLENDC, MSW_ENVC };
struct NdTd;
void nd_msnap_use(const uint32_t* q);
const uint32_t* nd_msnap_w(void);
const struct NdTd* nd_msnap_td0(void);
#endif
