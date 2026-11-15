#!/bin/sh
# MOON4: host test of pm_moon_allowed. A seeded fault (the window check removed) must fail at case 0.
mkdir -p /tmp/moon_seed
sed 's/timeInAir >= PM_MOON_WIN_END/1/' 3ds/src/pm_moon.h > /tmp/moon_seed/pm_moon.h
gcc -m32 -Wall -I /tmp/moon_seed -o /tmp/moon_seed/t 3ds/tools/pm_moon_test.c || exit 1
got=$(/tmp/moon_seed/t)
if [ "$got" != "moon FAIL case 0" ]; then echo "moon seed FAIL: got [$got]"; exit 1; fi
sed 's/if (mag > PM_MOON_STICK_MAX) mag = PM_MOON_STICK_MAX; //' 3ds/src/pm_moon.h > /tmp/moon_seed/pm_moon.h
gcc -m32 -Wall -I /tmp/moon_seed -o /tmp/moon_seed/t 3ds/tools/pm_moon_test.c || exit 1
got=$(/tmp/moon_seed/t)
if [ "$got" != "moon FAIL speed 2" ]; then echo "moon seed2 FAIL: got [$got]"; exit 1; fi
gcc -m32 -Wall -I 3ds/src -o /tmp/moon_t 3ds/tools/pm_moon_test.c || exit 1
/tmp/moon_t || exit 1
