#!/bin/sh
# HOSTCORE1: draw record text format (nd_drec.h / nd_drec.c) against the expected text.
gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/drec 3ds/tools/nd_drec_test.c 3ds/tools/nd_drec.c && /tmp/drec
