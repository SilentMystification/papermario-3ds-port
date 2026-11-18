#!/bin/sh
# SPEEDX2: pm_speedx2 (3ds/src/pm_speedx2.h) doubles the base once, no compound. The seed (every call doubles the current speed) must fail "speedx2 FAIL on walk got 4096.000000".
d=build3ds/out/speedx2; mkdir -p $d
sed 's/\*walk = s->walk \* 2.0f/*walk = *walk * 2.0f/' 3ds/src/pm_speedx2.h > $d/pm_speedx2.h
gcc -m32 -w -I $d -o $d/t 3ds/tools/pm_speedx2_test.c || exit 1
got=$($d/t); if [ "$got" != "speedx2 FAIL on walk got 4096.000000" ]; then echo "speedx2 seed FAIL: got [$got]"; exit 1; fi
grep -q "pm_speedx2(&spd," 3ds/src/pm_dbg_game.c || { echo "speedx2 FAIL: pm_dbg_game.c does not call pm_speedx2"; exit 1; }
gcc -m32 -Wall -Wextra -I 3ds/src -o $d/t2 3ds/tools/pm_speedx2_test.c && $d/t2 || exit 1
