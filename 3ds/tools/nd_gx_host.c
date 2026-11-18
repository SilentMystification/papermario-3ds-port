/* DEL3C1: the gbi macro redirect of the host unit tests (nd_gx_w, the top-level packet). The game writes no gbi macro. */
#include <stdint.h>
#include "ultra64.h"
#include "pm_port.h"
#include "nd_cmd.h"
#include "nd_op.h"
#include "nd_fs.h"
#include "nd_gbi.h"
extern Gfx* gMainGfxPos; /* the test defines it */
#define IS_NOP(w0) ((w0) == ND_NOP_A)
Gfx nd_rec_top[8];
Gfx* nd_rec_task(void) { return nd_rec_top; }
static uint32_t tr_w[3];
static unsigned tr_n; /* the words of the texture rectangle being recorded */
static __attribute__((cold, noinline)) void tr_bad(uint32_t w0) {
    pm_log_e("ndc: texture rectangle word %u followed by %08x", tr_n, (unsigned)w0);
    pm_fatal_stop("ndc: broken texture rectangle");
}
void nd_gx_w(Gfx* g, uint32_t w0, uint32_t w1) {
    if (g != gMainGfxPos - 1) { g->words.w0 = w0; g->words.w1 = w1; return; }
    const unsigned op = w0 >> 24;
    if (op == ND_FS_CALL && (w0 & 1u)) { g->words.w0 = w0; g->words.w1 = w1; return; } /* a branch: a unit of the list */
    gMainGfxPos = g;
    if (op == ND_FS_END) return; /* D7: the end of a task list or of a custom gfx builder (NDC_END closes both) */
    if (__builtin_expect(tr_n != 0, 0)) {
        if (tr_n == 1 && IS_NOP(w0)) { tr_w[2] = w1; tr_n = 2; return; } /* RDPHALF_1: s, t */
        if (tr_n == 2 && IS_NOP(w0)) { tr_n = 0; nd_rec_texrect(tr_w[0], tr_w[1], tr_w[2], w1); return; } /* RDPHALF_2: dsdx, dtdy */
        tr_bad(w0);
    }
    if (op == ND_FS_RECT && (w1 & ND_FS_RECT_TEX)) { /* the rectangle unit w0 and w1 without the flag (DEL1A) */
        tr_w[0] = w0; tr_w[1] = w1 & ~ND_FS_RECT_TEX; tr_n = 1; return;
    }
    if (IS_NOP(w0)) return; /* the syncs are dropped (the walker skips them) */
    if (op == ND_FS_CALL && nd_rec_inline(w1)) return;
    nd_rec_nr(w0, w1);
}
