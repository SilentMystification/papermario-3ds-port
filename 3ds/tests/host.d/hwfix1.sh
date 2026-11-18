#!/bin/sh
# HWFIX1: a pso table miss decides with nd_comb_core (nd_comb.c nd_comb_pick). Prints `hwfix1 ok 3`.
# Seeded fault: the miss call replaced by pso_fallback must fail check A first.
L='    if (psooff_on() || !nd_pso_find(in, (unsigned)pm_comb_vprim, o)) { pso_seen_note(in); pk_miss++; nd_comb_core(in, (unsigned)pm_comb_vprim, comb_mode5, o); return; }'
N=/tmp/hwfix1; rm -rf $N; mkdir -p $N/seed
chk_a() { [ "$(grep -cF "$L" "$1")" = 1 ]; }
chk_a 3ds/src/nd_comb.c || { echo "hwfix1 FAIL A"; exit 1; }
sed 's/nd_comb_core(in, (unsigned)pm_comb_vprim, comb_mode5, o)/pso_fallback(in, o)/' 3ds/src/nd_comb.c > $N/seed/nd_comb.c
cmp -s $N/seed/nd_comb.c 3ds/src/nd_comb.c && { echo "hwfix1 seeded fault not made"; exit 1; }
if chk_a $N/seed/nd_comb.c; then echo "hwfix1 seeded fault not seen"; exit 1; fi
grep -rq 'pso_fallback' 3ds/src && { echo "hwfix1 FAIL B"; exit 1; }
grep -q '^void nd_comb_core(' 3ds/src/nd_decide.c || { echo "hwfix1 FAIL C"; exit 1; }
echo "hwfix1 ok 3"
