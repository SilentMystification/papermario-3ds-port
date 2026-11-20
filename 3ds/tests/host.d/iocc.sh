#!/bin/sh
# IOCC (GD1): pm_interp_open node ids from the occurrence table (3ds/src/pm_interp.c) against the old full scan (3ds/tools/pm_interp_occ_test.c).
# Prints `iocc frames 5 nodes <n> capn <x> capd <y> bad <b>`. Seeded fault: --fault changes one reference id, bad must be above 0 first.
F="-m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
mkdir -p build3ds/out
gcc $F -o build3ds/out/iocc_bin 3ds/tools/pm_interp_occ_test.c 3ds/src/pm_interp.c || { echo "iocc FAIL build"; exit 1; }
if build3ds/out/iocc_bin --fault > build3ds/out/iocc_fault.txt; then echo "iocc FAIL: the seeded fault passed"; exit 1; fi
grep -q "bad [1-9]" build3ds/out/iocc_fault.txt || { echo "iocc FAIL: fault run gave no bad count"; exit 1; }
echo "iocc seed ok"
build3ds/out/iocc_bin || exit 1
