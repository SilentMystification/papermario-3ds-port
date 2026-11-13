#!/bin/sh
# CONV2B: host test of the N64 vertex slot resolver (3ds/tools/nd_vslot.c). Prints "vslot ok 6/6" or "vslot FAIL <case>".
gcc -m32 -Wall -Wextra -Werror -I 3ds/tools -o /tmp/vslot 3ds/tools/nd_vslot_test.c 3ds/tools/nd_vslot.c && /tmp/vslot
