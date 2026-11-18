#!/bin/sh
# PIPEFX: nd_imgfx_type (nd_imgfx.h) types the imgfx anim list in place. The seed (no ND_X0) must fail case 0.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I src -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
mkdir -p /tmp/pipefx_seed
sed 's/ND_X0(w0, w1)/w0/' 3ds/include/nd_imgfx.h > /tmp/pipefx_seed/nd_imgfx.h
gcc -m32 -w -I /tmp/pipefx_seed $F -o /tmp/pipefx_seed/t 3ds/tools/nd_imgfx_test.c || exit 1
got=$(/tmp/pipefx_seed/t); if [ "$got" != "imgfx FAIL case 0" ]; then echo "pipefx seed FAIL: got [$got]"; exit 1; fi
gcc -m32 -Wall -Wextra $F -o /tmp/pipefx_t 3ds/tools/nd_imgfx_test.c && /tmp/pipefx_t || exit 1
