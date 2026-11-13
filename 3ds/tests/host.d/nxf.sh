#!/bin/sh
# NXF1: host test of nd_xf_push and nd_xf_pop against the gSPMatrix and gSPPopMatrix words (nd_op_xlate). Prints "nxf ok 9/9" or "nxf FAIL <case>".
gcc -m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/nxf 3ds/tools/nd_xf_test.c 3ds/src/nd_xf.c 3ds/src/nd_op.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o -lm && /tmp/nxf
