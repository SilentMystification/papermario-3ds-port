#!/bin/sh
# WSCMB: the window style combine values (ND_CMB_W, ND_CMB_NOP) equal the gsDPSetCombineMode and gsDPNoOp words, all 18 pairs in use.
# Seeded fault: -DWSCMB_SEED changes one word; the test must then fail.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -DVERSION=us -D_MIPS_SZLONG=32 -Dnullptr=NULL -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -I src"
gcc -m32 -w $F -o /tmp/wscmb 3ds/tests/wscmb/wscmb.c || { echo "wscmb FAIL compile"; exit 1; }
/tmp/wscmb || { echo "wscmb FAIL"; exit 1; }
gcc -m32 -w $F -DWSCMB_SEED -o /tmp/wscmb_seed 3ds/tests/wscmb/wscmb.c || { echo "wscmb FAIL seed compile"; exit 1; }
if /tmp/wscmb_seed > /dev/null; then echo "wscmb FAIL seeded fault not seen"; exit 1; fi
echo "wscmb ok"
