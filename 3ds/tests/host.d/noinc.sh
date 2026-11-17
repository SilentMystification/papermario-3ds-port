#!/bin/sh
# NOINC: listcen.py does not count an asset that no src, include or gfxin twin file names. Fixture: one named and one unnamed asset list.
# The seed (a copy with NOINC_RULE = False) must count the unnamed asset too.
PY=${PY:-python3}
T=${TMPDIR:-/tmp}/noinc.$$
rm -rf "$T"; mkdir -p "$T/3ds/tools" "$T/src" "$T/assets/us/fx"
cp 3ds/tools/listcen.py "$T/3ds/tools/listcen.py"
printf '#include "fx/live.gfx.inc.c"\n' > "$T/src/a.c"
printf 'Gfx D_live[] = {\n    gsSPEndDisplayList(),\n};\n' > "$T/assets/us/fx/live.gfx.inc.c"
printf 'Gfx D_unnamed[] = {\n    gsSPEndDisplayList(),\n};\n' > "$T/assets/us/fx/unnamed.gfx.inc.c"
got=$($PY "$T/3ds/tools/listcen.py" | grep -E '^listcen total data(_arrays)? ' | tr '\n' ' ')
sed -i 's/^NOINC_RULE = True/NOINC_RULE = False/' "$T/3ds/tools/listcen.py"
seed=$($PY "$T/3ds/tools/listcen.py" | grep -E '^listcen total data(_arrays)? ' | tr '\n' ' ')
rm -rf "$T"
if [ "$got" != "listcen total data 1 listcen total data_arrays 1 " ]; then echo "noinc FAIL rule: [$got]"; exit 1; fi
if [ "$seed" != "listcen total data 2 listcen total data_arrays 2 " ]; then echo "noinc FAIL seed not seen: [$seed]"; exit 1; fi
echo "noinc ok"
