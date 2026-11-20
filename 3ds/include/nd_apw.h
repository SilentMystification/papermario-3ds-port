/* nd_apw.h: the applied snap (nd_ap, nd_snap.c) for the files that do not include nd_int.h (pm_gpu.c). NWS4B */
#ifndef ND_APW_H
#define ND_APW_H
#include <stdint.h>
enum { NDAPW_W0, NDAPW_W1, NDAPW_OH, NDAPW_OL, NDAPW_PRIM, NDAPW_PLOD, NDAPW_ENV, NDAPW_FOGC, NDAPW_DUAL, NDAPW_UT1, NDAPW_KH, NDAPW_KL };
uint32_t nd_apw(int k); /* one word of the snap core 0 applied last */
typedef struct NdGvC { uint32_t w0, w1, env, oml, primz; int use_t1, dual_on; } NdGvC; /* NWS4C: the combine inputs of pm_gbi_gv_at: the applied snap (nd_apw_gvc) or the model snap (nd_msnap_gvc) */
void nd_apw_gvc(NdGvC* c); void nd_msnap_gvc(NdGvC* c);
struct NdSnapBind; extern const struct NdSnapBind* nd_bsrc; /* NC0C: the tile blocks the binds read (nd_snap.h) */
const struct NdSnapBind* nd_apw_b(void); void nd_bsrc_chk(const struct NdSnapBind* b, unsigned site);
#endif
