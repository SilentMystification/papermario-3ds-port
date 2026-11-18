# HOSTSEL needs: 3ds/tests/host.d/conv2c.sh
#!/bin/sh
# FXLT: the tile size on the caller's image (NDN_TSZ) and the light units (NDN_LIT) are v2 ops (nd_gfx_conv.c v2_tilesize, v2_walk ND_FS_MW and
# ND_FS_MOVEMEM). Seeded faults: the first NDN_TSZ / NDN_LIT value + 1 in the compare's copy must give bad above 0. Reads /tmp/ndntab of conv2c.sh
# and /tmp/gconv of nd_host_test.sh.
for f in tsz lit; do
  /tmp/ndntab --fault-$f > /tmp/ndntab_$f.txt || true
  grep -q "^ntab tables [0-9]* draws [0-9]* bad [1-9]" /tmp/ndntab_$f.txt || { echo "fxlt FAIL: seeded $f fault not seen"; exit 1; }
done
for l in D_09001128_33B088 D_09004458_3FE908 D_09008CD0_3CA870 D_09006C98_39F858 D_09007188_39FD48; do
  grep -qx "$l" /tmp/gconv/gset_ok.txt || { echo "fxlt FAIL: $l is not gset ok"; exit 1; }
done
echo "fxlt ok"
