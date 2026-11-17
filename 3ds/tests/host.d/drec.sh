#!/bin/sh
# HOSTCORE1: draw record text format (nd_drec.h / nd_drec.c) against the expected text.
gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/drec 3ds/tools/nd_drec_test.c 3ds/src/nd_drec.c && /tmp/drec || exit 1
gcc -m32 -Wall -Wextra -Werror -I 3ds/include -DND_DREC_SEED_FAULT -o /tmp/drec_seed 3ds/tools/nd_drec_test.c 3ds/src/nd_drec.c
if /tmp/drec_seed > /dev/null; then echo "drec FAIL: the seeded fault passed"; exit 1; fi; echo "drec seed ok"
