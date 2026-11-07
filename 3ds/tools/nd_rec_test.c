/* nd_rec_test.c: host test of the record form of the leaves (nd_rec.h, plan_s10_path CORE-1, CORE-2c). Build with nd_rec_bridge.c, nd_gbi_ref.c and nd_state.c.
 * 1. The main list gets the record words: the record-mode sequence (pkt in gDisplayContext->mainGfx) writes the same words as the plain macros.
 * 2. Slot count: the record-mode sequence writes as many slots as the plain macros (nd_gbi_seq.inc).
 * 3. Other pkt: the same words, and nd_words gives the same nd_ws state for both.
 * Usage: nd_rec_test (no arguments). Exit 1 on a fail. */
#include <stdio.h>
#include <string.h>
#include "ultra64.h"
#include "nd_rec.h"

typedef struct { Gfx mainGfx[512]; } FakeDC; /* the main list buffer, as gDisplayContext->mainGfx */
static FakeDC fdc;
FakeDC* gDisplayContext = &fdc;

Gfx* nd_seq_bridge(Gfx* gp);
Gfx* nd_seq_ref(Gfx* gp);
#define ND_SEQ_NAME nd_seq_rec
#include "nd_gbi_seq.inc"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static Gfx wa[512], wc[512], wd[512];

int main(void) {
    unsigned* mw = (unsigned*)fdc.mainGfx;
    Gfx* e1 = nd_seq_rec(fdc.mainGfx);
    unsigned n1 = (unsigned)(e1 - fdc.mainGfx);
    CHECK(n1 > 40 && n1 < 500, "slot count %u", n1);

    Gfx* eb = nd_seq_bridge(wa);
    unsigned nb = (unsigned)(eb - wa);
    CHECK(nb == n1, "slots: record %u, plain %u", n1, nb);
    unsigned diff = 0;
    for (unsigned i = 0; i < 2 * n1; i++) if (mw[i] != ((unsigned*)wa)[i]) diff++;
    CHECK(diff == 0, "main list words differ from the plain words: %u", diff);

    Gfx* ec = nd_seq_rec(wc);
    unsigned nc = (unsigned)(ec - wc);
    Gfx* ed = nd_seq_ref(wd);
    unsigned nd = (unsigned)(ed - wd);
    CHECK(nc == nd, "other pkt slots: record %u, plain %u", nc, nd);
    for (unsigned i = 0; i < nc && i < nd; i++)
        if (wc[i].words.w0 != wd[i].words.w0 || wc[i].words.w1 != wd[i].words.w1) { CHECK(0, "other pkt word %u", i); break; }

    static NdWs s_rec, s_ref;
    memset(&nd_ws, 0, sizeof nd_ws); nd_words((const unsigned*)wc, nc); s_rec = nd_ws;
    memset(&nd_ws, 0, sizeof nd_ws); nd_words((const unsigned*)wd, nd); s_ref = nd_ws;
    CHECK(memcmp(&s_rec, &s_ref, sizeof s_rec) == 0, "nd_ws differs for other pkt");

    printf("nd_rec: %u slots, main list words equal plain, state %s\n", n1, fails ? "FAIL" : "PASS");
    return fails != 0;
}
