#!/bin/sh
# GBIHDR: gbiprune.py on the fixture 3ds/tests/gbiprune/case. Real: only gDPC is dead (gsDPB lives through the body of gDPA).
# Seeded fault: without src/user.c, gDPA and gsDPB die too (dead 3 lines 4). Prints `gbiprune ok seed ok` or fails.
T=${GBIPRUNE_T:-/tmp/gbiprunecase}
PY=${PY:-python3}
rm -rf "$T"; mkdir -p "$T"; cp -r 3ds/tests/gbiprune/case/. "$T/"
$PY 3ds/tools/gbiprune.py "$T" > "$T.out" 2>&1 || { echo "gbiprune FAIL: exit"; cat "$T.out"; exit 1; }
grep -qx "gbiprune dead 1 lines 1" "$T.out" || { echo "gbiprune FAIL: real case"; cat "$T.out"; exit 1; }
if grep -q "gDPC" "$T/include/PR/gbi.h"; then echo "gbiprune FAIL: gDPC stays"; exit 1; fi
grep -q "gsDPB" "$T/include/PR/gbi.h" || { echo "gbiprune FAIL: gsDPB removed"; exit 1; }
rm -rf "$T"; mkdir -p "$T"; cp -r 3ds/tests/gbiprune/case/. "$T/"; rm "$T/src/user.c"
$PY 3ds/tools/gbiprune.py --dry "$T" > "$T.out" 2>&1
grep -qx "gbiprune dead 3 lines 4" "$T.out" || { echo "gbiprune FAIL: the seed is not seen"; cat "$T.out"; exit 1; }
echo "gbiprune ok seed ok"
