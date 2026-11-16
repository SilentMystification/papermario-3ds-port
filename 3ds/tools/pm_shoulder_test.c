/* STARTLR host test: pm_shoulder_bits (pm_shoulder.h) with the default map (pm_options.c:53: L Z, R Z, ZL N64 L, ZR N64 R). */
#include <stdio.h>
#include "pm_shoulder.h"

static const unsigned short m[4] = { 0x2000, 0x2000, 0x0020, 0x0010 };
static const struct { const char* n; int l, r, zl, zr, zc, sm; unsigned short want; } c[] = {
    { "start L", 1, 0, 0, 0, 0, 1, 0x2000 },
    { "start R", 0, 1, 0, 0, 0, 1, 0x0010 },
    { "start ZL ZR", 0, 0, 1, 1, 0, 1, 0x0030 },
    { "start combo R", 0, 1, 0, 0, 1, 1, 0x0010 },
    { "world L", 1, 0, 0, 0, 0, 0, 0x2000 },
    { "world R", 0, 1, 0, 0, 0, 0, 0x2000 },
    { "world combo L", 1, 0, 0, 0, 1, 0, 0x0020 },
};

int main(void) {
    unsigned i;
    for (i = 0; i < sizeof c / sizeof c[0]; i++) {
        unsigned short got = pm_shoulder_bits(c[i].l, c[i].r, c[i].zl, c[i].zr, m, c[i].zc, c[i].sm);
        if (got != c[i].want) { printf("startlr FAIL %s got %04x\n", c[i].n, got); return 1; }
    }
    printf("startlr ok\n");
    return 0;
}
