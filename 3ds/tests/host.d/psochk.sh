#!/bin/sh
# NPSO4: nd_psochk_cmp (nd_psochk.c) on two descriptions, the walker compare. Prints the test line; exit 1 on a fail.
gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/psochk 3ds/tools/nd_psochk_test.c 3ds/src/nd_psochk.c && /tmp/psochk
