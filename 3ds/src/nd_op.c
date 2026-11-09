/* nd_op.c: the translator of F3DEX2 words into typed ops (HC4, nd_op.h). The recorder runs it (core 1, nd_rec.c); host-built for
 * nd_cmd_test.c. Pure: no walker state. */
#include "ultra64.h"
#include "nd_op.h"

const uint8_t nd_op_n64[NDO_END_ - NDO_BASE] = {
    G_SETOTHERMODE_H, G_SETOTHERMODE_L, G_GEOMETRYMODE, G_SETPRIMCOLOR, G_SETENVCOLOR, G_SETFOGCOLOR, G_SETBLENDCOLOR,
    G_SETFILLCOLOR, G_SETPRIMDEPTH, G_SETSCISSOR, G_TEXTURE, G_MOVEWORD, 0 /* NDO_RARE: the sub-op names it */, G_SETCOMBINE,
    G_ENDDL /* NDO_END */, G_SPNOOP /* NDO_DROP */
};

static int out2(uint32_t out[2], unsigned id, uint32_t a, uint32_t b) {
    out[0] = NDO_W0(id, a);
    out[1] = b;
    return 1;
}

int nd_op_xlate(uint32_t w0, uint32_t w1, uint32_t out[2]) {
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
    default:
        out[0] = w0;
        out[1] = w1;
        return 0;
    }
}
