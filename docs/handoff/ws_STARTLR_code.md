# ws_STARTLR code blocks (copy exactly)

## Block A (3ds/include/pm_shoulder.h)
```c
#ifndef PM_SHOULDER_H
#define PM_SHOULDER_H
/* STARTLR: the N64 bits of the 3DS shoulder keys. l, r, zl, zr: the key is held (0 or 1). sh4: pm_map from PM_IN_L (L, R, ZL, ZR,
 * pm_port.h:334). zcombo: the O3DS Z key is held (X, or Y with o3dsz=1). start_menu: the start (pause) menu (GAME_MODE_PAUSE).
 * Start menu (user, 2026-11-15): L gives N64 L (0x0020), R gives N64 Z (0x2000), ZL and ZR keep their map. The O3DS combo comes first. */
static inline unsigned short pm_shoulder_bits(int l, int r, int zl, int zr, const unsigned short* sh4, int zcombo, int start_menu) {
    if (zcombo) return (unsigned short)((l ? 0x0020 : 0) | (r ? 0x0010 : 0));
    if (start_menu) return (unsigned short)((l ? 0x0020 : 0) | (r ? 0x2000 : 0) | (zl ? sh4[2] : 0) | (zr ? sh4[3] : 0));
    return (unsigned short)((l ? sh4[0] : 0) | (r ? sh4[1] : 0) | (zl ? sh4[2] : 0) | (zr ? sh4[3] : 0));
}
#endif
```

## Block B (3ds/tools/pm_shoulder_test.c)
```c
/* STARTLR host test: pm_shoulder_bits (pm_shoulder.h) with the default map (pm_options.c:53: L Z, R Z, ZL N64 L, ZR N64 R). */
#include <stdio.h>
#include "pm_shoulder.h"

static const unsigned short m[4] = { 0x2000, 0x2000, 0x0020, 0x0010 };
static const struct { const char* n; int l, r, zl, zr, zc, sm; unsigned short want; } c[] = {
    { "start L", 1, 0, 0, 0, 0, 1, 0x0020 },
    { "start R", 0, 1, 0, 0, 0, 1, 0x2000 },
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
```

## Block C (3ds/tests/host.d/startlr.sh)
```sh
#!/bin/sh
# STARTLR: pm_shoulder_bits (pm_shoulder.h): in the start menu L gives N64 L, R gives N64 Z. The seed (no start menu branch) must fail "start L".
mkdir -p /tmp/startlr_seed
sed 's/if (start_menu) return/if (0) return/' 3ds/include/pm_shoulder.h > /tmp/startlr_seed/pm_shoulder.h
gcc -m32 -w -I /tmp/startlr_seed -o /tmp/startlr_seed/t 3ds/tools/pm_shoulder_test.c || exit 1
got=$(/tmp/startlr_seed/t); if [ "$got" != "startlr FAIL start L got 2000" ]; then echo "startlr seed FAIL: got [$got]"; exit 1; fi
grep -q "    return pm_shoulder_bits((k & KEY_L) != 0" 3ds/src/pm_input.c && grep -q "pm_start_menu()); /\* STARTLR \*/" 3ds/src/pm_input.c || { echo "startlr FAIL: shoulder_map does not call pm_shoulder_bits"; exit 1; }
gcc -m32 -Wall -Wextra -I 3ds/include -o /tmp/startlr_t 3ds/tools/pm_shoulder_test.c && /tmp/startlr_t || exit 1
```
