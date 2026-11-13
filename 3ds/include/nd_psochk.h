/* nd_psochk.h: NPSO4, the compare of a stated NdPsoDesc with the walker's description (host and device). */
#ifndef ND_PSOCHK_H
#define ND_PSOCHK_H
#include "nd_api.h"

/* 0 when every field of walker equals stated; else the count of differing fields, and in out the first one as
 * "<field> stated <v> walker <v>" (words in hex, bytes in hex). n: the size of out. */
int nd_psochk_cmp(const NdPsoDesc* stated, const NdPsoDesc* walker, char* out, unsigned n);
void nd_psochk_draw(const NdPsoDesc* stated); /* nd_comb.c, device: the psochk debug switch (one test when off) */

#endif
