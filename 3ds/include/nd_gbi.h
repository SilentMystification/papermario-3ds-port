/* nd_gbi.h: the bridge form of the gSP and gDP macros (plan_native_system 5 and 6, S2-1). A source file opts in with a force include (CMake, S2-2).
 * Every leaf macro still writes its N64 words (the walker runs them) and also decodes them into the native state (nd_words, nd_st).
 * Compound and alias macros are untouched: they expand into leaves. Leaves with no native form stop the compile. The list is generated:
 * 3ds/tools/gen_nd_gbi.sh writes nd_gbi_leaf.h from the active macros. Every argument of a leaf is evaluated twice: no side effect in an argument except in pkt. */
#ifndef ND_GBI_H
#define ND_GBI_H

#include "ultra64.h"
#include "nd.h"

#define ND_TMP_WORDS 8 /* the longest leaf is gSPClipRatio (4 words) */
#include "nd_gbi_leaf.h"

#endif
