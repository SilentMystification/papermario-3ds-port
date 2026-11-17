#!/bin/sh
# FXLOC: fxtype.py --local on the fixture 3ds/tests/fxloc/a.c. hA (read by the handle local dl, which only nd_draw_list uses) and dl become
# NdLst; hB stays (its local `other` has another use). The seeded fault: a copy with LOCAL_RULE = False rewrites hB, and the test must see
# that. Prints `fxloc test ok` or fails.
T=/tmp/fxloc; PY=${PY:-python3}; $PY -c 1 >/dev/null 2>&1 || PY=python
rm -rf "$T"; mkdir -p "$T"; tr -d '\r' < 3ds/tests/fxloc/a.c > "$T/a.c"
$PY 3ds/tools/fxtype.py --local "$T/a.c" > "$T.out" 2>&1 || { echo "fxloc test FAIL: exit"; cat "$T.out"; exit 1; }
tr -d '\r' < "$T.out" | grep -qx "fxtype arrays ok 1 other 1 files 1 locals 1" || { echo "fxloc test FAIL: count line"; cat "$T.out"; exit 1; }
grep -qx 'NdLst hA\[\] = { D_1, D_2 };' "$T/a.c" || { echo "fxloc test FAIL: hA not NdLst"; exit 1; }
grep -qx '    NdLst dl = hA\[i\];' "$T/a.c" || { echo "fxloc test FAIL: dl not NdLst"; exit 1; }
grep -qx 'Gfx\* hB\[\] = { D_3 };' "$T/a.c" || { echo "fxloc test FAIL: hB changed"; exit 1; }
grep -qx '    Gfx\* other = hB\[i\];' "$T/a.c" || { echo "fxloc test FAIL: other changed"; exit 1; }
[ "$(sed -n 2p "$T/a.c")" = '#include "nd_gtab.h"' ] || { echo "fxloc test FAIL: no nd_gtab.h include on line 2"; exit 1; }
tr -d '\r' < 3ds/tests/fxloc/a.c > "$T/b.c"
sed 's/^LOCAL_RULE = True/LOCAL_RULE = False/' 3ds/tools/fxtype.py > "$T/seed.py"
$PY "$T/seed.py" --local "$T/b.c" > /dev/null 2>&1
grep -qx 'NdLst hB\[\] = { D_3 };' "$T/b.c" || { echo "fxloc seeded fault not seen"; exit 1; }
echo "fxloc test ok"
