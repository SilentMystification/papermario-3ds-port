/* WCLIP1: x edges of the game scissor as screen columns (before the 400 - x flip of stage_scissor).
 * x0, x1 raw (not clamped), y0, y1 clamped by the caller.
 * mode 0 Native, 1 Widescreen, 2 Fullscreen; ui: a 2D or orthographic batch (1:1 at 40 + x in Widescreen and Fullscreen). */
#include "nd_sci.h"

static int ci(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

/* the full native screen (both decomp forms) or its safe area */
int nd_sci_full(int x0, int y0, int x1, int y1) {
    return (x0 == 0 && y0 == 0 && (x1 == 320 || x1 == 319) && (y1 == 240 || y1 == 239)) || (x0 == 12 && y0 == 20 && x1 == 308 && y1 == 220);
}

void nd_sci_x(int x0, int y0, int x1, int y1, int mode, int ui, int* top, int* bottom) {
    if (x1 < x0) { const int t = x0; x0 = x1; x1 = t; }
    if (mode == 0) { *top = 40 + ci(x0, 0, 320); *bottom = 40 + ci(x1, 0, 320); return; }
    if (nd_sci_full(x0, y0, x1, y1)) { *top = 0; *bottom = 400; return; }
    const int s = ui; /* FSUI1: Widescreen and Fullscreen: a 2D or orthographic batch is 1:1 at 40 + x */
    *top = x0 == 0 ? 0 : s ? ci(40 + x0, 0, 400) : ci(x0, 0, 320) * 5 / 4;
    *bottom = (x1 == 319 || x1 == 320) ? 400 : s ? ci(40 + x1, 0, 400) : ci(x1, 0, 320) * 5 / 4;
}
