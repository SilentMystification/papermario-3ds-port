#!/bin/sh
# PMOVE: the device gfx table (nd_gfx_tab.c of gen_gfx_tab.sh) has a moved particle list as &ndd_<name> (the v1 table key). Prints the count n of the
# device key lines. The seeded fault (one extra key line in a copy) must count 1 more than the real file.
n=$(grep -c "(const uint32_t\*)&ndd_" /tmp/gconv/nd_gfx_tab.c)
cp /tmp/gconv/nd_gfx_tab.c /tmp/gconv/pmove_seed.c || exit 1
printf 'const NdGTab pmove_seed_x = { (const uint32_t*)&ndd_pmove_seed_x, 0, 0, 0, 0, 0 };\n' >> /tmp/gconv/pmove_seed.c
s=$(grep -c "(const uint32_t\*)&ndd_" /tmp/gconv/pmove_seed.c)
if [ "$s" != "$((n + 1))" ]; then echo "pmove seed FAIL: real $n seeded $s"; exit 1; fi
echo "pmove ok $n"
