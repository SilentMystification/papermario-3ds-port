/* nd_rec_test.c: host test of the record-mode leaves (nd_rec.h, plan_s10_path CORE-1). Build with nd_rec_bridge.c, nd_gbi_ref.c and nd_state.c.
 * 1. A main list slot gets no word: the record-mode sequence (pkt in gDisplayContext->mainGfx) leaves the list words at their fill value.
 * 2. Native state: nd_st after the record-mode sequence equals nd_st after the bridge sequence (every leaf of nd_gbi_seq.inc), and the slot count equals.
 * 3. Other pkt: the record-mode sequence writes the same words as the plain gbi.h macros (nd_gbi_ref.c), and the same state.
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
#define FILL 0xA5A5A5A5u

static Gfx wa[512], wc[512], wd[512];

int main(void) {
    unsigned* mw = (unsigned*)fdc.mainGfx;
    for (unsigned i = 0; i < 2 * 512; i++) mw[i] = FILL;
    nd_st_reset();
    Gfx* e1 = nd_seq_rec(fdc.mainGfx);
    unsigned n1 = (unsigned)(e1 - fdc.mainGfx);
    NdSt s1 = nd_st;
    unsigned changed = 0;
    for (unsigned i = 0; i < 2 * 512; i++) if (mw[i] != FILL) changed++;
    CHECK(changed == 0, "main list words written by record mode: %u", changed);
    CHECK(n1 > 40 && n1 < 500, "slot count %u", n1);

    nd_st_reset();
    Gfx* eb = nd_seq_bridge(wa);
    unsigned nb = (unsigned)(eb - wa);
    NdSt s2 = nd_st;
    CHECK(nb == n1, "slots: record %u, bridge %u", n1, nb);
    CHECK(memcmp(&s1, &s2, sizeof s1) == 0, "native state differs between record mode and bridge");

    nd_st_reset();
    Gfx* ec = nd_seq_rec(wc);
    unsigned nc = (unsigned)(ec - wc);
    NdSt s3 = nd_st;
    Gfx* ed = nd_seq_ref(wd);
    unsigned nd = (unsigned)(ed - wd);
    CHECK(nc == nd, "other pkt slots: record %u, plain %u", nc, nd);
    for (unsigned i = 0; i < nc && i < nd; i++)
        if (wc[i].words.w0 != wd[i].words.w0 || wc[i].words.w1 != wd[i].words.w1) { CHECK(0, "other pkt word %u: %08x %08x vs %08x %08x", i, wc[i].words.w0, wc[i].words.w1, wd[i].words.w0, wd[i].words.w1); break; }
    CHECK(memcmp(&s2, &s3, sizeof s2) == 0, "native state differs for other pkt");

    printf("nd_rec: %u slots, main list writes 0, state %s\n", n1, fails ? "FAIL" : "PASS");
    return fails != 0;
}
