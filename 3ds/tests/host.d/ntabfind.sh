#!/bin/sh
# NTABFIND: nd_ndev_pack (nd_gtab.h) copies a built NdNDev into its record. The seed (the record holds the entry address) must fail case 0.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I src -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
mkdir -p /tmp/ntabfind_seed
sed 's/memcpy(rec + 4, e, sizeof \*e);/memcpy(rec + 4, \&e, sizeof e);/' 3ds/include/nd_gtab.h > /tmp/ntabfind_seed/nd_gtab.h
if cmp -s 3ds/include/nd_gtab.h /tmp/ntabfind_seed/nd_gtab.h; then echo "ntabfind seed FAIL: the sed changed nothing"; exit 1; fi
gcc -m32 -w -I /tmp/ntabfind_seed $F -o /tmp/ntabfind_seed/t 3ds/tools/nd_ndev_rec_test.c || exit 1
got=$(/tmp/ntabfind_seed/t); if [ "$got" != "ndev FAIL case 0" ]; then echo "ntabfind seed FAIL: got [$got]"; exit 1; fi
gcc -m32 -Wall -Wextra $F -o /tmp/ntabfind_t 3ds/tools/nd_ndev_rec_test.c && /tmp/ntabfind_t || exit 1
echo "ntabfind ok"
