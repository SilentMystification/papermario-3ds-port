#!/bin/sh
# NSET1: nd_sv_enc and nd_sv_sync (3ds/src/nd_ds.c) on 15 fixed cases against a table (3ds/tools/nd_sv_test.c). Prints `nset1 cases 15 bad <b>`.
# Seeded fault: --fault changes one expected value, bad must be above 0 first.
F="-m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
mkdir -p build3ds/out
gcc $F -o build3ds/out/nset1_bin 3ds/tools/nd_sv_test.c 3ds/src/nd_ds.c 3ds/src/nd_texwin.c || { echo "nset1 FAIL build"; exit 1; }
if build3ds/out/nset1_bin --fault > build3ds/out/nset1_fault.txt; then echo "nset1 FAIL: the seeded fault passed"; exit 1; fi
grep -q "bad [1-9]" build3ds/out/nset1_fault.txt || { echo "nset1 FAIL: fault run gave no bad count"; exit 1; }
echo "nset1 seed ok"
build3ds/out/nset1_bin || exit 1
