#!/bin/sh
# NTGT: nd_tgt_role (nd_tgt.h) equals the former img_role body for every kind bit combination (nd_tgt_test.c). Seeded fault: one branch changed in a copy must print ntgt FAIL.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
mkdir -p /tmp/ntgt_seed
sed 's/if (siz == 2 \&\& (kind \& (NDK_ZERO | NDK_INZ))/if (siz == 3 \&\& (kind \& (NDK_ZERO | NDK_INZ))/' 3ds/include/nd_tgt.h > /tmp/ntgt_seed/nd_tgt.h
if cmp -s 3ds/include/nd_tgt.h /tmp/ntgt_seed/nd_tgt.h; then echo "ntgt seed FAIL: sed changed nothing"; exit 1; fi
gcc -m32 -w -I /tmp/ntgt_seed $F -o /tmp/ntgt_seed/t 3ds/tools/nd_tgt_test.c || exit 1
got=$(/tmp/ntgt_seed/t | tail -1 | cut -c1-9); if [ "$got" != "ntgt FAIL" ]; then echo "ntgt seed FAIL: got [$got]"; exit 1; fi
gcc -m32 -Wall -Wextra $F -o /tmp/ntgt_t 3ds/tools/nd_tgt_test.c && /tmp/ntgt_t || exit 1
