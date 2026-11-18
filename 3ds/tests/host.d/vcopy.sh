#!/bin/sh
# VCOPY: the unit copy of mdl_local_gfx_update_vtx_pointers (extracted from src/model.c) equals the words of the old path, and nd_vcopy_dev (nd_vcopy.h)
# counts the units and triangles of the copy. Prints `vcopy ok <n>`. Seeded fault: the triangle count of NDO_TRI2 changed to 1 must print FAIL.
F="-m32 -w -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I src -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
N=/tmp/vcopy; rm -rf $N $N/seed; mkdir -p $N/seed
tr -d '\r' < src/model.c | awk '/^void mdl_local_gfx_update_vtx_pointers/ {on=1} on {print} on && /^}/ {exit}' > $N/ext_body.c
printf '#include "common.h"
#include "nd_op.h"
' > $N/ext.c; cat $N/ext_body.c >> $N/ext.c
grep -q "NDO_VTX" $N/ext.c || { echo "vcopy FAIL extract"; exit 1; }
sed 's/ntri += 2;/ntri += 1;/' 3ds/include/nd_vcopy.h > $N/seed/nd_vcopy.h
cmp -s $N/seed/nd_vcopy.h 3ds/include/nd_vcopy.h && { echo "vcopy seeded fault not made"; exit 1; }
gcc -I $N/seed $F -o $N/seed_t 3ds/tests/vcopy/vcopy_test.c $N/ext.c || { echo "vcopy FAIL seed build"; exit 1; }
got=$($N/seed_t | head -1)
case "$got" in "vcopy FAIL dev"*) ;; *) echo "vcopy seeded fault not seen: [$got]"; exit 1;; esac
gcc $F -o $N/t 3ds/tests/vcopy/vcopy_test.c $N/ext.c || exit 1
tr -d '' < src/model.c | awk '/^void mdl_make_local_vertex_copy/ {on=1} on {print} on && /^}/ {exit}' > $N/mk.c
grep -q 'nd_vcopy_dev(&copy->dev\[i\], copy->gfxCopy\[i\]);' $N/mk.c || { echo "vcopy FAIL dev not filled"; exit 1; }
sed '/nd_vcopy_dev(&copy->dev/d' $N/mk.c > $N/seed/mk.c
grep -q 'nd_vcopy_dev(&copy->dev\[i\], copy->gfxCopy\[i\]);' $N/seed/mk.c && { echo "vcopy seeded fault not seen (dev)"; exit 1; }
grep -q 'heap_malloc(gfxCount \* 2 \* sizeof(uint32_t))' $N/mk.c || { echo "vcopy FAIL copy size"; exit 1; }
sed 's/gfxCount \* 2 \*/gfxCount */' $N/mk.c > $N/seed/mk2.c; grep -q 'gfxCount \* 2 \* sizeof' $N/seed/mk2.c && { echo "vcopy seeded fault not seen (size)"; exit 1; }
$N/t || exit 1
