#!/bin/sh
# HUNIT: host test of the value calls (nd_val.c) and the matrix calls (nd_xf.c) against the macros they replace (3ds/tools/nd_val_test.c).
# Prints "hunit ok 7/7" or "hunit FAIL <shape>". First a seeded fault (env records NDO_PRIM) must fail at env; then the real link must pass.
FP="-msse2 -mfpmath=sse -ffp-contract=off"
FL="-m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
LIBS="3ds/src/nd_xf.c 3ds/src/nd_rec.c 3ds/src/nd_model.c 3ds/src/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o /tmp/ndcombs.c /tmp/ndlogs.c -lm"
sed 's/NDO_ENV, 0u/NDO_PRIM, 0u/' 3ds/src/nd_val.c > /tmp/nd_val_seed.c
gcc $FL $FP -o /tmp/hunit_seed 3ds/tools/nd_val_test.c /tmp/nd_val_seed.c $LIBS || exit 1
got=$(/tmp/hunit_seed)
if [ "$got" != "hunit FAIL env" ]; then echo "hunit seed FAIL: got [$got], want [hunit FAIL env]"; exit 1; fi
gcc $FL $FP -o /tmp/hunit 3ds/tools/nd_val_test.c 3ds/src/nd_val.c $LIBS || exit 1
/tmp/hunit || exit 1
