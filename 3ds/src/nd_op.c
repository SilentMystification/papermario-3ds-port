/* nd_op.c: the translator of F3DEX2 words into typed ops (HC4, nd_op.h). The recorder runs it (core 1, nd_rec.c); host-built for
 * nd_cmd_test.c. Pure: no walker state. */
#include "ultra64.h"
#include "nd_op.h"
#include "gbi_custom.h"

const uint8_t nd_op_n64[NDO_END_ - NDO_BASE] = {
    G_SETOTHERMODE_H, G_SETOTHERMODE_L, G_GEOMETRYMODE, G_SETPRIMCOLOR, G_SETENVCOLOR, G_SETFOGCOLOR, G_SETBLENDCOLOR,
    G_SETFILLCOLOR, G_SETPRIMDEPTH, G_SETSCISSOR, G_TEXTURE, G_MOVEWORD, 0 /* NDO_RARE: the sub-op names it */, G_SETCOMBINE,
    G_ENDDL /* NDO_END */, G_SPNOOP /* NDO_DROP */, G_SETTIMG, G_SETTILE, G_SETTILESIZE, G_LOADBLOCK, G_LOADTILE, G_LOADTLUT,
    G_VTX, G_TRI1, G_TRI2, G_MTX, G_POPMTX, G_MOVEMEM, G_DL, G_FILLRECT, 0 /* NDO_MISC: the sub-op names it */
};

static int out2(uint32_t out[2], unsigned id, uint32_t a, uint32_t b) {
    out[0] = NDO_W0(id, a);
    out[1] = b;
    return 1;
}

int nd_op_xlate(uint32_t w0, uint32_t w1, uint32_t out[2]) {
    if (nd_op_typed(w0 >> 24)) { out[0] = w0; out[1] = w1; return 1; } /* HC4e1: a typed unit passes */
    switch (w0 >> 24) {
    case G_SETOTHERMODE_H:
    case G_SETOTHERMODE_L: {
        const unsigned len = (w0 & 0xffu) + 1u, field = (w0 >> 8) & 0xffu, sft = 32u - field - len;
        return out2(out, (w0 >> 24) == G_SETOTHERMODE_H ? NDO_OMH : NDO_OML, (sft & 0xffu) | (len << 8), w1);
    }
    case G_GEOMETRYMODE: return out2(out, NDO_GEOM, w0, w1);
    case G_SETPRIMCOLOR: return out2(out, NDO_PRIM, w0 & 0xffffu, w1);
    case G_SETENVCOLOR: return out2(out, NDO_ENV, 0, w1);
    case G_SETFOGCOLOR: return out2(out, NDO_FOG, 0, w1);
    case G_SETBLENDCOLOR: return out2(out, NDO_BLEND, 0, w1);
    case G_SETFILLCOLOR: return out2(out, NDO_FILL, 0, w1);
    case G_SETPRIMDEPTH: return out2(out, NDO_PRIMZ, 0, w1);
    case G_SETSCISSOR: return out2(out, NDO_SCISSOR, w0, w1 & 0xffffffu);
    case G_TEXTURE: return out2(out, NDO_TEXTURE, (w0 & 0xffu) != 0u, w1);
    case G_MOVEWORD: {
        const unsigned index = (w0 >> 16) & 0xffu, offset = w0 & 0xffffu;
        unsigned kind = NDO_MW_NONE, arg = 0;
        if (index == G_MW_NUMLIGHT) kind = NDO_MW_NUMLIGHT;
        else if (index == G_MW_LIGHTCOL && offset / 24u < 9u && offset % 24u == 0u) { kind = NDO_MW_LIGHTCOL; arg = offset / 24u; }
        else if (index == G_MW_FOG) kind = NDO_MW_FOG;
        else if (index == G_MW_SEGMENT && offset / 4u < 16u) { kind = NDO_MW_SEG; arg = offset / 4u; }
        return out2(out, NDO_MW, kind << 16 | arg, w1);
    }
    case G_SETCONVERT: return out2(out, NDO_RARE, NDO_RARE_CONV, w1);
    case G_SETKEYR: return out2(out, NDO_RARE, NDO_RARE_KEYR, w1);
    case G_SETKEYGB: return out2(out, NDO_RARE, NDO_RARE_KEYGB, w1);
    case G_SETCOMBINE: return out2(out, NDO_COMB, w0, w1);
    case G_SETTIMG: return out2(out, NDO_TIMG, w0, w1);
    case G_SETTILE: return out2(out, NDO_TILE, w0, w1);
    case G_SETTILESIZE: return out2(out, NDO_TILESIZE, w0, w1);
    case G_LOADBLOCK: return out2(out, NDO_LOADBLOCK, w0, w1);
    case G_LOADTILE: return out2(out, NDO_LOADTILE, w0, w1);
    case G_LOADTLUT: return out2(out, NDO_LOADTLUT, w0, w1);
    case G_VTX: return out2(out, NDO_VTX, w0, w1);
    case G_TRI1: return out2(out, NDO_TRI1, w0, w1);
    case G_TRI2: return out2(out, NDO_TRI2, w0, w1);
    case G_MTX: return out2(out, NDO_MTX, w0, w1);
    case G_POPMTX: return out2(out, NDO_POPMTX, w0, w1);
    case G_MOVEMEM: return out2(out, NDO_MOVEMEM, w0, w1);
    case G_DL: return out2(out, NDO_CALL, ((w0 >> 16) & 0xffu) == G_DL_NOPUSH, w1);
    case G_FILLRECT: return out2(out, NDO_RECT, w0, w1 & 0x00ffffffu);
    case G_TEXRECT:
    case G_TEXRECTFLIP: return out2(out, NDO_RECT, w0, NDO_RECT_TEX | (w1 & 0x07ffffffu));
    case G_PM_LST: return out2(out, NDO_MISC, NDO_MISC_LST << 16, 0);
    case G_PM_ND: return out2(out, NDO_MISC, NDO_MISC_ND << 16 | ((w0 >> 8) & 0xff00u) | (w0 & 0xffu), 0);
    case G_MODIFYVTX: {
        const unsigned vi = (w0 & 0xffffu) / 2u;
        return out2(out, NDO_MISC, NDO_MISC_MODVTX << 16 | ((w0 >> 8) & 0xff00u) | (vi < 255u ? vi : 255u), w1);
    }
    case G_SETCIMG: return out2(out, NDO_MISC, NDO_MISC_CIMG << 16 | ((w0 >> 7) & 0x3000u) | (w0 & 0xfffu), w1);
    case G_NOOP:
    case G_RDPHALF_1:
    case G_RDPHALF_2:
    case G_SETZIMG: return out2(out, NDO_MISC, NDO_MISC_NOP << 16, w1);
    default:
        out[0] = w0;
        out[1] = w1;
        return 0;
    }
}

unsigned nd_op_type_run(uint32_t* w, unsigned n) {
    unsigned k = 0;
    for (unsigned i = 0; i < n; i++) {
        const uint32_t w0 = w[2u * i], w1 = w[2u * i + 1u], op = w0 >> 24;
        if ((op >= G_RDPLOADSYNC && op <= G_RDPFULLSYNC) || op == G_SPNOOP) continue;
        if (op == G_ENDDL) { w[2u * k] = NDO_W0(NDO_END, 0); w[2u * k + 1u] = 0; break; }
        nd_op_xlate(w0, w1, w + 2u * k);
        k++;
    }
    return k;
}
