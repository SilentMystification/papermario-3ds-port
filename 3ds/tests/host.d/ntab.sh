#!/bin/sh
# CONV2A: native table form v2 (NdNOp, NdNTab in nd_gtab.h) host test. Prints one line.
gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/ntab 3ds/tools/nd_ntab_test.c && /tmp/ntab
