#!/bin/sh
# NPSO3: gen_pso_tab (3ds/tools/gen_pso_tab.py) on 3ds/tests/pso, then nd_pso (table) against nd_pso_build. The seeded fault
# (each id compared with the other description) must report bad above 0; the real run must report bad 0.
set -e
T=/tmp/psotab
mkdir -p "$T"
F="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -I src"
python3 3ds/tools/gen_pso_tab.py "$T/nd_pso_tab.c" 3ds/tests/pso
gcc -m32 -w $F -c -o "$T/ndcomb_d.o" 3ds/src/pm_comb.c
gcc -m32 -Wall -Wextra -Werror $F -o "$T/psotab" 3ds/tools/nd_psotab_test.c 3ds/src/nd_decide.c 3ds/src/nd_tmpl.c "$T/nd_pso_tab.c" "$T/ndcomb_d.o" -lm
"$T/psotab" --fault > "$T/fault.txt" || true
if ! grep -q "^psotab entries 2 bad [1-9][0-9]*$" "$T/fault.txt"; then echo "FAIL psotab: fault line not above 0: $(cat "$T/fault.txt")"; exit 1; fi
tail -1 "$T/fault.txt"
"$T/psotab" > "$T/real.txt"
if ! grep -q "^psotab entries 2 bad 0$" "$T/real.txt"; then echo "FAIL psotab: $(cat "$T/real.txt")"; exit 1; fi
tail -1 "$T/real.txt"
