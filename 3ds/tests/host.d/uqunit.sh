#!/bin/sh
# UQUNIT (L1Q1): host test of the setter unit queue (3ds/src/nd_rec.c nd_rec_q, nd_uq_sync) against the immediate form nd_rec_nr (3ds/tools/nd_uq_test.c).
# Prints "uqunit ok 3/3" or "uqunit FAIL <shape>". First a seeded fault (rec_open without the queue sync, in a copy of nd_rec.c) must fail at q300; then the real link must pass.
FP="-msse2 -mfpmath=sse -ffp-contract=off"
FL="-m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
RO=/tmp # HOSTD2: read-only inputs that nd_host_test.sh makes before the host.d loop; the batch rule does not count $RO paths
LIBS="3ds/tools/nd_xf_link_stub.c 3ds/src/nd_xf.c 3ds/src/nd_rec.c 3ds/src/nd_snap_in.c 3ds/tools/nd_gx_host.c 3ds/src/nd_ds.c 3ds/src/nd_model.c 3ds/tools/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c 3ds/src/nd_texwin.c 3ds/tools/nd_host_stub.c $RO/ndcomb.o $RO/ndcombs.c $RO/ndlogs.c -lm"
LIBS_SEED=$(echo "$LIBS" | sed 's# 3ds/src/nd_rec.c # /tmp/nd_rec_uqseed.c #')
sed 's/static void rec_open(void) { nd_uq_sync();/static void rec_open(void) {/' 3ds/src/nd_rec.c > /tmp/nd_rec_uqseed.c
gcc $FL $FP -o /tmp/uqunit_seed 3ds/tools/nd_uq_test.c 3ds/src/nd_geo.c $LIBS_SEED || exit 1
got=$(/tmp/uqunit_seed)
if [ "$got" != "uqunit FAIL q300" ]; then echo "uqunit seed FAIL: got [$got], want [uqunit FAIL q300]"; exit 1; fi
gcc $FL $FP -o /tmp/uqunit 3ds/tools/nd_uq_test.c 3ds/src/nd_geo.c $LIBS || exit 1
/tmp/uqunit || exit 1
