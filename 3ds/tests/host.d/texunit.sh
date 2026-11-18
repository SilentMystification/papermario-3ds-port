#!/bin/sh
# TEXUNIT: host test of the texture, rectangle, viewport and colour setters (3ds/include/nd_tx.h) against the gbi macros they replace
# (3ds/tools/nd_tx_test.c). Prints "texunit ok 36/36" or "texunit FAIL <shape>". First a seeded fault (the tlut16 load's tlut count
# 15 changed to 14 in a copy of nd_tx.h) must fail at tlut16; then the real header must pass.
FP="-msse2 -mfpmath=sse -ffp-contract=off"
FL="-m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
RO=/tmp # HOSTD2: read-only inputs that nd_host_test.sh makes before the host.d loop; the batch rule does not count $RO paths
LIBS="3ds/tools/nd_xf_link_stub.c 3ds/src/nd_xf.c 3ds/src/nd_rec.c 3ds/src/nd_ds.c 3ds/src/nd_model.c 3ds/tools/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c $RO/ndcomb.o $RO/ndcombs.c $RO/ndlogs.c -lm"
mkdir -p /tmp/txseed
sed 's/G_TX_LOADTILE, 15)/G_TX_LOADTILE, 14)/' 3ds/include/nd_tx.h > /tmp/txseed/nd_tx.h
gcc -I /tmp/txseed $FL $FP -o /tmp/texunit_seed 3ds/tools/nd_tx_test.c $LIBS || exit 1
got=$(/tmp/texunit_seed)
if [ "$got" != "texunit FAIL tlut16" ]; then echo "texunit seed FAIL: got [$got], want [texunit FAIL tlut16]"; exit 1; fi
gcc $FL $FP -o /tmp/texunit 3ds/tools/nd_tx_test.c $LIBS || exit 1
/tmp/texunit || exit 1
