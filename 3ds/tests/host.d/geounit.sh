#!/bin/sh
# GEOUNIT: host test of the target, depth, vertex, triangle and light calls (3ds/src/nd_geo.c) against the macros they replace (3ds/tools/nd_geo_test.c).
# Prints "geounit ok 14/14" or "geounit FAIL <shape>". First a seeded fault (the zsrc shift changed in a copy of nd_geo.c) must fail at zsrc; then the real link must pass.
FP="-msse2 -mfpmath=sse -ffp-contract=off"
FL="-m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
RO=/tmp # HOSTD2: read-only inputs that nd_host_test.sh makes before the host.d loop; the batch rule does not count $RO paths
LIBS="3ds/tools/nd_xf_link_stub.c 3ds/src/nd_xf.c 3ds/src/nd_rec.c 3ds/src/nd_snap_in.c 3ds/tools/nd_gx_host.c 3ds/src/nd_ds.c 3ds/src/nd_model.c 3ds/tools/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c 3ds/src/nd_texwin.c 3ds/tools/nd_host_stub.c $RO/ndcomb.o $RO/ndcombs.c $RO/ndlogs.c -lm"
sed 's/G_MDSFT_ZSRCSEL, 1)/G_MDSFT_ZSRCSEL, 2)/' 3ds/src/nd_geo.c > /tmp/nd_geo_seed.c
gcc $FL $FP -o /tmp/geounit_seed 3ds/tools/nd_geo_test.c /tmp/nd_geo_seed.c $LIBS || exit 1
got=$(/tmp/geounit_seed)
if [ "$got" != "geounit FAIL zsrc" ]; then echo "geounit seed FAIL: got [$got], want [geounit FAIL zsrc]"; exit 1; fi
gcc $FL $FP -o /tmp/geounit 3ds/tools/nd_geo_test.c 3ds/src/nd_geo.c $LIBS || exit 1
/tmp/geounit || exit 1
