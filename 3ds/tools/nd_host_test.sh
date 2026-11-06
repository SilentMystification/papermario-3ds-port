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
  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/ndc 3ds/tools/nd_core_test.c 3ds/src/nd_core.c 3ds/src/nd_mtx.c 3ds/src/nd_state.c
  /tmp/ndc
  gcc -m32 -w -I 3ds/include -c -o /tmp/ndcomb.o 3ds/src/pm_comb.c
  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/ndt 3ds/tools/nd_tmpl_test.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o -lm
  /tmp/ndt 3ds/tests/ndref/ndref_*.bin
  printf "#include <nd.h>\nint pm_tex_slot_find(unsigned k, unsigned f, unsigned s, uint32_t d, NdUnit* u) { (void)k; (void)f; (void)s; (void)d; (void)u; return -1; }\nunsigned pm_tex_slot_key(int i) { (void)i; return 0; }
unsigned pm_tex_slot_gen(int i) { (void)i; return 0; }\nint pm_tex_slot_unit(int i, NdUnit* u) { (void)i; (void)u; return 0; }\n" > /tmp/ndtexs.c
  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/ndx 3ds/tools/nd_tex_test.c 3ds/src/nd_tex.c /tmp/ndtexs.c
  /tmp/ndx 3ds/tests/ndref/ndref_*.bin
  gcc -m32 -Wall -I 3ds/include -o /tmp/ndcm 3ds/tools/nd_cmd_test.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o -lm
  /tmp/ndcm 3ds/tests/ndref/ndref_*.bin
  gcc -m32 -Wall -Wextra -I 3ds/include -o /tmp/nde 3ds/tools/nd_emit_test.c 3ds/src/nd_emit.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o -lm
  /tmp/nde 3ds/tests/ndref/ndref_*.bin
  gcc -m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I 3ds/include -I 3ds/stub_include -I 3ds/tools -o /tmp/ndg 3ds/tools/nd_gbi_test.c 3ds/tools/nd_gbi_ref.c 3ds/src/nd_state.c 2>&1 | grep -E "error" && exit 1
  /tmp/ndg
  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/ndl 3ds/tools/nd_dlc_test.c 3ds/src/nd_dlc.c
  /tmp/ndl assets/us/mapfs/geom/mac_01_shape.bin assets/us/mapfs/geom/mac_02_shape.bin
  printf "#include <nd.h>
#include <string.h>
void nd_comb_decide(const NdCombIn* in, NdCombOut* out) { (void)in; memset(out, 0, sizeof *out); }
" > /tmp/ndcombs.c
  gcc -m32 -Wall -Wextra -Werror $FP -I 3ds/include -o /tmp/ndgv 3ds/tools/nd_gv_test.c 3ds/src/nd_model.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o /tmp/ndcombs.c -lm
  /tmp/ndgv
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -D_MIPS_SZLONG=32 -I include -I 3ds/include -I 3ds/stub_include -o /tmp/ndmd 3ds/tools/nd_model_test.c 3ds/src/nd_model.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o /tmp/ndcombs.c -lm
  /tmp/ndmd
  /tmp/ndseam assets/us/mapfs/geom/mac_01_shape.bin assets/us/mapfs/geom/mac_02_shape.bin 3ds/tests/ndref/ndref_*.bin
' || fail=1
[ "$fail" = 0 ] && echo "ND HOST TESTS PASS" || echo "ND HOST TESTS FAIL"
exit $fail
