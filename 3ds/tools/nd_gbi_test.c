/* nd_gbi_test.c: host test of the bridge form of nd_gbi.h (plan_native_slices S2-1, gate G1).
 * 1. Dual write: the list the wrapped macros write equals the list the plain gbi.h macros write (nd_gbi_ref.c), word for word.
 * 2. Native state: nd_ws (after nd_words of the plain words) holds the values the sequence set (geometry mode, other mode, colours, prim depth, texture, segment).
 * 3. The opcode numbers of nd_state.c equal the gbi.h values. Usage: nd_gbi_test (no arguments). */
#include <stdio.h>
#include <string.h>
#include "ultra64.h"
#include "nd_gbi.h"
Mtx seq_mtx;
Vp seq_vp;
unsigned char seq_img[8192];
#define ND_SEQ_NAME nd_seq_wrapped
#include "nd_gbi_seq.inc"
Gfx* nd_seq_ref(Gfx* gp);

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

int main(void) {
    static Gfx a[256], b[256];
    memset(&nd_ws, 0, sizeof nd_ws);
    Gfx* ea = nd_seq_wrapped(a);
    Gfx* eb = nd_seq_ref(b);
    unsigned na = (unsigned)(ea - a), nb = (unsigned)(eb - b);
    CHECK(na == nb && na > 40, "word count %u wrapped, %u plain", na, nb);
    for (unsigned i = 0; i < na && i < nb; i++)
        if (a[i].words.w0 != b[i].words.w0 || a[i].words.w1 != b[i].words.w1) { CHECK(0, "word %u: %08x %08x vs %08x %08x", i, a[i].words.w0, a[i].words.w1, b[i].words.w0, b[i].words.w1); break; }
    /* the reference decode of the plain words into nd_ws (CORE-2a-3: nd_ws is the state, nd_words the host decode) */
    memset(&nd_ws, 0, sizeof nd_ws);
    nd_words((const unsigned*)b, nb);
    const NdWs* s = &nd_ws;
    CHECK(s->geom == (G_ZBUFFER | G_CULL_BACK), "geom %08x want %08x", s->geom, (unsigned)(G_ZBUFFER | G_CULL_BACK));
    CHECK(s->prim == 0x11223344u && ((s->prim_lod >> 8) & 0xFF) == 5 && (s->prim_lod & 0xFF) == 6, "prim %08x lod %04x", s->prim, s->prim_lod);
    CHECK(s->envc == 0x55667788u && s->fill == 0xAABBCCDDu, "env %08x fill %08x", s->envc, s->fill);
    CHECK(s->prim_z == 0x7FFF, "prim depth %u", s->prim_z);
    CHECK(((s->omode_h >> G_MDSFT_CYCLETYPE) & 3) == (G_CYC_2CYCLE >> G_MDSFT_CYCLETYPE), "cycle type %08x", s->omode_h);
    CHECK(((s->omode_h >> G_MDSFT_TEXTFILT) & 3) == (G_TF_BILERP >> G_MDSFT_TEXTFILT), "texture filter %08x", s->omode_h);
    CHECK(((s->omode_h >> G_MDSFT_TEXTLUT) & 3) == (G_TT_RGBA16 >> G_MDSFT_TEXTLUT), "texture lut %08x", s->omode_h);
    CHECK((s->omode_l & 3) == G_AC_THRESHOLD, "alpha compare %08x", s->omode_l);
    CHECK((s->omode_l & 0xFFFFFFF8u) == ((G_RM_AA_ZB_OPA_SURF | G_RM_AA_ZB_OPA_SURF2) & 0xFFFFFFF8u), "render mode %08x", s->omode_l);
    CHECK(s->om_kh != 0 && s->om_kl != 0, "known bits %08x %08x", s->om_kh, s->om_kl);
    CHECK(s->tex_on && s->tex_ss == 0xFFFF && s->tex_ts == 0xFFFF, "texture state");
    CHECK(s->segbase[4] == 0x01234567u, "segment");
    /* opcode numbers used by nd_state.c */
    CHECK(G_VTX == 0x01 && G_TRI1 == 0x05 && G_TRI2 == 0x06 && G_QUAD == 0x07 && G_TEXTURE == 0xD7 && G_POPMTX == 0xD8 && G_GEOMETRYMODE == 0xD9 && G_MTX == 0xDA &&
          G_MOVEWORD == 0xDB && G_MOVEMEM == 0xDC && G_DL == 0xDE && G_ENDDL == 0xDF && G_SETOTHERMODE_L == 0xE2 && G_SETOTHERMODE_H == 0xE3 && G_TEXRECT == 0xE4 &&
          G_SETSCISSOR == 0xED && G_SETPRIMDEPTH == 0xEE && G_SETTILESIZE == 0xF2 && G_LOADBLOCK == 0xF3 && G_LOADTILE == 0xF4 && G_SETTILE == 0xF5 && G_FILLRECT == 0xF6 &&
          G_SETFILLCOLOR == 0xF7 && G_SETFOGCOLOR == 0xF8 && G_SETBLENDCOLOR == 0xF9 && G_SETPRIMCOLOR == 0xFA && G_SETENVCOLOR == 0xFB && G_SETCOMBINE == 0xFC &&
          G_SETTIMG == 0xFD && G_SETZIMG == 0xFE && G_SETCIMG == 0xFF, "opcode constants differ from gbi.h");
    printf("nd_gbi: %u words %s\n", na, fails ? "FAIL" : "PASS");
    return fails != 0;
}
