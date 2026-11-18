#!/bin/sh
# NREC1A: nd_msnap_in (3ds/src/nd_msnap_in.c) on a fixed NdDs (3ds/tools/nd_msnap_test.c): the take, the diff of a snap with itself and one with two fields. Prints `nrec1a cases 3 bad <b>`.
# Seeded fault: --fault changes case 2 to expect one difference, bad must be above 0 first.
F="-m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
gcc $F -o /tmp/nrec1a 3ds/tools/nd_msnap_test.c 3ds/src/nd_msnap_in.c 3ds/src/nd_snap_in.c || { echo "nrec1a FAIL build"; exit 1; }
if /tmp/nrec1a --fault > /tmp/nrec1a_fault.txt; then echo "nrec1a FAIL: the seeded fault passed"; exit 1; fi
grep -q "bad [1-9]" /tmp/nrec1a_fault.txt || { echo "nrec1a FAIL: fault run gave no bad count"; exit 1; }
echo "nrec1a seed ok"
/tmp/nrec1a || exit 1
