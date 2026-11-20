#!/bin/sh
# NST1: nd_ds_apply (3ds/src/nd_ds.c) on 12 fixed units against a table (3ds/tools/nd_ds_test.c). Prints `nst1 fields <n> bad <b>`.
# Seeded fault: --fault changes one expected value, bad must be above 0 first.
F="-m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
gcc $F -o /tmp/nst1 3ds/tools/nd_ds_test.c 3ds/src/nd_ds.c 3ds/src/nd_texwin.c || { echo "nst1 FAIL build"; exit 1; }
if /tmp/nst1 --fault > /tmp/nst1_fault.txt; then echo "nst1 FAIL: the seeded fault passed"; exit 1; fi
grep -q "bad [1-9]" /tmp/nst1_fault.txt || { echo "nst1 FAIL: fault run gave no bad count"; exit 1; }
grep -q "BAD nat5b" /tmp/nst1_fault.txt || { echo "nst1 FAIL: nat5b fault not seen"; exit 1; }
echo "nst1 seed ok"
/tmp/nst1 || exit 1
