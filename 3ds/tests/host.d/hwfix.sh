#!/bin/sh
# HWFIX: host tests of pm_vp_snap (3ds/tools/pm_vpsnap_test.c). A seeded fault (the dedupe loop empty) must fail at dedupe first.
FL="-m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
mkdir -p /tmp/vps_seed
sed 's/k < n; k++/k < 0u; k++/' 3ds/src/pm_vpsnap.c > /tmp/vps_seed/pm_vpsnap.c
gcc $FL -o /tmp/vps_seed/t 3ds/tools/pm_vpsnap_test.c /tmp/vps_seed/pm_vpsnap.c || exit 1
got=$(/tmp/vps_seed/t)
if [ "$got" != "vpsnap FAIL dedupe" ]; then echo "vpsnap seed FAIL: got [$got]"; exit 1; fi
gcc $FL -o /tmp/vps_t 3ds/tools/pm_vpsnap_test.c 3ds/src/pm_vpsnap.c || exit 1
/tmp/vps_t || exit 1
