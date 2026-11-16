#!/bin/sh
# MODELRM: the device render mode entries and defs (nd_gfx_tab.c of the gconv step, nd_rmtab.c) equal the frozen old lists
# (3ds/tests/modelrm/old_lists.c) word for word. Needs /tmp/gconv from nd_host_test.sh. Prints `modelrm ok <n>`. Seeded fault: one changed
# word in the old copy must print FAIL.
set -e
DEF="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -DVERSION=us -D_MIPS_SZLONG=32 -Dnullptr=NULL"
INC="-I 3ds/tools/gconv_inc -I 3ds/tools -I 3ds/include -I 3ds/stub_include -I include -I src -I assets/us -I ver/us/build/include -I ."
M=/tmp/modelrm; rm -rf $M; mkdir -p $M
run() { # $1 = frozen source
  gcc -m32 -w $DEF -DND_GCONV_IN $INC -c -o $M/old.o "$1"
  objcopy --prefix-symbols=old_ $M/old.o $M/old_p.o
  gcc -m32 -Wall -Wextra -Werror $DEF -DND_HOST_GFX -I 3ds/include -I 3ds/stub_include -I include -I ver/us/build/include -c -o $M/rmtab.o 3ds/src/nd_rmtab.c
  gcc -m32 -Wall -Wextra -Werror -DND_SCOMP_HOST $DEF -I 3ds/tools -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o $M/t 3ds/tools/modelrm_test.c $M/rmtab.o $M/old_p.o 3ds/src/nd_scomp.c 3ds/src/nd_texwin.c /tmp/gconv/nd_gfx_tab.o /tmp/gconv/gconv_in.o -Wl,--gc-sections
  $M/t
}
run 3ds/tests/modelrm/old_lists.c
sed '0,/G_CYC_1CYCLE/s//G_CYC_2CYCLE/' 3ds/tests/modelrm/old_lists.c > $M/seed.c
cmp -s $M/seed.c 3ds/tests/modelrm/old_lists.c && { echo "modelrm seeded fault not made"; exit 1; }
if run $M/seed.c > $M/seed.out 2>&1; then echo "modelrm seeded fault not seen"; exit 1; fi
grep -q "modelrm FAIL" $M/seed.out || { echo "modelrm seeded fault: no FAIL line"; cat $M/seed.out; exit 1; }
