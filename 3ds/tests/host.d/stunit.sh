#!/bin/sh
# STUNIT: host test of the state calls (nd_st.c) against the macros they replace (3ds/tools/nd_st_test.c).
# Prints "stunit ok 20/20" or "stunit FAIL <shape>". First a seeded fault (the cyc shift changed in a copy of nd_st.c) must fail at cyc; then the real link must pass.
FP="-msse2 -mfpmath=sse -ffp-contract=off"
FL="-m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
RO=/tmp # HOSTD2: read-only inputs that nd_host_test.sh makes before the host.d loop; the batch rule does not count $RO paths
LIBS="3ds/tools/nd_xf_link_stub.c 3ds/src/nd_xf.c 3ds/src/nd_rec.c 3ds/src/nd_ds.c 3ds/src/nd_model.c 3ds/tools/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c 3ds/src/nd_texwin.c 3ds/tools/nd_host_stub.c $RO/ndcomb.o $RO/ndcombs.c $RO/ndlogs.c -lm"
sed 's/G_MDSFT_CYCLETYPE, 2)/G_MDSFT_CYCLETYPE, 3)/' 3ds/src/nd_st.c > /tmp/nd_st_seed.c
gcc $FL $FP -o /tmp/stunit_seed 3ds/tools/nd_st_test.c /tmp/nd_st_seed.c $LIBS || exit 1
got=$(/tmp/stunit_seed)
if [ "$got" != "stunit FAIL cyc" ]; then echo "stunit seed FAIL: got [$got], want [stunit FAIL cyc]"; exit 1; fi
sed 's/p\[0\] = NDC_HDR(NDC_NAPI, 1, 0);/p[0] = NDC_HDR(NDC_NRUN, 1, 0);/' 3ds/src/nd_rec.c > /tmp/nd_rec_seed.c
LIBS_RS=$(echo "$LIBS" | sed 's#3ds/src/nd_rec.c#/tmp/nd_rec_seed.c#')
gcc $FL $FP -o /tmp/stunit_rseed 3ds/tools/nd_st_test.c 3ds/src/nd_st.c $LIBS_RS || exit 1
got=$(/tmp/stunit_rseed)
if [ "$got" != "stunit FAIL run" ]; then echo "stunit run seed FAIL: got [$got], want [stunit FAIL run]"; exit 1; fi
sed 's/if (x0 == ND_NOP_A) return;/(void)0;/' 3ds/src/nd_st.c > /tmp/nd_st_useed.c # GFXPOS7: nd_pso_unit records the no-op unit
gcc $FL $FP -o /tmp/stunit_useed 3ds/tools/nd_st_test.c /tmp/nd_st_useed.c $LIBS || exit 1
got=$(/tmp/stunit_useed)
if [ "$got" != "stunit FAIL unop" ]; then echo "stunit unit seed FAIL: got [$got], want [stunit FAIL unop]"; exit 1; fi
gcc $FL $FP -o /tmp/stunit 3ds/tools/nd_st_test.c 3ds/src/nd_st.c $LIBS || exit 1
/tmp/stunit || exit 1
