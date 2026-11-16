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
