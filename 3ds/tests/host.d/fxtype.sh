#!/bin/sh
# FXTYPE: fxtype.py on the fixture 3ds/tests/fxtype/a.c. hA (only a gSPDisplayList use) becomes NdLst and the file gets nd_gtab.h; hB (a local
# reads it) stays. The seeded fault: a copy with USE_RULE = False rewrites hB, and the test must see that. Prints `fxtype test ok` or fails.
T=/tmp/fxtype; PY=${PY:-python3}
rm -rf "$T"; mkdir -p "$T"; tr -d '\r' < 3ds/tests/fxtype/a.c > "$T/a.c"
$PY 3ds/tools/fxtype.py "$T/a.c" > "$T.out" 2>&1 || { echo "fxtype test FAIL: exit"; cat "$T.out"; exit 1; }
tr -d '\r' < "$T.out" | grep -qx "fxtype arrays ok 1 other 1 files 1" || { echo "fxtype test FAIL: count line"; cat "$T.out"; exit 1; }
grep -qx 'NdLst hA\[\] = { D_1, D_2 };' "$T/a.c" || { echo "fxtype test FAIL: hA not NdLst"; exit 1; }
grep -qx 'Gfx\* hB\[\] = { D_3 };' "$T/a.c" || { echo "fxtype test FAIL: hB changed"; exit 1; }
[ "$(sed -n 2p "$T/a.c")" = '#include "nd_gtab.h"' ] || { echo "fxtype test FAIL: no nd_gtab.h include on line 2"; exit 1; }
tr -d '\r' < 3ds/tests/fxtype/a.c > "$T/b.c"
sed 's/^USE_RULE = True/USE_RULE = False/' 3ds/tools/fxtype.py > "$T/seed.py"
$PY "$T/seed.py" "$T/b.c" > /dev/null 2>&1
grep -qx 'NdLst hB\[\] = { D_3 };' "$T/b.c" || { echo "fxtype seeded fault not seen"; exit 1; }
echo "fxtype test ok"
