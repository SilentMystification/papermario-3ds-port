/* WCLIP1: host test of the scissor x rule (nd_sci_x, nd_sci_full). Prints "wclip1 ok" when every check passes. */
#include <stdio.h>
#include "nd_sci.h"

static int bad;

static void chk(int x0, int y0, int x1, int y1, int mode, int ui, int et, int eb) {
    int top = -1, bottom = -1;
    nd_sci_x(x0, y0, x1, y1, mode, ui, &top, &bottom);
    if (top != et || bottom != eb) {
        bad++;
        printf("FAIL x %d %d %d %d mode %d ui %d got %d %d want %d %d\n", x0, y0, x1, y1, mode, ui, top, bottom, et, eb);
    }
}

static int ci(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

int main(void) {
    /* Native (mode 0): 40 + x, raw x clamped to 0..320 */
    chk(0, 0, 320, 240, 0, 0, 40, 360);
    chk(0, 0, 319, 239, 0, 0, 40, 359);
    chk(100, 0, 200, 240, 0, 0, 140, 240);
    /* Widescreen 2D or ortho (mode 1, ui 1) */
    chk(0, 0, 320, 240, 1, 1, 0, 400);
    chk(0, 0, 319, 239, 1, 1, 0, 400);
    chk(12, 20, 308, 220, 1, 1, 0, 400);
    chk(100, 0, 319, 240, 1, 1, 140, 400);
    chk(-20, 0, 92, 240, 1, 1, 20, 132);
    chk(250, 0, 340, 240, 1, 1, 290, 380);
    chk(100, 50, 220, 190, 1, 1, 140, 260);
    chk(330, 0, 380, 240, 1, 1, 370, 400);
    /* Widescreen 3D (mode 1, ui 0) */
    chk(0, 0, 319, 239, 1, 0, 0, 400);
    chk(100, 0, 200, 240, 1, 0, 125, 250);
    /* Fullscreen (mode 2) */
    chk(0, 0, 319, 239, 2, 0, 0, 400);
    chk(12, 20, 308, 220, 2, 0, 0, 400);
    chk(100, 0, 200, 240, 2, 0, 125, 250);
    chk(-20, 0, 92, 240, 2, 0, 0, 115);

    /* grid: bounds for every mode and ui; the Widescreen ui centred rule for 1..318 */
    {
        static const int xs[] = { -40, -20, 0, 1, 12, 100, 160, 250, 308, 319, 320, 340, 360 };
        const int n = (int)(sizeof xs / sizeof xs[0]);
        int i, j, mode, ui;
        for (i = 0; i < n; i++) for (j = 0; j < n; j++) for (mode = 0; mode < 3; mode++) for (ui = 0; ui < 2; ui++) {
            int top = -1, bottom = -1, lo = xs[i] < xs[j] ? xs[i] : xs[j], hi = xs[i] < xs[j] ? xs[j] : xs[i];
            nd_sci_x(xs[i], 0, xs[j], 240, mode, ui, &top, &bottom);
            if (top < 0 || top > bottom || bottom > 400) { bad++; printf("FAIL grid bounds x %d %d mode %d ui %d got %d %d\n", xs[i], xs[j], mode, ui, top, bottom); }
            if (mode == 0 && top != 40 + ci(lo, 0, 320)) { bad++; printf("FAIL grid native x %d %d got %d\n", xs[i], xs[j], top); }
            if (mode == 1 && ui == 1 && lo >= 1 && lo <= 318 && hi >= 1 && hi <= 318) {
                if (top != 40 + lo || bottom != 40 + hi) { bad++; printf("FAIL grid wide ui x %d %d got %d %d\n", xs[i], xs[j], top, bottom); }
            }
        }
    }
    if (nd_sci_full(0, 0, 320, 240) != 1 || nd_sci_full(12, 20, 308, 220) != 1 || nd_sci_full(0, 0, 319, 239) != 1 || nd_sci_full(12, 0, 308, 220) != 0) {
        bad++; printf("FAIL nd_sci_full\n");
    }
    if (bad) return 1;
    printf("wclip1 ok\n");
    return 0;
}
