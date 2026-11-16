#!/bin/sh
# GDEFREN: gen_gfx_syms.py gives an include the `#define A B` renames before it (src/effects/gfx/fire_breath.c:10-12, peach_star_beam.c:58-60).
# Fixture: A is renamed A_body in its include (the wrapper keeps A). M is renamed and undefined before its include, so it keeps M.
# The seeded fault (a copy with REN_RULE = False) must lose A_body.
W=/tmp/gdefren
rm -rf $W && mkdir -p $W/o $W/f
printf '#define A A_body\n#include "a.gfx.inc.c"\n#undef A\nGfx A[] = {\n    gsSPEndDisplayList(),\n};\n#define M M_x\n#undef M\n#include "m.gfx.inc.c"\n' > $W/x.c
printf 'Gfx A[] = {\n    gsSPEndDisplayList(),\n};\n' > $W/a.gfx.inc.c
printf 'Gfx M[] = {\n    gsSPEndDisplayList(),\n};\n' > $W/m.gfx.inc.c
python3 3ds/tools/gen_gfx_syms.py $W/o/gconv_in.c $W/x.c > /dev/null || { echo "gdefren FAIL gen"; exit 1; }
for n in A_body A M; do grep -q "\"$n\", GS_GFX" $W/o/gconv_tu0.c || { echo "gdefren FAIL no $n"; exit 1; }; done
grep -q '"M_x"' $W/o/gconv_tu0.c && { echo "gdefren FAIL undef not seen"; exit 1; }
sed 's/^REN_RULE = True/REN_RULE = False/' 3ds/tools/gen_gfx_syms.py > $W/gen_fault.py
cmp -s $W/gen_fault.py 3ds/tools/gen_gfx_syms.py && { echo "gdefren FAIL seeded fault not applied"; exit 1; }
python3 $W/gen_fault.py $W/f/gconv_in.c $W/x.c > /dev/null 2>&1
grep -q '"A_body", GS_GFX' $W/f/gconv_tu0.c 2>/dev/null && { echo "gdefren FAIL seeded fault not seen"; exit 1; }
echo "gdefren ok"
