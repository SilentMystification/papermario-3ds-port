#!/bin/sh
# NDR0A: nd_snap_in (3ds/src/nd_snap_in.c) on a fixed NdDs against the expected fields (3ds/tools/nd_snap_test.c). Prints `ndr0a in bad <b>`.
# Seeded fault: --fault changes one expected value, bad must be above 0 first.
F="-m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
gcc $F -o /tmp/ndr0a 3ds/tools/nd_snap_test.c 3ds/src/nd_snap_in.c || { echo "ndr0a FAIL build"; exit 1; }
if /tmp/ndr0a --fault > /tmp/ndr0a_fault.txt; then echo "ndr0a FAIL: the seeded fault passed"; exit 1; fi
grep -q "bad [1-9]" /tmp/ndr0a_fault.txt || { echo "ndr0a FAIL: fault run gave no bad count"; exit 1; }
echo "ndr0a seed ok"
/tmp/ndr0a || exit 1
