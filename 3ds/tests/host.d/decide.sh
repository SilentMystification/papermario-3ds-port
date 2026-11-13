#!/bin/sh
# NPSO2: nd_pso_build (nd_decide.c) against the old path on the ndref state words. The seeded fault (one desc field not copied) runs first and must report bad above 0.
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include"
gcc -m32 -w $F -c -o /tmp/ndcomb_d.o 3ds/src/pm_comb.c
gcc -m32 -Wall -Wextra -Werror $F -o /tmp/nddec 3ds/tools/nd_decide_test.c 3ds/src/nd_decide.c 3ds/src/nd_tmpl.c /tmp/ndcomb_d.o -lm
echo "seeded fault (tex_on not copied), bad above 0 expected:"
/tmp/nddec --fault 3ds/tests/ndref/ndref_*.bin > /tmp/nddec_fault.txt
if grep -q "^decide entries [0-9]* bad 0$" /tmp/nddec_fault.txt; then echo "seeded fault not seen"; exit 1; fi
tail -1 /tmp/nddec_fault.txt
/tmp/nddec 3ds/tests/ndref/ndref_*.bin
