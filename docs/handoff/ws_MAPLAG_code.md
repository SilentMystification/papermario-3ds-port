# ws_MAPLAG code blocks (exact text, LF line ends). Designer to ask: the oracle.
## Block A: 3ds/src/nd_shape.c
A1: after line 262 (the `}` that ends dlcp_open, before the line `/* 1: o filled from the file, 0: no section ...`) add:
```c
/* MAPLAG (b): the file part of the static list build of a new shape file: the T-point table (seam_get) and the .dlc file (dlcp_open).
 * decode_yay0 (pm_map.c) calls it in the load step on the logic thread (core 1 on the O3DS; inline in the serial mode), after pm_walk_drain:
 * no walk and no queued entry reads these statics then, and the walker's pm_dlc_gen bump of the shape invalidation (pm_tex.c:1252) has run.
 * The next walker entry is queued after these writes (nd_wq.c wq_put barrier). dlc_reset keeps the tables of the current pm_dlc_gen, so the
 * first drawn step reads them and does not load them. Same files, same bytes as the walker build: only the thread and the time change. */
__attribute__((cold, noinline)) void pm_dlc_preload(void) {
    if (!seam_get()) return; /* as dlc_build: no seam table, no .dlc read */
    const unsigned t0 = pm_ticks();
    (void)dlcp_open();
    if (pm_xf.on) pm_xf.dlcp_t += pm_ticks() - t0; /* XFT */
}
```
A2: in dlc_reset, replace lines 648 and 649. Before:
```c
    seam_reset();
    dlcp_close();
```
After:
```c
    if (seam_gen != pm_dlc_gen || !seam_ok) seam_reset(); /* MAPLAG: keep the tables pm_dlc_preload built for this shape (a failed one is built again after the frees above) */
    if (dlcp_gen != pm_dlc_gen || !dlcp_f) dlcp_close();
```
## Block B: 3ds/src/pm_map.c (decode_yay0)
After line 151 (`        { extern unsigned pm_dlc_gen; pm_dlc_gen++; } /* DLC1: the shape file is final now */`) add the line:
```c
        { void pm_walk_drain(const char* why); void pm_dlc_preload(void); pm_walk_drain("dlcpre"); pm_dlc_preload(); } /* MAPLAG (b): the seam table and the .dlc file of the new shape load here (logic thread, core 1); the drain first runs the queued shape invalidation (pm_tex.c:1252 bumps pm_dlc_gen on the walker) */
```
## Block C: 3ds/tests/host.d/dlcpre.sh (new file)
```sh
#!/bin/sh
# MAPLAG (b): decode_yay0 calls pm_dlc_preload after the shape invalidation and the drain; dlc_reset keeps the tables of the current gen.
# Each seeded fault must print its exact line. The tr and awk strip CR (autocrlf checkout).
mkdir -p build3ds/out/dlcpre_seed
chk() {
    awk '{ sub(/\r$/, "") } /pm_tex_invalidate\(dst, dec\);/ { t = NR } /DLC1: the shape file is final now/ { g = NR } /pm_walk_drain\("dlcpre"\); pm_dlc_preload\(\);/ { p = NR } END { if (!t || !g || !p || p < t || p < g) { print "dlcpre FAIL order"; exit 1 } }' "$1" || return 1
    tr -d '\r' < "$2" | grep -qF 'if (seam_gen != pm_dlc_gen || !seam_ok) seam_reset();' || { echo "dlcpre FAIL reset seam"; return 1; }
    tr -d '\r' < "$2" | grep -qF 'if (dlcp_gen != pm_dlc_gen || !dlcp_f) dlcp_close();' || { echo "dlcpre FAIL reset dlcp"; return 1; }
    tr -d '\r' < "$2" | grep -qF 'void pm_dlc_preload(void) {' || { echo "dlcpre FAIL no preload"; return 1; }
    echo "dlcpre ok"
}
seed() { got=$(chk "$1" "$2" || true); if [ "$got" != "$3" ]; then echo "dlcpre seed FAIL: [$3] got [$got]"; exit 1; fi; }
sed '/pm_walk_drain("dlcpre"); pm_dlc_preload();/d' 3ds/src/pm_map.c > build3ds/out/dlcpre_seed/pm_map.c
seed build3ds/out/dlcpre_seed/pm_map.c 3ds/src/nd_shape.c "dlcpre FAIL order"
sed 's/if (seam_gen != pm_dlc_gen || !seam_ok) seam_reset();/seam_reset();/' 3ds/src/nd_shape.c > build3ds/out/dlcpre_seed/nd_shape.c
seed 3ds/src/pm_map.c build3ds/out/dlcpre_seed/nd_shape.c "dlcpre FAIL reset seam"
sed 's/if (dlcp_gen != pm_dlc_gen || !dlcp_f) dlcp_close();/dlcp_close();/' 3ds/src/nd_shape.c > build3ds/out/dlcpre_seed/nd_shape.c
seed 3ds/src/pm_map.c build3ds/out/dlcpre_seed/nd_shape.c "dlcpre FAIL reset dlcp"
chk 3ds/src/pm_map.c 3ds/src/nd_shape.c || exit 1
```
## Block D: 3ds/include/pm_maplag.h (new file)
```c
/* MAPLAG (a) (user 2026-11-16): the fade-in of a map change (src/state_map_transitions.c:190, CHANGE_MAP_FADE_IN) keeps the front overlay at
 * full cover (progress 255) PM_MAPLAG_HOLD more steps. Under full cover the walker builds the new map's model records (pm_xfer.c pm_xfer_skip,
 * 64 a frame) and the hidden models' lists (pm_xfer_hidden, 12 a frame), so that work leaves the visible fade-in frames. pm_map.c arms the hold
 * at each map load and wraps update_enter_map_screen_overlay. Only GAME_MODE_CHANGE_MAP holds; another mode (ENTER_WORLD) clears it. */
#pragma once
#define PM_MAPLAG_HOLD 3
/* One call per update_enter_map_screen_overlay call. hold: the steps left. chg: the game mode is GAME_MODE_CHANGE_MAP. full: the progress
 * was 255 at the call. Returns 1 when the caller must set the progress back to 255 and return 0 (not done). */
static inline int pm_maplag_keep(int* hold, int chg, int full) {
    if (!chg) { *hold = 0; return 0; }
    if (!full || *hold <= 0) return 0;
    (*hold)--;
    return 1;
}
```
## Block E: 3ds/src/pm_map.c
E1: after line 10 (`#include "nd_yay0.h"`) add the line `#include "pm_maplag.h"`.
E2: after the line `char pm_shape_name[24] = "none"; /* SEAMFIX: ...` add the line:
`int pm_maplag_hold; /* MAPLAG (a): fade-in steps left at full cover, armed by each map load (pm_maplag.h) */`
E3: in __wrap_load_map_by_IDs, after the line `    __real_load_map_by_IDs(areaID, mapID, loadType);` add the line:
`    pm_maplag_hold = PM_MAPLAG_HOLD; /* MAPLAG (a): the fade-in of a map change keeps full cover PM_MAPLAG_HOLD more steps */`
E4: after the `}` that ends __wrap_load_battle_section add:
```c
/* MAPLAG (a): the real call sets the overlay of this step from the progress, then lowers it; a held step puts the progress back to 255. */
extern s16 __real_update_enter_map_screen_overlay(s16* progress);
s16 __wrap_update_enter_map_screen_overlay(s16* progress) {
    const int full = *progress >= 255;
    const s16 r = __real_update_enter_map_screen_overlay(progress);
    if (pm_maplag_keep(&pm_maplag_hold, get_game_mode() == GAME_MODE_CHANGE_MAP, full)) { *progress = 255; return 0; }
    return r;
}
```
## Block F: 3ds/tools/pm_maplag_test.c (new file)
```c
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
```
## Block G: 3ds/CMakeLists.txt line 411 and 3ds/tests/host.d/maplag.sh (new file)
G1, line 411. Before: `    -Wl,--wrap=set_game_mode -Wl,--wrap=load_map_by_IDs -Wl,--wrap=load_battle_section -Wl,--wrap=dma_copy -Wl,--wrap=load_asset_by_name`
After: `    -Wl,--wrap=set_game_mode -Wl,--wrap=load_map_by_IDs -Wl,--wrap=load_battle_section -Wl,--wrap=dma_copy -Wl,--wrap=load_asset_by_name -Wl,--wrap=update_enter_map_screen_overlay`
G2: 3ds/tests/host.d/maplag.sh:
```sh
#!/bin/sh
# MAPLAG (a): pm_maplag.h holds the map change fade-in at full cover PM_MAPLAG_HOLD more steps. Each seeded fault must print its exact line.
mkdir -p build3ds/out/maplag_seed
seed() {
    sed "$1" 3ds/include/pm_maplag.h > build3ds/out/maplag_seed/pm_maplag.h
    gcc -m32 -Wall -Wextra -I build3ds/out/maplag_seed -o build3ds/out/maplag_seed/t 3ds/tools/pm_maplag_test.c || exit 1
    got=$(build3ds/out/maplag_seed/t || true)
    if [ "$got" != "$2" ]; then echo "maplag seed FAIL: [$1] got [$got]"; exit 1; fi
}
seed 's/#define PM_MAPLAG_HOLD 3/#define PM_MAPLAG_HOLD 0/' 'maplag FAIL range 0'
seed 's/if (!chg) { \*hold = 0; return 0; }/if (!chg) return 0;/' 'maplag FAIL enter world clear'
seed 's/(\*hold)--;//' 'maplag FAIL covered 64'
tr -d '\r' < 3ds/src/pm_map.c | grep -qF 'pm_maplag_hold = PM_MAPLAG_HOLD;' || { echo "maplag FAIL: load_map_by_IDs does not arm the hold"; exit 1; }
tr -d '\r' < 3ds/src/pm_map.c | grep -qF 's16 __wrap_update_enter_map_screen_overlay(s16* progress) {' || { echo "maplag FAIL: no wrap"; exit 1; }
tr -d '\r' < 3ds/CMakeLists.txt | grep -qF -- '-Wl,--wrap=update_enter_map_screen_overlay' || { echo "maplag FAIL: no link wrap"; exit 1; }
gcc -m32 -Wall -Wextra -I 3ds/include -o build3ds/out/maplag_t 3ds/tools/pm_maplag_test.c && build3ds/out/maplag_t || exit 1
```
