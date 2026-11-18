#!/bin/sh
# NREC2B: a memo hit (scm_replay) and a core 0 decision (nd_direct_bind) write sl_const, sl_env, sl_cc as nd_comb_apply does. Prints `nrec2b ok 2`.
# Seeded fault: the sl_const write in scm_replay removed must fail check A first.
L1='    sl_const = m->cc_on; sl_env = m->env_on; if (m->cc_on) sl_cc = m->cc; /* NREC2B: a memo hit leaves sl_* as nd_comb_apply does (snapchk sl) */'
L2='        sl_const = o->cc_on; sl_env = o->env_on; if (o->cc_on) sl_cc = o->cc; /* NREC2B: as nd_comb_apply */'
N=/tmp/nrec2b; rm -rf $N; mkdir -p $N/seed
chk_a() { [ "$(grep -cF "$L1" "$1")" = 1 ]; }
chk_a 3ds/src/nd_comb.c || { echo "nrec2b FAIL A"; exit 1; }
[ "$(grep -cF "$L2" 3ds/src/nd_direct.c)" = 1 ] || { echo "nrec2b FAIL B"; exit 1; }
sed 's/sl_const = m->cc_on; //' 3ds/src/nd_comb.c > $N/seed/nd_comb.c
cmp -s $N/seed/nd_comb.c 3ds/src/nd_comb.c && { echo "nrec2b FAIL seed"; exit 1; }
if chk_a $N/seed/nd_comb.c; then echo "nrec2b FAIL seed"; exit 1; fi
echo "nrec2b ok 2"
