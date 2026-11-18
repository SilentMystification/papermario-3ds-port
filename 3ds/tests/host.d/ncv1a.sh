#!/bin/sh
# NCV1A: the field-set ids (3ds/include/nd_fs.h) equal the unit ids (3ds/include/nd_op.h), tested by 3ds/tools/nd_fs_test.c alone.
# Prints `ncv1a ids <n> bad <b>`. Seeded fault: --fault compares ND_FS_TILE with NDO_TILESIZE, bad must be above 0 first.
F="-m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
gcc $F -o /tmp/ncv1a 3ds/tools/nd_fs_test.c || { echo "ncv1a FAIL build"; exit 1; }
if /tmp/ncv1a --fault > /tmp/ncv1a_fault.txt; then echo "ncv1a FAIL: the seeded fault passed"; exit 1; fi
grep -q "bad [1-9]" /tmp/ncv1a_fault.txt || { echo "ncv1a FAIL: fault run gave no bad count"; exit 1; }
echo "ncv1a seed ok"
/tmp/ncv1a || exit 1
