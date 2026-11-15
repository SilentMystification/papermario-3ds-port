#!/bin/sh
# LIT1: a list with a Lights1 light unit gets a v2 table (nd_gfx_conv.c v2_walk NDO_MOVEMEM). Reads /tmp/gconv of nd_host_test.sh.
for l in D_802A76E8_7A5628 D_802A20D0_7A9A70; do
  grep -qx "$l" /tmp/gconv/ntab_tables.txt || { echo "lit1 FAIL: $l has no v2 table"; exit 1; }
done
echo "lit1 ok"
