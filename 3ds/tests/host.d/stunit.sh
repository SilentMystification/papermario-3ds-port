#!/bin/sh
# STUNIT: host test of the state calls (nd_st.c) against the macros they replace (3ds/tools/nd_st_test.c).
# Prints "stunit ok 18/18" or "stunit FAIL <shape>". First a seeded fault (the cyc shift changed in a copy of nd_st.c) must fail at cyc; then the real link must pass.
FP="-msse2 -mfpmath=sse -ffp-contract=off"
FL="-m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
LIBS="3ds/src/nd_xf.c 3ds/src/nd_rec.c 3ds/src/nd_model.c 3ds/src/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o /tmp/ndcombs.c /tmp/ndlogs.c -lm"
sed 's/G_MDSFT_CYCLETYPE, 2)/G_MDSFT_CYCLETYPE, 3)/' 3ds/src/nd_st.c > /tmp/nd_st_seed.c
gcc $FL $FP -o /tmp/stunit_seed 3ds/tools/nd_st_test.c /tmp/nd_st_seed.c $LIBS || exit 1
got=$(/tmp/stunit_seed)
if [ "$got" != "stunit FAIL cyc" ]; then echo "stunit seed FAIL: got [$got], want [stunit FAIL cyc]"; exit 1; fi
gcc $FL $FP -o /tmp/stunit 3ds/tools/nd_st_test.c 3ds/src/nd_st.c $LIBS || exit 1
/tmp/stunit || exit 1
