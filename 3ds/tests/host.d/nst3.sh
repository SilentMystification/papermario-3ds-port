#!/bin/sh
# NST3: nd_ds_cmp (3ds/src/nd_ds.c) on 3 cases (3ds/tools/nd_dschk_test.c). Prints `nst3 cmp ok 3/3`. The changed-prim case is the seeded fault.
F="-m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
gcc $F -o /tmp/nst3 3ds/tools/nd_dschk_test.c 3ds/src/nd_ds.c 3ds/src/nd_texwin.c 3ds/tools/nd_host_stub.c || { echo "nst3 FAIL build"; exit 1; }
/tmp/nst3 || exit 1
