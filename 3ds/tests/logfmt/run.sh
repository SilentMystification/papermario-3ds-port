#!/bin/sh
# LOG2 host test: generate the cases, compile pm_logfmt.h with the host gcc (-m32: 32-bit long and pointers, as the 3DS), run.
set -e
cd "$(dirname "$0")/../../.."
py -3 3ds/tests/logfmt/gen_logfmt.py
D="$(pwd -W 2>/dev/null || pwd)"
MSYS_NO_PATHCONV=1 docker run --rm -v "$D/3ds:/w:ro" pm3ds-host:1 sh -c 'gcc -m32 -O2 -Wall -Wno-format-nonliteral -Wno-format -I/w/include /w/tests/logfmt/logfmt_test.c -o /tmp/logfmt_test && /tmp/logfmt_test'
