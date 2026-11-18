#!/bin/sh
# NS3UNIT: host test of the O forms (3ds/include/nd_at.h, nd_tx.h, nd_nr.h) against the gbi macros they replace (3ds/tools/nd_at_test.c).
# Prints "ns3unit ok 27/27" or "ns3unit FAIL <shape>". First a seeded fault (the texture rectangle flag dropped from the O form in a copy of
# nd_tx.h) must fail at rtex; then the real headers must pass.
FP="-msse2 -mfpmath=sse -ffp-contract=off"
FL="-m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
RO=/tmp # HOSTD2: read-only inputs that nd_host_test.sh makes before the host.d loop; the batch rule does not count $RO paths
LIBS="3ds/tools/nd_xf_link_stub.c 3ds/src/nd_xf.c 3ds/src/nd_rec.c 3ds/src/nd_ds.c 3ds/src/nd_model.c 3ds/tools/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c 3ds/src/nd_texwin.c 3ds/tools/nd_host_stub.c $RO/ndcomb.o $RO/ndcombs.c $RO/ndlogs.c -lm"
mkdir -p /tmp/n3seed
sed 's/ND_FS_RECT_TEX | ND_NR_TXY(tile, xl, yl)/ND_NR_TXY(tile, xl, yl)/' 3ds/include/nd_tx.h > /tmp/n3seed/nd_tx.h
gcc -I /tmp/n3seed $FL $FP -o /tmp/ns3unit_seed 3ds/tools/nd_at_test.c $LIBS || exit 1
got=$(/tmp/ns3unit_seed)
if [ "$got" != "ns3unit FAIL rtex" ]; then echo "ns3unit seed FAIL: got [$got], want [ns3unit FAIL rtex]"; exit 1; fi
gcc $FL $FP -o /tmp/ns3unit 3ds/tools/nd_at_test.c $LIBS || exit 1
/tmp/ns3unit || exit 1
