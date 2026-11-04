#!/bin/sh
# nd_host_test.sh: native draw host tests (plan_native_system 9.4, gate G1). Runs the 32-bit tests in docker pm3ds-host:1
# (Dockerfile docker/Dockerfile.host). Exit 1 on any fail. A missing nd_core.c is a fail, so the row stays red until S1-2.
cd "$(dirname "$0")/../.." || exit 2
ROOT=$(pwd -W 2>/dev/null || pwd)
fail=0
if [ ! -f 3ds/src/nd_core.c ]; then echo "FAIL nd_core missing (3ds/src/nd_core.c, row S1-2)"; fail=1; fi
MSYS_NO_PATHCONV=1 docker run --rm -v "$ROOT:/w" -w /w pm3ds-host:1 sh -c '
  set -e
  gcc -m32 -Wall -Wextra -o /tmp/ndd 3ds/tools/nd_host_decode_test.c 2>&1 | grep -v "^$" || true
  /tmp/ndd
  gcc -m32 -Wall -c -o /tmp/ndst.o 3ds/tools/nd_host_stubs.c
  echo "stubs compile"
  python3 3ds/tools/nd_stream_cmp.py --selftest
  FP="-msse2 -mfpmath=sse -ffp-contract=off"
  gcc -m32 -Wall -Wextra -Werror $FP -I 3ds/include -o /tmp/ndseam 3ds/tools/nd_seam_test.c 3ds/src/nd_mtx.c
  gcc -m32 -Wall -Wextra -Werror $FP -I 3ds/include -o /tmp/ndm 3ds/tools/nd_mtx_test.c 3ds/src/nd_mtx.c
  /tmp/ndm
  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/ndc 3ds/tools/nd_core_test.c 3ds/src/nd_core.c 3ds/src/nd_mtx.c
  /tmp/ndc
  gcc -m32 -w -I 3ds/include -c -o /tmp/ndcomb.o 3ds/src/pm_comb.c
  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/ndt 3ds/tools/nd_tmpl_test.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o -lm
  /tmp/ndt 3ds/tests/ndref/ndref_*.bin
  /tmp/ndseam assets/us/mapfs/geom/mac_01_shape.bin assets/us/mapfs/geom/mac_02_shape.bin 3ds/tests/ndref/ndref_*.bin
' || fail=1
[ "$fail" = 0 ] && echo "ND HOST TESTS PASS" || echo "ND HOST TESTS FAIL"
exit $fail
