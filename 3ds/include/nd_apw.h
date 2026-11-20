/* nd_apw.h: the applied snap (nd_ap, nd_snap.c) for the files that do not include nd_int.h (pm_gpu.c), and the drec shadow check ND_WCHK. NWS4B */
#ifndef ND_APW_H
#define ND_APW_H
#include <stdint.h>
enum { NDAPW_W0, NDAPW_W1, NDAPW_OH, NDAPW_OL, NDAPW_PRIM, NDAPW_PLOD, NDAPW_ENV, NDAPW_FOGC, NDAPW_DUAL, NDAPW_UT1 };
uint32_t nd_apw(int k); /* one word of the snap core 0 applied last */
void nd_wsnap_chk(void); int pm_drec_now(void);
#define ND_WCHK() do { if (__builtin_expect(pm_drec_now(), 0)) nd_wsnap_chk(); } while (0) /* NWS4B: drec frames only */
#endif
