/* nd_op.c: the translator of F3DEX2 words into typed ops (HC4, nd_op.h). HC4e4: host only (not in the ELF); the lists are typed at
 * their producers. Pure: no walker state. */
#include "ultra64.h"
#include "nd_op.h"
#include "gbi_custom.h"

static int out2(uint32_t out[2], unsigned id, uint32_t a, uint32_t b) {
    out[0] = ND_FS_W0(id, a);
    out[1] = b;
    return 1;
}

int nd_op_xlate(uint32_t w0, uint32_t w1, uint32_t out[2]) {
    if (nd_op_typed(w0 >> 24)) { out[0] = w0; out[1] = w1; return 1; } /* HC4e1: a typed unit passes */
    switch (w0 >> 24) {
    case G_SETOTHERMODE_H:
    case G_SETOTHERMODE_L: {
        const unsigned len = (w0 & 0xffu) + 1u, field = (w0 >> 8) & 0xffu, sft = 32u - field - len;
        return out2(out, (w0 >> 24) == G_SETOTHERMODE_H ? ND_FS_OMH : ND_FS_OML, (sft & 0xffu) | (len << 8), w1);
    }
    case G_GEOMETRYMODE: return out2(out, ND_FS_GEOM, w0, w1);
    case G_SETPRIMCOLOR: return out2(out, ND_FS_PRIM, w0 & 0xffffu, w1);
    case G_SETENVCOLOR: return out2(out, ND_FS_ENV, 0, w1);
    case G_SETFOGCOLOR: return out2(out, ND_FS_FOG, 0, w1);
    case G_SETBLENDCOLOR: return out2(out, ND_FS_BLEND, 0, w1);
    case G_SETFILLCOLOR: return out2(out, ND_FS_FILL, 0, w1);
    case G_SETPRIMDEPTH: return out2(out, ND_FS_PRIMZ, 0, w1);
    case G_SETSCISSOR: return out2(out, ND_FS_SCISSOR, w0, w1 & 0xffffffu);
    case G_TEXTURE: return out2(out, ND_FS_TEXTURE, (w0 & 0xffu) != 0u, w1);
    case G_MOVEWORD: {
        const unsigned index = (w0 >> 16) & 0xffu, offset = w0 & 0xffffu;
        unsigned kind = ND_FS_MW_NONE, arg = 0;
        if (index == G_MW_NUMLIGHT) kind = ND_FS_MW_NUMLIGHT;
        else if (index == G_MW_LIGHTCOL && offset / 24u < 9u && offset % 24u == 0u) { kind = ND_FS_MW_LIGHTCOL; arg = offset / 24u; }
        else if (index == G_MW_FOG) kind = ND_FS_MW_FOG;
        else if (index == G_MW_SEGMENT && offset / 4u < 16u) { kind = ND_FS_MW_SEG; arg = offset / 4u; }
        return out2(out, ND_FS_MW, kind << 16 | arg, w1);
    }
    case G_SETCONVERT: return out2(out, ND_FS_RARE, ND_FS_RARE_CONV, w1);
    case G_SETKEYR: return out2(out, ND_FS_RARE, ND_FS_RARE_KEYR, w1);
    case G_SETKEYGB: return out2(out, ND_FS_RARE, ND_FS_RARE_KEYGB, w1);
    case G_SETCOMBINE: return out2(out, ND_FS_COMB, w0, w1);
    case G_SETTIMG: return out2(out, ND_FS_TIMG, w0, w1);
    case G_SETTILE: return out2(out, ND_FS_TILE, w0, w1);
    case G_SETTILESIZE: return out2(out, ND_FS_TILESIZE, w0, w1);
    case G_LOADBLOCK: return out2(out, ND_FS_LOADBLOCK, w0, w1);
    case G_LOADTILE: return out2(out, ND_FS_LOADTILE, w0, w1);
    case G_LOADTLUT: return out2(out, ND_FS_LOADTLUT, w0, w1);
    case G_VTX: return out2(out, ND_FS_VTX, w0, w1);
    case G_TRI1: return out2(out, ND_FS_TRI1, w0, w1);
    case G_TRI2: return out2(out, ND_FS_TRI2, w0, w1);
    case G_MTX: return out2(out, ND_FS_MTX, w0, w1);
    case G_POPMTX: return out2(out, ND_FS_POPMTX, w0, w1);
    case G_MOVEMEM: return out2(out, ND_FS_MOVEMEM, w0, w1);
    case G_DL: return out2(out, ND_FS_CALL, ((w0 >> 16) & 0xffu) == G_DL_NOPUSH, w1);
    case G_FILLRECT: return out2(out, ND_FS_RECT, w0, w1 & 0x00ffffffu);
    case G_TEXRECT:
    case G_TEXRECTFLIP: return out2(out, ND_FS_RECT, w0, ND_FS_RECT_TEX | (w1 & 0x07ffffffu));
    case G_PM_LST: return out2(out, ND_FS_MISC, ND_FS_MISC_LST << 16, 0);
    case G_PM_ND: return out2(out, ND_FS_MISC, ND_FS_MISC_ND << 16 | ((w0 >> 8) & 0xff00u) | (w0 & 0xffu), 0);
    case G_MODIFYVTX: {
        const unsigned vi = (w0 & 0xffffu) / 2u;
        return out2(out, ND_FS_MISC, ND_FS_MISC_MODVTX << 16 | ((w0 >> 8) & 0xff00u) | (vi < 255u ? vi : 255u), w1);
    }
    case G_SETCIMG: return out2(out, ND_FS_MISC, ND_FS_MISC_CIMG << 16 | ((w0 >> 7) & 0x3000u) | (w0 & 0xfffu), w1);
    case G_NOOP:
    case G_RDPHALF_1:
    case G_RDPHALF_2:
    case G_SETZIMG: return out2(out, ND_FS_MISC, ND_FS_MISC_NOP << 16, w1);
    default:
        out[0] = w0;
        out[1] = w1;
        return 0;
    }
}
