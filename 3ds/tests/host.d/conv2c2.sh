# HOSTSEL needs: 3ds/tests/host.d/conv2c.sh
#!/bin/sh
# CONV2C2 C2b: the seeded v2 tables (one per site desc of an inheriting list) against the v1 tables seeded the same way. Needs the
# /tmp/ndntab binary and the --fault output of conv2c.sh (it runs first, name order). The seeded fault (key_c ^ 1 in the model of every
# seeded table) must fail every seeded table; the normal run must have no bad seeded table.
grep -qE "^ntab seeded ([1-9][0-9]*) bad \1$" /tmp/ndntab_fault.txt || { echo "seed fault not seen"; exit 1; }
/tmp/ndntab > /tmp/ndntab_ok.txt || { echo "conv2c2 FAIL ndntab exit"; exit 1; }
grep -qE "^ntab seeded [1-9][0-9]* bad 0$" /tmp/ndntab_ok.txt || { echo "conv2c2 FAIL seeded bad or none"; grep "^ntab seeded" /tmp/ndntab_ok.txt; exit 1; }
grep -E "^ntab (seeded|tables|inherit)" /tmp/ndntab_ok.txt
echo "conv2c2 ok"
