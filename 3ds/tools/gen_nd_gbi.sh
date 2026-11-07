#!/bin/sh
# gen_nd_gbi.sh: regenerate 3ds/include/nd_gbi_leaf.h (see gen_nd_gbi.py). Needs docker image pm3ds-host:1 and python.
cd "$(dirname "$0")/../.." || exit 2
ROOT=$(pwd -W 2>/dev/null || pwd)
MSYS_NO_PATHCONV=1 docker run --rm -v "$ROOT:/w" -w /w pm3ds-host:1 sh -c 'gcc -m32 -E -dM -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -Iinclude -I3ds/include -I3ds/stub_include -include ultra64.h -x c /dev/null' > build3ds/nd_gbi_macros.txt || exit 1
PY=/c/Python314/python; [ -x "$PY" ] || PY=python3
$PY 3ds/tools/gen_nd_gbi.py build3ds/nd_gbi_macros.txt 3ds/include/nd_gbi_leaf.h 3ds/include/nd_rec_leaf.h
