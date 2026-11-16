#include <stdio.h>
#include "pm_maplag.h"
/* MAPLAG host test: the fade-in of update_enter_map_screen_overlay (src/screen_overlays.c:699, STANDARD_FADE_RATE 20) under the pm_map.c wrap.
 * Model of the real call: the overlay of the step is the progress; it returns 1 when the progress is 0; then the progress drops 20 (floor 0). */
static int run(int* hold, int chg, int* cov) {
    int p = 255, n = 0;
    *cov = 0;
    while (n < 64) {
        const int full = p >= 255, r = p == 0;
        if (full) (*cov)++;
        p = p < 20 ? 0 : p - 20;
        n++;
        if (pm_maplag_keep(hold, chg, full)) p = 255;
        else if (r) return n;
    }
    return -1;
}
int main(void) {
    int hold = PM_MAPLAG_HOLD, cov = 0, n;
    n = run(&hold, 1, &cov); /* a map change after its load */
    if (cov != 1 + PM_MAPLAG_HOLD) { printf("maplag FAIL covered %d\n", cov); return 1; }
    if (n != 14 + PM_MAPLAG_HOLD || hold != 0) { printf("maplag FAIL steps %d hold %d\n", n, hold); return 1; }
    hold = PM_MAPLAG_HOLD;
    n = run(&hold, 0, &cov); /* ENTER_WORLD: no hold */
    if (cov != 1 || n != 14) { printf("maplag FAIL enter world %d %d\n", cov, n); return 1; }
    if (hold != 0) { printf("maplag FAIL enter world clear\n"); return 1; }
    n = run(&hold, 1, &cov); /* a fade-in with no load before it: no hold */
    if (cov != 1 || n != 14) { printf("maplag FAIL no load %d %d\n", cov, n); return 1; }
    if (PM_MAPLAG_HOLD < 2 || PM_MAPLAG_HOLD > 4) { printf("maplag FAIL range %d\n", PM_MAPLAG_HOLD); return 1; }
    printf("maplag ok 5/5\n");
    return 0;
}
