#!/bin/sh
# NPSO1: NdPsoDesc layout and ND_PSO_DESC compile and run on the host.
gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/psodesc 3ds/tools/nd_psodesc_test.c && /tmp/psodesc
