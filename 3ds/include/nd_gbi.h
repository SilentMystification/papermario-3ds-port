/* nd_gbi.h: the device word helpers of the gbi macros (ND_FS form, the typed units of nd_op.h).
 * The macro redirect is 3ds/tools/gbi_inc/nd_gbi_host.h: host compiles only (included at the end of this file). */
#ifndef ND_GBI_H
#define ND_GBI_H
#include "nd_cmd.h"
#include "nd_op.h"
#include "nd_fs.h"
#define ND_W0_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
    ND_FS_COMB_W0(GCCc0w0(G_CCMUX_##a0, G_CCMUX_##c0, G_ACMUX_##Aa0, G_ACMUX_##Ac0) | GCCc1w0(G_CCMUX_##a1, G_CCMUX_##c1)) /* DEL3C2: the 0xFC combine word */
#define ND_W1_CMB(a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1) \
    ((unsigned int)(GCCc0w1(G_CCMUX_##b0, G_CCMUX_##d0, G_ACMUX_##Ab0, G_ACMUX_##Ad0) | \
                    GCCc1w1(G_CCMUX_##b1, G_ACMUX_##Aa1, G_ACMUX_##Ac1, G_CCMUX_##d1, G_ACMUX_##Ab1, G_ACMUX_##Ad1)))
/* WSCMB: a combine unit (or a no-op unit) as an NdCmbW value (include/common_structs.h); the words of gsDPSetCombineMode and gsDPNoOp */
#define ND_CMB_W_(w0, w1) { ND_FS_W0(ND_FS_COMB, (w0)), (w1) } /* DEL3C2: the typed combine unit (the nd_op.h units of the combine word) */
#define ND_CMB_W(...) ND_CMB_W_(ND_W0_CMB(__VA_ARGS__), ND_W1_CMB(__VA_ARGS__))
#define ND_CMB_NOP { ND_NOP_A, 0u }
#define ND_W0_D2(c, len, idx, ofs) (_SHIFTL((c), 24, 8) | _SHIFTL(((len) - 1) / 8, 19, 5) | _SHIFTL((ofs) / 8, 8, 8) | _SHIFTL((idx), 0, 8))
#define ND_W0_PMND(kind, arg) (((u32)G_PM_ND << 24) | ((u32)(kind) << 16) | (arg))
#define ND_W0_TR(op, xh, yh) (_SHIFTL(op, 24, 8) | _SHIFTL(xh, 12, 12) | _SHIFTL(yh, 0, 12))
#define ND_W1_TR(tile, xl, yl) (_SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) | _SHIFTL(yl, 0, 12))
#define ND_TR_H1(s, t) (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16))
#define ND_TR_H2(dsdx, dtdy) (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16))
#ifndef __arm__ /* DEL3C2: the host compiles (gfx table converter TUs, unit tests) keep the gbi macro redirect; the device build has none */
#include "../tools/gbi_inc/nd_gbi_host.h"
#endif
#endif
