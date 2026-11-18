#!/bin/sh
# MAPCONV: every map and battle shape list of the ROM has a .dlc section that the device loader turns into the compile result of the list (nd_dlc_compile).
# Steps: the converter (bin mode: assets/us/mapfs/geom/*_shape.bin, the decode of the ROM assets) writes the .nds, .seam and .dlc files; mapconv_test loads each
# section with the dlcp_* code of 3ds/src/nd_shape.c (cut out here, the device code itself) and compares. Prints "mapconv ok <n>". Seeded fault: one index byte of one
# section changed in the loaded file must print FAIL. Runs in the nd_host_test.sh docker block (cwd /work, gcc -m32, set -e).
FL="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
FP="-msse2 -mfpmath=sse -ffp-contract=off"
mc=/tmp/mapconv
rm -rf $mc
mkdir -p $mc/out
gcc -m32 -O2 -Wall $FP $FL -o $mc/conv 3ds/tools/nd_shape_conv.c 3ds/src/nd_seam.c 3ds/src/nd_dlc.c || exit 1
$mc/conv /work $mc/out bin > $mc/conv.txt || { cat $mc/conv.txt; echo "mapconv FAIL: converter"; exit 1; }
grep '^shapeconv\|^dlc lists' $mc/conv.txt
tr -d '\r' < 3ds/src/nd_shape.c | sed -n '/^typedef struct { u32 list, var, off, size; } DlcPreIdx;/,/^\/\* the dlcpre line: per frame/p' > $mc/loader.inc
grep -q 'static int dlcp_load' $mc/loader.inc || { echo "mapconv FAIL: loader cut"; exit 1; }
gcc -m32 -g -O1 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable $FP $FL -DLOADER_INC="\"$mc/loader.inc\"" -o $mc/test 3ds/tests/host.d/mapconv/mapconv_test.c 3ds/src/nd_seam.c 3ds/src/nd_dlc.c -lm || exit 1
$mc/test $mc/out || exit 1
if $mc/test $mc/out --seed > $mc/seed.txt 2>&1; then echo "mapconv seed FAIL: the changed index byte was not detected"; exit 1; fi
grep -q 'mapconv FAIL' $mc/seed.txt || { echo "mapconv seed FAIL: no FAIL line"; cat $mc/seed.txt; exit 1; }
echo "mapconv seed detected"
