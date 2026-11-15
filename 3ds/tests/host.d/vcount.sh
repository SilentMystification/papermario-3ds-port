#!/bin/sh
# VCOUNT: nd_vcount (nd_vcount.h) counts the device units of a moved list (mdl_get_vertex_count). The seed (no unit bound) must fail case 0.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I src -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
mkdir -p /tmp/vcount_seed
sed 's/while (k < n)/while (1)/' 3ds/include/nd_vcount.h > /tmp/vcount_seed/nd_vcount.h
gcc -m32 -w -I /tmp/vcount_seed $F -o /tmp/vcount_seed/t 3ds/tools/nd_vcount_test.c || exit 1
got=$(/tmp/vcount_seed/t); if [ "$got" != "vcount FAIL case 0" ]; then echo "vcount seed FAIL: got [$got]"; exit 1; fi
gcc -m32 -Wall -Wextra $F -o /tmp/vcount_t 3ds/tools/nd_vcount_test.c && /tmp/vcount_t || exit 1
