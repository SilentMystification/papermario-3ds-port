#!/bin/sh
# STARTLR2: pm_shoulder_bits (pm_shoulder.h): in the start menu L gives N64 Z, R gives N64 R. The seed (no start menu branch) must fail "start R".
mkdir -p /tmp/startlr_seed
sed 's/if (start_menu) return/if (0) return/' 3ds/include/pm_shoulder.h > /tmp/startlr_seed/pm_shoulder.h
gcc -m32 -w -I /tmp/startlr_seed -o /tmp/startlr_seed/t 3ds/tools/pm_shoulder_test.c || exit 1
got=$(/tmp/startlr_seed/t); if [ "$got" != "startlr FAIL start R got 2000" ]; then echo "startlr seed FAIL: got [$got]"; exit 1; fi
grep -q "    return pm_shoulder_bits((k & KEY_L) != 0" 3ds/src/pm_input.c && grep -q "pm_start_menu()); /\* STARTLR \*/" 3ds/src/pm_input.c || { echo "startlr FAIL: shoulder_map does not call pm_shoulder_bits"; exit 1; }
gcc -m32 -Wall -Wextra -I 3ds/include -o /tmp/startlr_t 3ds/tools/pm_shoulder_test.c && /tmp/startlr_t || exit 1
