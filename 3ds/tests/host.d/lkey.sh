#!/bin/sh
# LKEY: nd_lkey.h gives the device units of a moved list key (nd_lkey_test.c). The seed (no entry test) must print lkey FAIL case 0.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I src -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
mkdir -p /tmp/lkey_seed
sed 's/if (e) {/if (0) {/' 3ds/include/nd_lkey.h > /tmp/lkey_seed/nd_lkey.h
gcc -m32 -w -I /tmp/lkey_seed $F -o /tmp/lkey_seed/t 3ds/tools/nd_lkey_test.c || exit 1
got=$(/tmp/lkey_seed/t); if [ "$got" != "lkey FAIL case 0" ]; then echo "lkey seed FAIL: got [$got]"; exit 1; fi
gcc -m32 -Wall -Wextra $F -o /tmp/lkey_t 3ds/tools/nd_lkey_test.c && /tmp/lkey_t || exit 1
