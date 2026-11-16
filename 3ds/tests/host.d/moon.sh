#!/bin/sh
# MOON6: host test of pm_moon.h (gate, flat-ground jump arc, timeInAir cap, stick speed). Each seeded fault must print its exact line.
mkdir -p /tmp/moon_seed
seed() {
    sed "$1" 3ds/src/pm_moon.h > /tmp/moon_seed/pm_moon.h
    gcc -m32 -Wall -I /tmp/moon_seed -o /tmp/moon_seed/t 3ds/tools/pm_moon_test.c || exit 1
    got=$(/tmp/moon_seed/t)
    if [ "$got" != "$2" ]; then echo "moon seed FAIL: [$1] got [$got]"; exit 1; fi
}
seed 's/ || peakJumpTime > 0//' 'moon FAIL arc boots 1 first 0 land 15'
seed 's/timeInAir >= PM_MOON_WIN_END/1/' 'moon FAIL arc boots 1 first -1 land 0'
seed 's/? PM_MOON_AIR_TIME : timeInAir/? timeInAir : timeInAir/' 'moon FAIL cap'
seed 's/if (mag > PM_MOON_STICK_MAX) mag = PM_MOON_STICK_MAX; //' 'moon FAIL speed 2'
gcc -m32 -Wall -I 3ds/src -o /tmp/moon_t 3ds/tools/pm_moon_test.c || exit 1
/tmp/moon_t || exit 1
