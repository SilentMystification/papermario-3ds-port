#!/bin/sh
# HUNIT: host test of the value calls (nd_val.c) and the matrix calls (nd_xf.c) against the macros they replace (3ds/tools/nd_val_test.c).
# Prints "hunit ok 7/7" or "hunit FAIL <shape>". First a seeded fault (env records ND_FS_PRIM) must fail at env; then the real link must pass.
FP="-msse2 -mfpmath=sse -ffp-contract=off"
FL="-m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
RO=/tmp # HOSTD2: read-only inputs that nd_host_test.sh makes before the host.d loop; the batch rule does not count $RO paths
LIBS="3ds/src/nd_xf.c 3ds/src/nd_rec.c 3ds/src/nd_ds.c 3ds/src/nd_model.c 3ds/tools/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c 3ds/src/nd_texwin.c 3ds/tools/nd_host_stub.c $RO/ndcomb.o $RO/ndcombs.c $RO/ndlogs.c -lm"
# NSTREAM3: the env record is an O form now (nd_at.h NDVAL_ENV), so the seed is a copy of nd_at.h in a first include dir.
mkdir -p /tmp/hunit_seed_inc
sed 's/ND_FS_ENV, 0u/ND_FS_PRIM, 0u/' 3ds/include/nd_at.h > /tmp/hunit_seed_inc/nd_at.h
gcc -I /tmp/hunit_seed_inc $FL $FP -o /tmp/hunit_seed 3ds/tools/nd_val_test.c 3ds/src/nd_val.c $LIBS || exit 1
got=$(/tmp/hunit_seed)
if [ "$got" != "hunit FAIL env" ]; then echo "hunit seed FAIL: got [$got], want [hunit FAIL env]"; exit 1; fi
gcc $FL $FP -o /tmp/hunit 3ds/tools/nd_val_test.c 3ds/src/nd_val.c $LIBS || exit 1
/tmp/hunit || exit 1
