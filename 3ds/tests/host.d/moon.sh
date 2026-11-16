#!/bin/sh
# MOON7: host test of pm_moon.h (pm_moon_late table, jump model: taps, spin after a hold, hold lift, steer gate, re-entry, stick speed).
# Each seeded fault must print its exact line. SSE math: the cheat-off and cheat-on runs compare floats bit for bit.
mkdir -p /tmp/moon_seed
CF="-m32 -msse2 -mfpmath=sse -Wall"
seed() {
    sed "$1" 3ds/src/pm_moon.h > /tmp/moon_seed/pm_moon.h
    gcc $CF -I /tmp/moon_seed -o /tmp/moon_seed/t 3ds/tools/pm_moon_test.c || exit 1
    got=$(/tmp/moon_seed/t)
    if [ "$got" != "$2" ]; then echo "moon seed FAIL: [$1] got [$got]"; exit 1; fi
}
seed 's/|| timeInAir >= PM_MOON_WIN_END/|| 1/' 'moon FAIL case 0'
seed 's/>= PM_MOON_FLOOR) return r;/>= 1e9f) return r;/' 'moon FAIL tap 1'
seed 's/if (m->up) {/if (0) {/' 'moon FAIL spin on'
seed 's/\*steer = \*timeInAir >= PM_MOON_WIN_END;/*steer = 1;/' 'moon FAIL steer step 5'
seed 's/if (mag > PM_MOON_STICK_MAX) mag = PM_MOON_STICK_MAX; //' 'moon FAIL speed 2'
gcc $CF -I 3ds/src -o /tmp/moon_t 3ds/tools/pm_moon_test.c || exit 1
/tmp/moon_t || exit 1
