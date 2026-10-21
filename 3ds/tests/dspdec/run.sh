#!/bin/sh
# PT22 host test. Run from the repo root. Plain C path of pm_rsp_audio.c (no SSE4.1), the path the 3DS uses.
set -e
cd "$(dirname "$0")/../../.."
D=3ds/tests/dspdec; O=${TMPDIR:-/tmp}/dspdec_test; mkdir -p $O
INC="-D_LANGUAGE_C -I3ds/include -I3ds/stub_include -I include"
gcc -O2 -w -fsigned-char -fno-strict-aliasing -fwrapv $INC -c 3ds/src/pm_rsp_audio.c -o $O/rsp.o
gcc -O2 -w -fsigned-char -fwrapv $INC -c $D/test_dspdec.c -o $O/t.o
gcc $O/t.o $O/rsp.o -o $O/t
$O/t
