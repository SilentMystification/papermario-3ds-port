/* nd_rec_bridge.c: the bridge form (nd_gbi.h) of nd_gbi_seq.inc, the state reference of nd_rec_test.c (CORE-1). Defines the seq_ objects. */
#include "ultra64.h"
#include "nd_gbi.h"
Mtx seq_mtx;
Vp seq_vp;
unsigned char seq_img[8192];
#define ND_SEQ_NAME nd_seq_bridge
#include "nd_gbi_seq.inc"
