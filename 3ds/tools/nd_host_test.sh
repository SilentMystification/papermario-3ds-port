#!/bin/sh
# nd_host_test.sh: native draw host tests (plan_native_system 9.4, gate G1). Runs the 32-bit tests in docker pm3ds-host:1
# (Dockerfile docker/Dockerfile.host). Exit 1 on any fail. A missing nd_core.c is a fail, so the row stays red until S1-2.
cd "$(dirname "$0")/../.." || exit 2
t0=$(date +%s); stage() { now=$(date +%s); echo "STAGE $1 $((now - t0)) s" >&2; t0=$now; }
# HOSTSLOT: the docker runs below take a build slot (3ds/tools/build_slot.sh, BUILDLOCK) and are limited to 4 GB each.
. 3ds/tools/build_slot.sh
build_slot_take
trap 'build_slot_release' EXIT
trap 'build_slot_release; exit 130' INT TERM
ROOT=$(pwd -W 2>/dev/null || pwd)
# WTASSETS: assets/us and ver/us/build are git-ignored. A worktree without them reads them from the main worktree (read
# only, mounted at /ar). The container cwd /work links the worktree folders and those two; nothing is copied.
AR=$ROOT
if [ ! -d assets/us ] || [ ! -d ver/us/build ]; then AR=$(cd "$(git rev-parse --git-common-dir)/.." && pwd -W 2>/dev/null || pwd); fi
fail=0
if [ ! -f 3ds/src/nd_core.c ]; then echo "FAIL nd_core missing (3ds/src/nd_core.c, row S1-2)"; fail=1; fi
# LINRACE: every linear heap call goes through pm_linear_* (3ds/include/pm_linear.h, one lock). A direct call fails the test.
direct=$(grep -nE '(^|[^_A-Za-z0-9])linear(Alloc|MemAlign|Free|Realloc)[[:space:]]*\(' 3ds/src/*.c 3ds/src/*.h 3ds/include/*.h 2>/dev/null)
if [ -n "$direct" ]; then echo "FAIL direct linear call (use pm_linear_*):"; echo "$direct"; fail=1; fi
MSYS_NO_PATHCONV=1 docker run --rm --memory "${PM_BUILD_MEM:-4g}" -v "$ROOT:/w" -v "$AR/assets:/ar/assets:ro" -v "$AR/ver/us/build:/ar/ver/us/build:ro" -w /work pm3ds-host:1 sh -c '
  set -e
  t0=$(date +%s); stage() { now=$(date +%s); echo "STAGE $1 $((now - t0)) s" >&2; t0=$now; }
  mkdir -p /work/ver/us
  for d in 3ds include src; do ln -s "/w/$d" "/work/$d"; done
  ln -s /ar/assets /work/assets
  ln -s /ar/ver/us/build /work/ver/us/build
  gcc -m32 -Wall -Wextra -o /tmp/ndd 3ds/tools/nd_host_decode_test.c 2>&1 | grep -v "^$" || true
  stage L26
  /tmp/ndd
  stage L27
  gcc -m32 -Wall -c -o /tmp/ndst.o 3ds/tools/nd_host_stubs.c
  stage L28
  echo "stubs compile"
  python3 3ds/tools/nd_stream_cmp.py --selftest
  stage L30
  FP="-msse2 -mfpmath=sse -ffp-contract=off"
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndseam 3ds/tools/nd_seam_test.c 3ds/src/nd_mtx.c 3ds/src/nd_seam.c 3ds/src/nd_dlc.c -lm
  stage L32
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndm 3ds/tools/nd_mtx_test.c 3ds/src/nd_mtx.c -lm
  stage L33
  /tmp/ndm
  stage L34
  gcc -m32 -w -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -c -o /tmp/ndcomb.o 3ds/src/pm_comb.c
  stage L35
  gcc -m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndt 3ds/tools/nd_tmpl_test.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o -lm
  stage L36
  /tmp/ndt 3ds/tests/ndref/ndref_*.bin
  stage L37
  printf "#include <nd.h>\nint pm_tex_slot_find(unsigned k, unsigned f, unsigned s, uint32_t d, const void* img, const void* tlut, NdUnit* u) { (void)k; (void)f; (void)s; (void)d; (void)img; (void)tlut; (void)u; return -1; }
int pm_tex_slot_clean(int i, const void* img, const void* tlut) { (void)i; (void)img; (void)tlut; return 0; }
void pm_tex_stale_chk(int i, const void* img, const void* pal, unsigned key) { (void)i; (void)img; (void)pal; (void)key; }\nunsigned pm_tex_slot_key(int i) { (void)i; return 0; }
unsigned pm_tex_slot_gen(int i) { (void)i; return 0; }\nint pm_tex_slot_unit(int i, NdUnit* u) { (void)i; (void)u; return 0; }\nvoid pm_log_prof_line(const char* fmt, ...) { (void)fmt; }\n" > /tmp/ndtexs.c
  gcc -m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndx 3ds/tools/nd_tex_test.c 3ds/src/nd_tex.c /tmp/ndtexs.c
  stage L42
  /tmp/ndx 3ds/tests/ndref/ndref_*.bin
  stage L43
  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/idxt 3ds/tools/pm_tex_idx_test.c
  stage L44
  /tmp/idxt
  stage L45
  gcc -m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndcm 3ds/tools/nd_cmd_test.c 3ds/src/nd_op.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o -lm
  stage L46
  /tmp/ndcm 3ds/tests/ndref/ndref_*.bin
  stage L47
  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/ndapi 3ds/tools/nd_api_test.c 3ds/src/nd_api.c
  stage L48
  /tmp/ndapi
  stage L49
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/nddir 3ds/tools/nd_direct_test.c 3ds/src/nd_api.c -lm
  stage L50
  /tmp/nddir
  stage L51
  gcc -m32 -Wall -Wextra -Werror -DND_SCOMP_HOST -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndsc 3ds/tools/nd_scomp_test.c 3ds/src/nd_scomp.c; /tmp/ndsc
  stage L52
  gcc -m32 -Wall -Wextra -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/nde 3ds/tools/nd_emit_test.c 3ds/src/nd_emit.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o -lm
  stage L53
  /tmp/nde 3ds/tests/ndref/ndref_*.bin
  stage L54
  gcc -m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -I 3ds/stub_include -I 3ds/tools -o /tmp/ndg 3ds/tools/nd_gbi_test.c 3ds/tools/nd_gbi_ref.c 3ds/src/nd_state.c 2>&1 | grep -E "error" && exit 1
  stage L55
  /tmp/ndg
  stage L56
  gcc -m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndl 3ds/tools/nd_dlc_test.c 3ds/src/nd_dlc.c
  stage L57
  /tmp/ndl assets/us/mapfs/geom/mac_01_shape.bin assets/us/mapfs/geom/mac_02_shape.bin
  stage L58
  printf "void pm_log_d(const char* fmt, ...) { (void)fmt; }\nvoid pm_log_w(const char* fmt, ...) { (void)fmt; }\nvoid pm_log_prof_line(const char* fmt, ...) { (void)fmt; }\nunsigned pm_frame_count(void) { return 0; }\nunsigned nd_r13_nohit;\nint pm_texf_centre; int pm_tex_filt_linear(void) { return 0; }\nstatic unsigned char gs_dummy[1024];\nvoid* gGameStatusPtr = gs_dummy;\n" > /tmp/ndlogs.c
  printf "#include <nd.h>
#include <string.h>
void nd_comb_decide(const NdCombIn* in, NdCombOut* out) { (void)in; memset(out, 0, sizeof *out); }
int pm_gpu_noise_ok(void) { return 0; }
void pm_gpu_noise_draw(NdUnit* u, float* nzo) { (void)u; (void)nzo; }
void* nd_ntab_find(const void* list) { (void)list; return 0; } /* GKEY2: rec_inline (nd_rec.c) links; the unit tests have no device entries (nd_gtab.c needs the generated tables) */
__attribute__((weak)) void nd_nr_run(const uint32_t* q) { (void)q; } /* NSTREAM2: nd_r11_walk (nd_model.c) names the native record runner; weak: nd_geo_test.c and the other unit doubles define their own */
" > /tmp/ndcombs.c
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndgv 3ds/tools/nd_gv_test.c 3ds/src/nd_model.c 3ds/src/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o /tmp/ndcombs.c /tmp/ndlogs.c -lm
  stage L68
  /tmp/ndgv
  stage L69
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -D_MIPS_SZLONG=32 -I include -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -I 3ds/stub_include -o /tmp/ndmd 3ds/tools/nd_model_test.c 3ds/src/nd_model.c 3ds/src/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o /tmp/ndcombs.c /tmp/ndlogs.c -lm
  stage L70
  /tmp/ndmd
  stage L71
  # S7CONV: the gfx table tool and the host compare of its tables (nd_gtab_test.c: old list reader against the tables)
  sh 3ds/tools/gen_gfx_tab.sh /tmp/gconv/nd_gfx_tab.c /tmp/gconv
  stage L73
  gcc -m32 -w -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -DVERSION=us -D_MIPS_SZLONG=32 -Dnullptr=NULL -DND_HOST_GFX -I 3ds/include -I 3ds/stub_include -I include -I src -I assets/us -I ver/us/build/include -c -o /tmp/gconv/nd_gfx_tab.o /tmp/gconv/nd_gfx_tab.c
  stage L74
  gcc -m32 -Wall -Wextra -Werror -DND_SCOMP_HOST -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndgt 3ds/tools/nd_gtab_test.c 3ds/src/nd_gtab.c 3ds/src/nd_scomp.c /tmp/gconv/nd_gfx_tab.o /tmp/gconv/gconv_in.o -Wl,--gc-sections
  stage L75
  /tmp/ndgt
  stage L76
  /tmp/ndseam assets/us/mapfs/geom/mac_01_shape.bin assets/us/mapfs/geom/mac_02_shape.bin 3ds/tests/ndref/ndref_*.bin -- assets/us/mapfs/geom/mac_01_shape.bin assets/us/mapfs/geom/mac_00_shape.bin assets/us/mapfs/geom/kkj_14_shape.bin assets/us/mapfs/geom/iwa_10_shape.bin assets/us/mapfs/geom/kmr_20_shape.bin assets/us/mapfs/geom/nok_11_shape.bin
  stage L77
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -c -o /tmp/ndtexwin.o 3ds/src/nd_texwin.c  # S7TEX1: compile check of the moved tile window (nd_texwin.c)
  stage L78
  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/kcc 3ds/tools/kccmd_test.c
  stage L79
  /tmp/kcc
  stage L80
  for t in 3ds/tests/host.d/*.sh; do [ -f "$t" ] || continue; echo "host.d: $t"; sh "$t"; done  # HOSTDIR: per-row host tests
  stage L81
' || fail=1
stage container-end
# CRASHALL: every port thread is created by pm_thread_create (3ds/src/pm_thread.c), so each one has the crash handler. A direct
# threadCreate outside that helper fails the test.
rawth=$(grep -nE '(^|[^_A-Za-z0-9])threadCreate[[:space:]]*\(' 3ds/src/*.c 2>/dev/null | grep -v '^3ds/src/pm_thread.c:')
if [ -n "$rawth" ]; then echo "FAIL direct threadCreate (use pm_thread_create):"; echo "$rawth"; fail=1; fi
# CRASHALL part 2: the crash page renders its lines into the RGBA16 buffer (3ds/tools/crash_page_test.c).
MSYS_NO_PATHCONV=1 docker run --rm --memory "${PM_BUILD_MEM:-4g}" -v "$ROOT:/w" -w /w pm3ds-host:1 sh -c 'gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/cp 3ds/tools/crash_page_test.c 3ds/src/pm_crash_page.c && /tmp/cp' || fail=1
stage crash-page
[ "$fail" = 0 ] && echo "ND HOST TESTS PASS" || echo "ND HOST TESTS FAIL"
exit $fail
