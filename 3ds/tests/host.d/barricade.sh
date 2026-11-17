#!/bin/sh
# BARRICADE: the map shape list walkers (barricade.c DetermineSphericalSize, GetFirstTriangleNormal.inc.c) read u32 unit pairs.
# The real functions are extracted into $N/ext.c and run on hand-built lists (3ds/tests/maplist/maplist_test.c).
# Prints `barricade ok 2`. Seeded fault: the unit step of both walkers changed to 4 words must print `barricade FAIL` first.
F="-m32 -w -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I src -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -msse2 -mfpmath=sse -ffp-contract=off"
N=/tmp/barricade; rm -rf $N $N/seed; mkdir -p $N/seed
printf '#define NAMESPACE t
#include "common.h"
' > $N/ext.c
tr -d '\r' < src/world/area_omo/omo_02/barricade.c | awk '/^#define BAR_/ {print} /^void N\(DetermineSphericalSize\)/ {on=1} on {print} on && /^}/ {exit}' >> $N/ext.c
grep -q "^void N(DetermineSphericalSize)" $N/ext.c || { echo "barricade FAIL extract"; exit 1; }
tr -d '\r' < src/world/common/util/GetFirstTriangleNormal.inc.c >> $N/ext.c
grep -q "gfx += 2;" $N/ext.c || { echo "barricade FAIL extract gftn"; exit 1; }
sed 's/+= 2;/+= 4;/' $N/ext.c > $N/seed/ext.c
cmp -s $N/seed/ext.c $N/ext.c && { echo "barricade seeded fault not made"; exit 1; }
gcc $F -o $N/seed_t 3ds/tests/maplist/maplist_test.c $N/seed/ext.c -lm || { echo "barricade FAIL seed build"; exit 1; }
got=$($N/seed_t | head -1)
case "$got" in "barricade FAIL"*) ;; *) echo "barricade seeded fault not seen: [$got]"; exit 1;; esac
gcc $F -o $N/t 3ds/tests/maplist/maplist_test.c $N/ext.c -lm || exit 1
$N/t || exit 1
