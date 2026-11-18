#!/bin/sh
# TATTLEAR: the tattle camera view in wide. set_cam_viewport and pm_wide_x3d (src/cam_main.c) are extracted into $N/ext.c and run
# by 3ds/tests/tattlear/tattlear_test.c. Prints `tattlear ok 7`. Seeded fault: the `* 8 / 5` of the tattle view changed to `* 2`
# must print `tattlear FAIL` first.
F="-m32 -w -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I src -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -msse2 -mfpmath=sse -ffp-contract=off"
N=/tmp/tattlear; rm -rf $N $N/seed; mkdir -p $N/seed
printf '#define NAMESPACE t
#include "common.h"
extern int pm_wide;
' > $N/ext.c
tr -d '\r' < src/cam_main.c | awk '/^s32 pm_wide_x3d\(/ {on=1} on {print} on && /^}/ {exit}' >> $N/ext.c
tr -d '\r' < src/cam_main.c | awk '/^void set_cam_viewport\(/ {on=1} on {print} on && /^}/ {exit}' >> $N/ext.c
grep -q "^s32 pm_wide_x3d(" $N/ext.c || { echo "tattlear FAIL extract x3d"; exit 1; }
grep -q "^void set_cam_viewport(" $N/ext.c || { echo "tattlear FAIL extract viewport"; exit 1; }
sed 's/\* 8 \/ 5/* 2/g' $N/ext.c > $N/seed/ext.c
cmp -s $N/seed/ext.c $N/ext.c && { echo "tattlear seeded fault not made"; exit 1; }
gcc $F -o $N/seed_t 3ds/tests/tattlear/tattlear_test.c $N/seed/ext.c -lm || { echo "tattlear FAIL seed build"; exit 1; }
got=$($N/seed_t | head -1)
case "$got" in "tattlear FAIL"*) ;; *) echo "tattlear seeded fault not seen: [$got]"; exit 1;; esac
gcc $F -o $N/t 3ds/tests/tattlear/tattlear_test.c $N/ext.c -lm || { echo "tattlear FAIL build"; exit 1; }
$N/t || exit 1
