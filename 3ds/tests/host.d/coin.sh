#!/bin/sh
# COIN: nd_dual_keep and the item combine choice. The seed keeps dual_on: decal fails first.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
gcc -m32 -w $F -c -o /tmp/coin_comb.o 3ds/src/pm_comb.c
mkdir -p /tmp/coin_seed; sed 's/return comb_a == 1u \&\& comb_c == 2u;/(void)comb_a; (void)comb_c; return 1;/' 3ds/include/nd_dual.h > /tmp/coin_seed/nd_dual.h
gcc -m32 -w -I /tmp/coin_seed $F -o /tmp/coin_seed/t 3ds/tools/nd_dual_test.c 3ds/src/nd_decide.c 3ds/src/nd_tmpl.c /tmp/coin_comb.o -lm || exit 1
got=$(/tmp/coin_seed/t); if [ "$got" != "coin FAIL decal" ]; then echo "coin seed FAIL: got [$got]"; exit 1; fi
gcc -m32 -Wall -Wextra -Werror $F -o /tmp/coin_t 3ds/tools/nd_dual_test.c 3ds/src/nd_decide.c 3ds/src/nd_tmpl.c /tmp/coin_comb.o -lm && /tmp/coin_t || exit 1
