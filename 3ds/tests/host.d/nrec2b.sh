#!/bin/sh
# NREC2B: a core 0 decision (nd_direct_bind) writes sl_const, sl_env, sl_cc as nd_comb_apply does. Prints `nrec2b ok 1`.
# Seeded fault: the sl_const write in nd_direct_bind removed must fail check B.
L2='        sl_const = o->cc_on; sl_env = o->env_on; if (o->cc_on) sl_cc = o->cc; /* NREC2B: as nd_comb_apply */'
N=/tmp/nrec2b; rm -rf $N; mkdir -p $N/seed
chk_b() { [ "$(grep -cF "$L2" "$1")" = 1 ]; }
chk_b 3ds/src/nd_direct.c || { echo "nrec2b FAIL B"; exit 1; }
sed 's/sl_const = o->cc_on; //' 3ds/src/nd_direct.c > $N/seed/nd_direct.c
cmp -s $N/seed/nd_direct.c 3ds/src/nd_direct.c && { echo "nrec2b FAIL seed"; exit 1; }
if chk_b $N/seed/nd_direct.c; then echo "nrec2b FAIL seed"; exit 1; fi
echo "nrec2b ok 1"
