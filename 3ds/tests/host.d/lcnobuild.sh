#!/bin/sh
# LCNOBUILD: listcen.py does not count an asset whose only include is commented out. Fixture: one dead and one live asset list.
# The seed (a copy with NOBUILD_RULE = False) must count the dead asset too.
PY=${PY:-python3}
T=${TMPDIR:-/tmp}/lcnobuild.$$
rm -rf "$T"; mkdir -p "$T/3ds/tools" "$T/src" "$T/assets/us/fx"
cp 3ds/tools/listcen.py "$T/3ds/tools/listcen.py"
printf '//#include "fx/dead.gfx.inc.c"\n#include "fx/live.gfx.inc.c"\n' > "$T/src/a.c"
printf 'Gfx D_dead[] = {\n    gsSPEndDisplayList(),\n};\n' > "$T/assets/us/fx/dead.gfx.inc.c"
printf 'Gfx D_live[] = {\n    gsSPEndDisplayList(),\n};\n' > "$T/assets/us/fx/live.gfx.inc.c"
got=$($PY "$T/3ds/tools/listcen.py" | grep -E '^listcen total data(_arrays)? ' | tr '\n' ' ')
sed -i 's/^NOBUILD_RULE = True/NOBUILD_RULE = False/' "$T/3ds/tools/listcen.py"
seed=$($PY "$T/3ds/tools/listcen.py" | grep -E '^listcen total data(_arrays)? ' | tr '\n' ' ')
rm -rf "$T"
if [ "$got" != "listcen total data 1 listcen total data_arrays 1 " ]; then echo "lcnobuild FAIL rule: [$got]"; exit 1; fi
if [ "$seed" != "listcen total data 2 listcen total data_arrays 2 " ]; then echo "lcnobuild FAIL seed not seen: [$seed]"; exit 1; fi
echo "lcnobuild ok"
