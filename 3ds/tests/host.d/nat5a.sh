#!/bin/sh
# NAT5A: host test of the cut table of a flat copy (3ds/tools/nd_alt_et_test.c). A copy of nd_rec.c without the VTX cut must fail; the real link must print "nat5a cases 3 bad 0".
FP="-msse2 -mfpmath=sse -ffp-contract=off"
FL="-m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
RO=/tmp
LIBS="3ds/tools/nd_xf_link_stub.c 3ds/src/nd_xf.c 3ds/src/nd_rec.c 3ds/tools/nd_gx_host.c 3ds/src/nd_ds.c 3ds/src/nd_model.c 3ds/tools/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c 3ds/src/nd_texwin.c 3ds/tools/nd_host_stub.c $RO/ndcomb.o $RO/ndcombs.c $RO/ndlogs.c -lm"
sed 's/ || c == 1u;/;/' 3ds/src/nd_rec.c > /tmp/nat5a_rec_seed.c
cmp -s 3ds/src/nd_rec.c /tmp/nat5a_rec_seed.c && { echo "nat5a seed edit missed"; exit 1; }
LIBS_S=$(echo "$LIBS" | sed 's#3ds/src/nd_rec.c#/tmp/nat5a_rec_seed.c#')
gcc $FL $FP -o /tmp/nat5a_seed 3ds/tools/nd_alt_et_test.c 3ds/src/nd_st.c $LIBS_S || exit 1
/tmp/nat5a_seed > /tmp/nat5a_seed.txt; if [ $? -eq 0 ]; then echo "nat5a seed FAIL: the seed passed"; exit 1; fi
gcc $FL $FP -o /tmp/nat5a 3ds/tools/nd_alt_et_test.c 3ds/src/nd_st.c $LIBS || exit 1
/tmp/nat5a || exit 1
