#!/bin/sh
# NPSO2D: the pso table lookup (nd_psolk.c, nd_pso_find, nd_pso_in_eq, nd_pso_fix) against nd_comb_core (the old path) on the ndref states
# and the table of 3ds/tests/pso. The seeded fault (nd_pso_fix not applied) runs first and must report bad above 0; the real run must report
# bad 0 with entries above 0.
set -e
T=/tmp/psolk
mkdir -p "$T"
SSE="-msse2 -mfpmath=sse"
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -I src"
python3 3ds/tools/gen_pso_tab.py "$T/nd_pso_tab.c" 3ds/tests/pso
gcc -m32 $SSE -w $F -c -o "$T/ndcomb_d.o" 3ds/src/pm_comb.c
gcc -m32 $SSE -Wall -Wextra -Werror $F -o "$T/psolk" 3ds/tools/nd_psolk_test.c 3ds/src/nd_psolk.c 3ds/tools/nd_decide.c 3ds/src/nd_psokey.c 3ds/src/nd_tmpl.c "$T/nd_pso_tab.c" "$T/ndcomb_d.o" -lm
"$T/psolk" --fault 3ds/tests/ndref/ndref_*.bin > "$T/fault.txt" || true
if ! tail -1 "$T/fault.txt" | grep -q "^psolk entries [0-9]* neq [0-9]* bad [1-9][0-9]*$"; then echo "FAIL psolk: fault line not above 0: $(tail -1 "$T/fault.txt")"; exit 1; fi
tail -1 "$T/fault.txt"
if ! "$T/psolk" 3ds/tests/ndref/ndref_*.bin > "$T/real.txt"; then echo "FAIL psolk: $(tail -5 "$T/real.txt")"; exit 1; fi
if ! grep -q "^psolk find [0-9]* bad 0$" "$T/real.txt"; then echo "FAIL psolk find: $(grep "^psolk find" "$T/real.txt")"; exit 1; fi
if ! tail -1 "$T/real.txt" | grep -q "^psolk entries [1-9][0-9]* neq [0-9]* bad 0$"; then echo "FAIL psolk: $(tail -1 "$T/real.txt")"; exit 1; fi
tail -2 "$T/real.txt"
