#!/bin/sh
# PBUF: the effect part buffer units (nd_at.h, nd_tx.h forms into a local buffer) equal the gbi macro units, 6 shapes x 9 values.
# Seeded fault: -DPBUF_SEED changes one tile size argument; the test must then fail.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -DVERSION=us -D_MIPS_SZLONG=32 -Dnullptr=NULL -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -I src"
gcc -m32 -w $F -o /tmp/pbuf 3ds/tests/pbuf/pbuf.c || { echo "pbuf FAIL compile"; exit 1; }
/tmp/pbuf || { echo "pbuf FAIL"; exit 1; }
gcc -m32 -w $F -DPBUF_SEED -o /tmp/pbuf_seed 3ds/tests/pbuf/pbuf.c || { echo "pbuf FAIL seed compile"; exit 1; }
if /tmp/pbuf_seed > /dev/null; then echo "pbuf FAIL seeded fault not seen"; exit 1; fi
echo "pbuf ok"
