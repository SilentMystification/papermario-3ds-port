#!/bin/sh
# CONV2C: host compare of the v2 tables (nd_gtab_test.c, -DNTAB_V2) against the v1 tables on the reset walker state. The seeded fault (one NDN_COL value + 1 in the compare's copy)
# runs first and must report bad above 0. Needs /tmp/gconv of nd_host_test.sh (gen_gfx_tab.sh output, nd_gfx_tab.o, gconv_in.o).
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
FP="-msse2 -mfpmath=sse -ffp-contract=off"
gcc -m32 -w $F -c -o /tmp/ndcomb_c.o 3ds/src/pm_comb.c
gcc -m32 -w $F -I /tmp/gconv -c -o /tmp/gconv/nd_ntab_idx.o /tmp/gconv/nd_ntab_idx.c
gcc -m32 -Wall -Wextra -Werror $FP -DNTAB_V2 -DND_SCOMP_HOST $F -I 3ds/tools -o /tmp/ndntab 3ds/tools/nd_gtab_test.c 3ds/src/nd_gtab.c 3ds/src/nd_scomp.c 3ds/src/nd_texwin.c \
    3ds/src/nd_decide.c 3ds/src/nd_tmpl.c /tmp/ndcomb_c.o /tmp/gconv/nd_gfx_tab.o /tmp/gconv/nd_ntab_idx.o /tmp/gconv/gconv_in.o -lm
echo "seeded fault (one NDN_COL value + 1), bad above 0 expected:"
/tmp/ndntab --fault > /tmp/ndntab_fault.txt || true
if grep -q "^ntab tables [0-9]* draws [0-9]* bad 0$" /tmp/ndntab_fault.txt; then echo "seeded fault not seen"; exit 1; fi
grep "^ntab tables" /tmp/ndntab_fault.txt
/tmp/ndntab | grep -v "^gtab def"
