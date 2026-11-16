#!/bin/sh
# FLIPLOG: the flipfps row (pm_flipfps.c) and the switch gate of its 3 hooks in pm_gpu.c, with a seeded fault.
gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/fliplog 3ds/tools/pm_flipfps_test.c 3ds/src/pm_flipfps.c || { echo "fliplog FAIL: compile"; exit 1; }
/tmp/fliplog || exit 1
chk() { n=$(grep -cE 'pm_ff_(begin|wait|end)\(' "$1"); g=$(grep -E 'pm_ff_(begin|wait|end)\(' "$1" | grep -cE '__builtin_expect\((pm_ff_on\(\)|ff_on), 0\)'); [ "$n" = 3 ] && [ "$g" = 3 ] && echo ok || echo bad; }
mkdir -p /tmp/fliplog_seed
sed 's/if (__builtin_expect(pm_ff_on(), 0)) pm_ff_begin/pm_ff_begin/' 3ds/src/pm_gpu.c > /tmp/fliplog_seed/pm_gpu.c
[ "$(chk /tmp/fliplog_seed/pm_gpu.c)" = bad ] || { echo "fliplog seed FAIL: the gate check misses a hook with no switch test"; exit 1; }
[ "$(chk 3ds/src/pm_gpu.c)" = ok ] || { echo "fliplog FAIL: pm_gpu.c needs 3 pm_ff hooks, each behind the flipfps switch"; exit 1; }
echo "fliplog gate ok"
