#!/bin/sh
# nd_host_test.sh: native draw host tests (plan_native_system 9.4, gate G1). Runs the 32-bit tests in docker pm3ds-host:1
# (Dockerfile docker/Dockerfile.host). Exit 1 on any fail. A missing nd_core.c is a fail, so the row stays red until S1-2.
# CACHE (BENCHSPEED): the gfx table (gen_gfx_tab.sh) is kept in build3ds/out/gfxcache/<key>, and the output of a passing run in
# build3ds/out/hostcache/<key>. Both keys hash the content of every input file (sha1 per file), the docker image id and this script.
# A hit checks the stored files again (sha1) before use; a mismatch deletes the entry and rebuilds. Entries are written under a
# temp name and renamed. Stage timers: STAGE lines on stderr (name, seconds).
# VOLHOST: the keys are content ids (3ds/tools/src_sync.sh: in_manifest of HOST_IN, asset_manifest AHASH). The containers get
# one tar of the HOST_IN files (the key list is the tar list, so a test that reads a file outside the key fails) and the
# shared asset volume pm3ds-assets-<AHASH> (checked against its out.sha1 first: exit 7 removes it). Only the crash page run
# (2 files, 1 s FACT) keeps the bind mount; no other read crosses WSL2 9p. Self-test: 3ds/tests/volhost/run.sh.
# HOSTTMP: the fixed /tmp paths of the units run inside the docker container (pm3ds-host:1), and each docker run has its own
# /tmp (FACT 2026-11-18: a second run does not see the first run's /tmp file). Git Bash /tmp does not follow TMP (FACT: it is
# the Windows temp folder), so no host-side step may write a fixed /tmp path. Parallel runs (PM_LANE) do not share /tmp.
cd "$(dirname "$0")/../.." || exit 2
# HSRUN: each run has its own out folders (hs_run_, host_run_, hostkey_ and $RUN), so two runs in one worktree do not share them.
# Folders of older runs (more than 1 day) are removed at the start.
RUN=$(printf '%s' "${PM_LANE:-${PM_ROW:-run}}" | tr -c 'A-Za-z0-9_' _)_$$
find build3ds/out -maxdepth 1 \( -name 'hs_run_*' -o -name 'hostkey_*' -o -name 'host_run_*' \) -mtime +1 -exec rm -rf {} + 2>/dev/null
t0=$(date +%s); stage() { now=$(date +%s); echo "STAGE $1 $((now - t0)) s" >&2; t0=$now; }
ROOT=$(pwd -W 2>/dev/null || pwd)
# WTASSETS: assets/us and ver/us/build are git-ignored. A worktree without them reads them from the main worktree (read
# only). VOLHOST: AR is the asset base of asset_manifest; the containers read the assets from the shared asset volume.
AR=$ROOT
if [ ! -d assets/us ] || [ ! -d ver/us/build ]; then AR=$(cd "$(git rev-parse --git-common-dir)/.." && pwd -W 2>/dev/null || pwd); fi
fail=0
if [ ! -f 3ds/src/nd_core.c ]; then echo "FAIL nd_core missing (3ds/src/nd_core.c, row S1-2)"; fail=1; fi
# LINRACE: every linear heap call goes through pm_linear_* (3ds/include/pm_linear.h, one lock). A direct call fails the test.
direct=$(grep -nE '(^|[^_A-Za-z0-9])linear(Alloc|MemAlign|Free|Realloc)[[:space:]]*\(' 3ds/src/*.c 3ds/src/*.h 3ds/include/*.h 2>/dev/null)
if [ -n "$direct" ]; then echo "FAIL direct linear call (use pm_linear_*):"; echo "$direct"; fail=1; fi
# CRASHALL: every port thread is created by pm_thread_create (3ds/src/pm_thread.c), so each one has the crash handler. A direct
# threadCreate outside that helper fails the test.
rawth=$(grep -nE '(^|[^_A-Za-z0-9])threadCreate[[:space:]]*\(' 3ds/src/*.c 2>/dev/null | grep -v '^3ds/src/pm_thread.c:')
if [ -n "$rawth" ]; then echo "FAIL direct threadCreate (use pm_thread_create):"; echo "$rawth"; fail=1; fi
stage checks
# outs_hash <entry dir>: content hash of every stored file except key.txt and outs.sha1.
outs_hash() { (cd "$1" && find . -type f ! -name key.txt ! -name outs.sha1 | LC_ALL=C sort | xargs -r sha1sum) | sha1sum | cut -d' ' -f1; }
IMG_ID=$(docker image inspect -f '{{.Id}}' pm3ds-host:1 2>/dev/null || echo none)
SELF=$(sha1sum 3ds/tools/nd_host_test.sh | cut -d' ' -f1)
# VOLHOST: HOST_IN is every file a host test reads (gtin.sh reads 3ds/build.sh, captool.sh 3ds/push_3ds.sh, maplag.sh
# 3ds/CMakeLists.txt). It is the key list and the tar list. The asset files are AHASH (asset_manifest).
HOST_IN="3ds/tools 3ds/src 3ds/include 3ds/stub_include 3ds/tests 3ds/build.sh 3ds/push_3ds.sh 3ds/CMakeLists.txt include src docker/Dockerfile.build docker/Dockerfile.host"
HOST_AV='mkdir -p /work/ver/us; tar -xf - -C /work; (cd /a/c && sha1sum -c --status out.sha1) || { echo "ASSET VOLUME CHECK FAILED" >&2; exit 7; }; ln -s /a/c/assets /work/assets; ln -s /a/c/ver/us/build /work/ver/us/build'
OUT=build3ds/out/hostkey_$RUN; mkdir -p $OUT
ASSET_BASE=$AR; ASSET_DIRS="assets/us ver/us/build"; IMG=pm3ds-host:1
MAIN_ROOT=$(cd "$(git rev-parse --git-common-dir)/.." && pwd) # BUILDO: the asset stat cache (src_sync.sh asset_manifest)
. 3ds/tools/src_sync.sh
GC_EXCL=""; . 3ds/tools/readkey.sh # GENKEY: every id comes from host_in.txt (the host test writes no generator output)
asset_manifest
# HOSTSEL: the build outputs under HOST_IN stay out of the key and the tar. A parallel build (rowloop.sh) writes them while this
# test lists its files, and pm_build.h changes on every lane build. No unit reads them (FACT: no hostsel reads.txt names one);
# a unit that needs one fails on the missing file, it never passes on a stale one.
HOST_GEN='^3ds/(src/(nd_gfx_tab|nd_ntab_descs|nd_pso_tab|nd_imgfx_tab|map_restore_table)\.c|include/(pm_build|texkey_patch)\.h)	'
in_manifest $HOST_IN | grep -v -E "$HOST_GEN" > $OUT/host_in.txt
[ -s $OUT/host_in.txt ] || { echo "FAIL host key: no files in HOST_IN"; exit 1; }
# GENKEY (3ds/tools/readkey.sh): the gfx table key is K0 (image, this script, assets, trace tool, the path list of HOST_IN, the
# tar of the step) plus the content id of every file gen_gfx_tab.sh read (traced). An edit to a file it does not read hits.
cut -f1 $OUT/host_in.txt > $OUT/host_files.txt
GFX_K0="gfx v3
img $IMG_ID
self $SELF
assets $AHASH
rk $(rk_id)
paths $(rk_pathsum $OUT/host_files.txt)"
TEST_KEY=$( { echo "host v3"; cat $OUT/host_in.txt; echo "assets $AHASH"; echo "$IMG_ID $SELF gfx v3 $(rk_id)"; } | sha1sum | cut -d' ' -f1)
stage keys
# SELFTEST: one changed byte of one input changes its key line (in_manifest of an untracked file; a stale entry can never match).
st=build3ds/out/cache_selftest; rm -rf $st; mkdir -p $st; cp 3ds/tools/gen_gfx_tab.sh $st/gen_gfx_tab.sh
k1=$(in_manifest $st); printf 'X' | dd of=$st/gen_gfx_tab.sh bs=1 seek=0 conv=notrunc 2>/dev/null
k2=$(in_manifest $st); rm -rf $st
[ -n "$k1" ] || { echo "FAIL cache selftest: no key line for $st"; fail=1; }
if [ "$k1" = "$k2" ]; then echo "FAIL cache selftest: a changed byte kept the key"; fail=1; else echo "cache selftest ok (a changed byte misses)"; fi
# Whole-run cache: a passing run with the same TEST_KEY replays its output. The stored output is checked by its sha1 first.
TC=build3ds/out/hostcache/$TEST_KEY
if [ "${PM_CACHE:-on}" != off ] && [ "${PM_HOSTSEL:-on}" != all ] && [ -f $TC/rc.txt ] && [ -f $TC/out.txt ] && [ "$(cat $TC/out.sha1 2>/dev/null)" = "$(sha1sum $TC/out.txt | cut -d' ' -f1)" ]; then
    echo "hostcache: hit ${TEST_KEY}"
    cat $TC/out.txt; exit "$(cat $TC/rc.txt)"
fi
rm -rf $TC
# Host slot: the docker runs below take one (3ds/tools/build_slot.sh, BUILDLOCK) and are limited to 4 GB each. BUILDO: the host
# tests have their own pool (build3ds/hostslots, PM_HOST_SLOTS, default 4), so a row loop does not hold 2 build slots.
BUILDSLOT_DIR=${BUILDSLOT_DIR:-$MAIN_ROOT/build3ds/hostslots}; PM_BUILD_SLOTS=${PM_HOST_SLOTS:-4}
. 3ds/tools/build_slot.sh
build_slot_take
# PM_CACHE=off: asset_manifest named a new cold volume; it goes at the end.
cold_rm() { [ -z "$HSR" ] || rm -rf "$HSR"; if [ "${PM_CACHE:-on}" = off ]; then docker volume rm $AVOL >/dev/null 2>&1 || true; fi; }
trap 'build_slot_release; cold_rm' EXIT
trap 'build_slot_release; cold_rm; exit 130' INT TERM
stage slot-wait
TMPLOG=build3ds/out/host_run_$RUN.txt
# VOLHOST: the tar of the key files, and the asset volume of AHASH (made once when it is missing).
tar -cf $OUT/host_in.tar -T $OUT/host_files.txt || { echo "FAIL host tar of HOST_IN"; exit 1; }
case "$(MSYS_NO_PATHCONV=1 docker run --rm -v $AVOL:/a:ro -e K=$AHASH $IMG sh -c '[ "$(cat /a/c/key.txt 2>/dev/null)" = "$K" ] && echo assets-ok; true')" in
    *assets-ok*) ;;
    *) asset_vol_make ;;
esac
# av_fault <rc>: exit 7 of a container (the asset volume failed its check) removes the volume.
av_fault() { if [ "$1" = 7 ]; then docker volume rm $AVOL >/dev/null 2>&1 || true; echo "HOST FAULT: the asset volume $AVOL failed its content check and was removed. Run again (the next run makes it new)."; fi; }
stage tar-assets
# gfx table entry (GENKEY): <main>/build3ds/hostgfx/<16 hex of K0>/<16 hex of the full key>/. A hit needs a stored read list whose
# full key equals key.txt and an outs hash that matches; else the step runs traced under a temp name, then one rename.
# HOSTSEL: the gfx entries and the unit results live in the main folder, shared by every worktree (content keys).
MAINR=$(cd "$(git rev-parse --git-common-dir)/.." && pwd)
wpath() { (cd "$1" && { pwd -W 2>/dev/null || pwd; }); }
GC=$MAINR/build3ds/hostgfx; GK0=$GC/$(printf '%s' "$GFX_K0" | sha1sum | cut -c1-16)
gfx_ok() { [ -f "$1/key.txt" ] && [ "$(cat "$1/key.txt")" = "$2" ] && [ -f "$1/outs.sha1" ] && [ "$(outs_hash "$1")" = "$(cat "$1/outs.sha1")" ]; }
GE=
if [ "${PM_CACHE:-on}" != off ]; then # GFXINC: PM_CACHE=off (cold gate) never hits
    for c in "$GK0"/*/; do
        case $c in *.tmp.*) continue ;; esac
        [ -f "$c/reads.txt" ] || continue
        fk=$(rk_full "$GFX_K0" $OUT/host_in.txt "$c/reads.txt")
        d=$GK0/$(printf '%s' "$fk" | sha1sum | cut -c1-16)
        if gfx_ok "$d" "$fk"; then GE=$d; break; fi
    done
fi
if [ -n "$GE" ]; then echo "gfxcache: hit"; else
    echo "gfxcache: miss, build"
    mkdir -p "$GK0"; TMPE=$GK0/.tmp.$$; rm -rf "$TMPE"; mkdir -p "$TMPE/out"
    # GFXINC: the object cache volume of this host image (3ds/tools/gconv_objcache.py), shared by every worktree and build.sh.
    grc=0
    MSYS_NO_PATHCONV=1 docker run --rm -i --memory "${PM_BUILD_MEM:-4g}" -v $AVOL:/a:ro -v "$(wpath "$TMPE"):/gcw" \
        -v "pm3ds-gconvobj-$(printf '%s' "$IMG_ID" | cut -c8-23):/gobj" -e GCONV_OBJ=/gobj -e PM_CACHE="${PM_CACHE:-on}" -w /work pm3ds-host:1 sh -c "
      set -e; $HOST_AV; $RK_SO
      $RK_TRACE sh 3ds/tools/gen_gfx_tab.sh /gcw/nd_gfx_tab.c /gcw
      cp /tmp/rt/log /gcw/rk.log" < $OUT/host_in.tar || grc=$?
    [ "$grc" = 0 ] || { av_fault "$grc" >&2; rm -rf "$TMPE"; echo "gen_gfx_tab failed" >&2; exit 1; }
    [ -s "$TMPE/nd_gfx_tab.c" ] || { rm -rf "$TMPE"; echo "gen_gfx_tab failed: no output" >&2; exit 1; }
    ( rk_reads "$TMPE/rk.log" $OUT/host_files.txt "$TMPE/reads.txt" 3ds/tools/gen_gfx_tab.sh 3ds/tools/nd_gfx_conv.c 3ds/include/nd_rmdef.h ) || { rm -rf "$TMPE"; echo "FAIL gfx table read trace"; exit 1; }
    rm -f "$TMPE/rk.log"
    for lg in ntab_inherit.txt ntab_unsup.txt ntab_tables.txt ntab_entries.txt ntab_pairs.txt gset_ok.txt; do cp "$TMPE/$lg" "$TMPE/out/$lg"; done
    fk=$(rk_full "$GFX_K0" $OUT/host_in.txt "$TMPE/reads.txt")
    GE=$GK0/$(printf '%s' "$fk" | sha1sum | cut -c1-16)
    printf '%s' "$fk" > "$TMPE/key.txt"; outs_hash "$TMPE" > "$TMPE/outs.sha1"
    rm -rf "$GE"; mv "$TMPE" "$GE"
    ls -dt "$GK0"/*/ 2>/dev/null | grep -v '\.tmp\.' | tail -n +5 | xargs -r rm -rf # 4 read states per K0
fi
cp -f "$GE"/out/* build3ds/out/
# Keep the 16 newest K0 folders (all worktrees) (an entry is 13 MB, FACT oracle 2026-11-16). The entry of this run is made newest first.
touch "$GE" "$GK0"; ls -dt $GC/*/ 2>/dev/null | tail -n +17 | xargs -r rm -rf
stage gfx-table
# HOSTSEL (3ds/tools/hostsel.py): one result entry per unit (the inline block, each host.d script), keyed by K0 (below) and the
# content id of every file the unit read (traced, 3ds/tools/readtrace.c). A unit with a hit does not run; its stored output is
# printed. hostsel.py adds the path list of the tar to K0 (new, removed, created files). The command lines and the environment of
# every unit are text of this script (self) or of the image (img). PM_HOSTSEL=all runs every unit and fails a unit that had a hit but fails now (the post-merge check, post_merge.sh 4).
PY=python; command -v py >/dev/null 2>&1 && PY="py -3"
HS=$MAINR/build3ds/hostsel; HSR=build3ds/out/hs_run_$RUN; rm -rf $HSR; mkdir -p $HSR/hd $HSR/rt
printf '%s\n' "hostsel v1" "img $IMG_ID" "self $SELF" "assets $AHASH" "rk $(rk_id)" "gfx $(sha1sum < "$GE/key.txt" | cut -c1-40)" > $HSR/k0.txt
{ echo inline; for t in 3ds/tests/host.d/*.sh; do [ -f "$t" ] && echo "$t"; done; } > $HSR/units.txt
: > $HSR/hits.txt
if [ "${PM_CACHE:-on}" != off ]; then $PY 3ds/tools/hostsel.py lookup $HS $HSR/k0.txt $OUT/host_in.txt $HSR/units.txt $HSR/hits.txt || { echo "FAIL hostsel lookup"; exit 1; }; fi
if [ "${PM_HOSTSEL:-on}" = all ]; then cp $HSR/units.txt $HSR/run.txt; else cut -f1 $HSR/hits.txt | LC_ALL=C sort > $HSR/hit_s.txt; LC_ALL=C sort $HSR/units.txt | LC_ALL=C comm -23 - $HSR/hit_s.txt > $HSR/run.txt; fi
# HSDEP: a host.d script that reads a /tmp file of another unit names it on a line "# HOSTSEL needs: <unit>". A unit that runs
# pulls every unit it needs (they run before it: the needed unit must sort first, else FAIL), and its stored key holds their reads.
grep -H -E '^# HOSTSEL needs: ' 3ds/tests/host.d/*.sh 2>/dev/null | sed -E 's/^([^:]*):# HOSTSEL needs: */\1\t/' > $HSR/needs.txt || true
while IFS="$(printf '\t')" read -r u nd; do case "$nd" in "$u"|"") ;; *) [ "$(printf '%s\n%s\n' "$nd" "$u" | LC_ALL=C sort | head -1)" = "$nd" ] || { echo "FAIL hostsel: $u needs $nd, which sorts after it"; exit 1; } ;; esac; done < $HSR/needs.txt
while :; do
    awk -F'\t' 'FILENAME == ARGV[1] { r[$0] = 1; next } ($1 in r) && !($2 in r) { print $2 }' $HSR/run.txt $HSR/needs.txt | LC_ALL=C sort -u > $HSR/pull.txt
    [ -s $HSR/pull.txt ] || break
    cat $HSR/pull.txt >> $HSR/run.txt; LC_ALL=C sort -u -o $HSR/run.txt $HSR/run.txt
done
HS_INLINE=0; grep -qx inline $HSR/run.txt && HS_INLINE=1
grep -v -x inline $HSR/run.txt > $HSR/run_hd.txt || true
echo "hostsel: hit $(grep -c . $HSR/hits.txt) of $(grep -c . $HSR/units.txt) units, run $(grep -c . $HSR/run.txt)$([ "${PM_HOSTSEL:-on}" = all ] && echo ' (PM_HOSTSEL=all: every unit runs)')"
# BATCH: host.d scripts run in parallel in batches; two scripts share a batch only when they name no common /tmp or build3ds/out path.
if [ -s $HSR/run_hd.txt ]; then grep -o -H -E '(/tmp|build3ds/out)/[A-Za-z0-9_./-]*' $(cat $HSR/run_hd.txt) /dev/null > $HSR/tok.txt || true; else : > $HSR/tok.txt; fi
awk 'FILENAME == ARGV[1] { o[++n] = $0; next } { i = index($0, ":"); f = substr($0, 1, i - 1); k = substr($0, i + 1); if (!((f, k) in seen)) { seen[f, k] = 1; tk[f] = tk[f] " " k } }
END { for (i = 1; i <= n; i++) print o[i] "|" tk[o[i]] }' $HSR/run_hd.txt $HSR/tok.txt | awk -F'|' '
{ n = split($2, a, " "); placed = 0
  for (b = 1; b <= nb && !placed; b++) { if (cnt[b] < 12) { ok = 1; for (i = 1; i <= n; i++) if ((b, a[i]) in tok) { ok = 0; break }
      if (ok) { for (i = 1; i <= n; i++) tok[b, a[i]] = 1; cnt[b]++; bl[b] = bl[b] " " $1; placed = 1 } } }
  if (!placed) { nb++; cnt[nb] = 1; for (i = 1; i <= n; i++) tok[nb, a[i]] = 1; bl[nb] = $1 } }
END { for (b = 1; b <= nb; b++) print bl[b] }' > $HSR/batches.txt
stage hostsel
hrc=0
MSYS_NO_PATHCONV=1 docker run --rm -i --memory "${PM_BUILD_MEM:-4g}" -v $AVOL:/a:ro -v "$(wpath "$GE"):/gcache:ro" -v "$ROOT/$HSR:/hso" -e HS_INLINE=$HS_INLINE -w /work pm3ds-host:1 sh -c "set -e; $HOST_AV; $RK_SO"'
  # HSDEP: /tmp/gconv (the gfx entry and nd_gfx_tab.o) is made here, outside every unit, traced to prep.log. A unit that opens a
  # file below /tmp/gconv (logged "@..." by RT_ROOT2) gets the prep reads in its key (hostsel.py store).
  mkdir -p /tmp/gconv; cp -r /gcache/. /tmp/gconv/; rm -rf /tmp/gconv/out; rm -f /tmp/gconv/key.txt /tmp/gconv/outs.sha1
  RT_LOG=/tmp/rt/prep.log RT_ROOT=/work/ LD_PRELOAD=/tmp/rt/\$PLATFORM/rt.so gcc -m32 -w -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -DVERSION=us -D_MIPS_SZLONG=32 -Dnullptr=NULL -DND_HOST_GFX -I 3ds/include -I 3ds/stub_include -I 3ds/tools/gbi_inc -I include -I src -I assets/us -I ver/us/build/include -c -o /tmp/gconv/nd_gfx_tab.o /tmp/gconv/nd_gfx_tab.c
  cp /tmp/rt/prep.log /hso/rt/prep.log
  # HOSTCACHE: /tmp/ndcomb.o, /tmp/ndcombs.c, /tmp/ndlogs.c are the RO inputs of the host.d units (RO=/tmp); made here, on every path.
  gcc -m32 -w -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -c -o /tmp/ndcomb.o 3ds/src/pm_comb.c
  printf "void pm_log_d(const char* fmt, ...) { (void)fmt; }\nvoid pm_log_w(const char* fmt, ...) { (void)fmt; }\nvoid pm_log_prof_line(const char* fmt, ...) { (void)fmt; }\nunsigned pm_frame_count(void) { return 0; }\nunsigned nd_r13_nohit;\nint pm_texf_centre; int pm_tex_filt_linear(void) { return 0; }\nstatic unsigned char gs_dummy[1024];\nvoid* gGameStatusPtr = gs_dummy;\n" > /tmp/ndlogs.c
  printf "#include <nd.h>
#include <string.h>
void nd_comb_dec(const NdCombIn* in, NdCombOut* out) { (void)in; memset(out, 0, sizeof *out); }
int pm_gpu_noise_ok(void) { return 0; }
void pm_gpu_noise_draw(NdUnit* u, float* nzo) { (void)u; (void)nzo; }
void* nd_ntab_find(const void* list) { (void)list; return 0; } /* GKEY2: rec_inline (nd_rec.c) links; the unit tests have no device entries (nd_gtab.c needs the generated tables) */
__attribute__((weak)) int pm_wide; __attribute__((weak)) int is_host(unsigned p) { return p >= 0x00100000u; } __attribute__((weak)) void tex_win_s25(int* a, int* b, int* c, int* d, unsigned e, unsigned f, unsigned g, unsigned h) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h; } /* NST1: nd_ds.c (linked with nd_rec.c, the nd_rec_nr hook) */
__attribute__((weak)) void nd_omh_run(const uint32_t* q) { (void)q; } /* NAT4J: nd_r11_walk (nd_model.c) names the OMH record runner */
__attribute__((weak)) void nd_nr_run(const uint32_t* q) { (void)q; } /* NSTREAM2: nd_r11_walk (nd_model.c) names the native record runner; weak: nd_geo_test.c and the other unit doubles define their own */
__attribute__((weak)) void nd_geo_rec(const uint32_t* q) { (void)q; } __attribute__((weak)) void nd_ctl_rec(const uint32_t* q) { (void)q; } /* NAT5G: nd_r11_walk (nd_model.c) names the geometry and control runners */
" > /tmp/ndcombs.c
  t0=$(date +%s); stage() { now=$(date +%s); echo "STAGE $1 $((now - t0)) s" >&2; t0=$now; }
  # the inline block runs as an async subshell (set -e holds there; in an && list it would not), its status from wait
  if [ "$HS_INLINE" = 1 ]; then r=0; ( set -e; export RT_LOG=/tmp/rt/inline.log RT_ROOT=/work/ RT_ROOT2=/tmp/gconv/ LD_PRELOAD=/tmp/rt/\$PLATFORM/rt.so
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
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndseam 3ds/tools/nd_seam_test.c 3ds/src/nd_mtx.c 3ds/src/nd_seam.c 3ds/src/nd_dlc.c -lm
  stage L32
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndm 3ds/tools/nd_mtx_test.c 3ds/src/nd_mtx.c -lm
  stage L33
  /tmp/ndm
  stage L34
  stage L35
  gcc -m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndt 3ds/tools/nd_tmpl_test.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o -lm
  stage L36
  /tmp/ndt 3ds/tests/ndref/ndref_*.bin
  stage L37
  printf "#include <nd.h>\nint pm_tex_slot_find(unsigned k, unsigned f, unsigned s, uint32_t d, const void* img, const void* tlut, NdUnit* u) { (void)k; (void)f; (void)s; (void)d; (void)img; (void)tlut; (void)u; return -1; }
int pm_tex_slot_clean(int i, const void* img, const void* tlut) { (void)i; (void)img; (void)tlut; return 0; }
void pm_tex_stale_chk(int i, const void* img, const void* pal, unsigned key) { (void)i; (void)img; (void)pal; (void)key; }\nunsigned pm_tex_slot_key(int i) { (void)i; return 0; }
unsigned pm_tex_slot_gen(int i) { (void)i; return 0; }\nint pm_tex_slot_unit(int i, NdUnit* u) { (void)i; (void)u; return 0; }\nvoid pm_log_prof_line(const char* fmt, ...) { (void)fmt; }\n" > /tmp/ndtexs.c
  gcc -m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndx 3ds/tools/nd_tex_test.c 3ds/src/nd_tex.c /tmp/ndtexs.c
  stage L42
  /tmp/ndx 3ds/tests/ndref/ndref_*.bin
  stage L43
  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/idxt 3ds/tools/pm_tex_idx_test.c
  stage L44
  /tmp/idxt
  stage L45
  gcc -m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndcm 3ds/tools/nd_cmd_test.c 3ds/tools/nd_op.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o -lm
  stage L46
  /tmp/ndcm 3ds/tests/ndref/ndref_*.bin
  stage L47
  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/ndapi 3ds/tools/nd_api_test.c 3ds/src/nd_api.c
  stage L48
  /tmp/ndapi
  stage L49
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/nddir 3ds/tools/nd_direct_test.c 3ds/src/nd_api.c 3ds/tools/nd_snap_link_stub.c -lm
  stage L50
  /tmp/nddir
  stage L51
  gcc -m32 -Wall -Wextra -Werror -DND_SCOMP_HOST -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndsc 3ds/tools/nd_scomp_test.c 3ds/src/nd_scomp.c; /tmp/ndsc
  stage L52
  gcc -m32 -Wall -Wextra -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/nde 3ds/tools/nd_emit_test.c 3ds/src/nd_emit.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o -lm
  stage L53
  /tmp/nde 3ds/tests/ndref/ndref_*.bin
  stage L54
  gcc -m32 -Wall -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -I 3ds/stub_include -I 3ds/tools -o /tmp/ndg 3ds/tools/nd_gbi_test.c 3ds/tools/nd_gbi_ref.c 3ds/src/nd_state.c 2>&1 | grep -E "error" && exit 1
  stage L55
  /tmp/ndg
  stage L56
  gcc -m32 -Wall -Wextra -Werror -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndl 3ds/tools/nd_dlc_test.c 3ds/src/nd_dlc.c
  stage L57
  /tmp/ndl assets/us/mapfs/geom/mac_01_shape.bin assets/us/mapfs/geom/mac_02_shape.bin
  stage L58
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndgv 3ds/tools/nd_gv_test.c 3ds/src/nd_model.c 3ds/tools/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o /tmp/ndcombs.c /tmp/ndlogs.c -lm
  stage L68
  /tmp/ndgv
  stage L69
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -I 3ds/stub_include -o /tmp/ndmd 3ds/tools/nd_model_test.c 3ds/src/nd_model.c 3ds/tools/nd_op.c 3ds/src/nd_mtx.c 3ds/src/nd_tmpl.c /tmp/ndcomb.o /tmp/ndcombs.c /tmp/ndlogs.c -lm
  stage L70
  /tmp/ndmd
  stage L71
  # S7CONV: the gfx table tool and the host compare of its tables (nd_gtab_test.c: old list reader against the tables)
  # HSDEP: /tmp/gconv and nd_gfx_tab.o are made before the units (see the start of this container)
  gcc -m32 -Wall -Wextra -Werror -DND_SCOMP_HOST -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -o /tmp/ndgt 3ds/tools/nd_gtab_test.c 3ds/src/nd_gtab.c 3ds/src/nd_scomp.c /tmp/gconv/nd_gfx_tab.o /tmp/gconv/gconv_in.o -Wl,--gc-sections
  stage L75
  /tmp/ndgt
  stage L76
  /tmp/ndseam assets/us/mapfs/geom/mac_01_shape.bin assets/us/mapfs/geom/mac_02_shape.bin 3ds/tests/ndref/ndref_*.bin -- assets/us/mapfs/geom/mac_01_shape.bin assets/us/mapfs/geom/mac_00_shape.bin assets/us/mapfs/geom/kkj_14_shape.bin assets/us/mapfs/geom/iwa_10_shape.bin assets/us/mapfs/geom/kmr_20_shape.bin assets/us/mapfs/geom/nok_11_shape.bin
  stage L77
  gcc -m32 -Wall -Wextra -Werror $FP -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I 3ds/tools/gbi_inc -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include -c -o /tmp/ndtexwin.o 3ds/src/nd_texwin.c  # S7TEX1: compile check of the moved tile window (nd_texwin.c)
  stage L78
  gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/kcc 3ds/tools/kccmd_test.c
  stage L79
  /tmp/kcc
  stage L80
  ) > /tmp/inline.out 2>&1 & wait $! || r=$?; echo $r > /hso/inline.rc; cp /tmp/inline.out /hso/inline.out; cp /tmp/rt/inline.log /hso/rt/inline.log 2>/dev/null || true; cat /tmp/inline.out; fi
  hd=/tmp/hostd; rm -rf $hd; mkdir -p $hd
  while read -r batch; do
    for t in $batch; do n=$(echo "$t" | tr / _); ( s=$(date +%s); RT_LOG=/tmp/rt/$n.log RT_ROOT=/work/ RT_ROOT2=/tmp/gconv/ LD_PRELOAD=/tmp/rt/\$PLATFORM/rt.so sh "$t" < /dev/null > "$hd/$n.log" 2>&1; echo $? > "$hd/$n.rc"; echo $(($(date +%s) - s)) > "$hd/$n.s" ) & done
    wait
  done < /hso/batches.txt
  hdfail=0
  while read -r t; do n=$(echo "$t" | tr / _); { echo "host.d: $t"; cat "$hd/$n.log"; echo "HOSTD $t $(cat "$hd/$n.s" 2>/dev/null) s"; } > "$hd/$n.sec"; cat "$hd/$n.sec"; cp "$hd/$n.sec" "$hd/$n.rc" /hso/hd/; cp /tmp/rt/$n.log /hso/rt/ 2>/dev/null || true; [ "$(cat "$hd/$n.rc" 2>/dev/null)" = 0 ] || { echo "host.d FAILED: $t"; hdfail=1; }; done < /hso/run_hd.txt
  [ "$hdfail" = 0 ] && [ "$(cat /hso/inline.rc 2>/dev/null || echo 0)" = 0 ]
  stage L81
' > "$TMPLOG" 2>&1 < $OUT/host_in.tar || hrc=$?
[ "$hrc" = 0 ] || { av_fault "$hrc" >> "$TMPLOG"; fail=1; }
stage container-end
# HOSTSEL: store each passing unit that ran (its trace must name its own script: else the trace is broken and nothing is stored).
: > $HSR/store.txt
if [ "$HS_INLINE" = 1 ] && [ "$(cat $HSR/inline.rc 2>/dev/null)" = 0 ]; then printf 'inline\t%s\t%s\t3ds/tools/nd_host_decode_test.c\t\n' $HSR/rt/inline.log $HSR/inline.out >> $HSR/store.txt; fi
while read -r t; do n=$(echo "$t" | tr / _); nl=$(awk -F'\t' -v u="$t" '$1 == u { printf "%s%s", s, $2; s = "," }' $HSR/needs.txt | sed 's|/|_|g; s|,|.log,'$HSR'/rt/|g'); [ -n "$nl" ] && nl="$HSR/rt/$nl.log"
    [ "$(cat $HSR/hd/$n.rc 2>/dev/null)" = 0 ] && printf '%s\t%s\t%s\t%s\t%s\n' "$t" $HSR/rt/$n.log $HSR/hd/$n.sec "$t" "$nl" >> $HSR/store.txt; done < $HSR/run_hd.txt
if [ "${PM_CACHE:-on}" != off ] && [ -s $HSR/store.txt ]; then $PY 3ds/tools/hostsel.py store $HS $HSR/k0.txt $OUT/host_in.txt $HSR/store.txt $HSR/rt/prep.log >> "$TMPLOG" 2>&1 || { echo "FAIL hostsel store (read trace)" >> "$TMPLOG"; fail=1; }; fi
# the stored output of each hit; PM_HOSTSEL=all: a hit whose unit fails now is a stale entry (it is removed)
while IFS="$(printf '\t')" read -r u d; do
    if [ "${PM_HOSTSEL:-on}" = all ]; then
        n=$(echo "$u" | tr / _); r=$(cat $HSR/hd/$n.rc 2>/dev/null); [ "$u" = inline ] && r=$(cat $HSR/inline.rc 2>/dev/null)
        if [ "$r" = 0 ]; then echo "hostsel cachecheck ok $u" >> "$TMPLOG"; else echo "HOSTSEL CACHECHECK FAIL $u: the entry passed, the unit fails now (entry $d removed)" >> "$TMPLOG"; rm -rf "$d"; fail=1; fi
    else
        { echo "hostsel: hit $u"; cat "$d/out.txt"; } >> "$TMPLOG"
    fi
done < $HSR/hits.txt
stage hostsel-store
# CRASHALL part 2: the crash page renders its lines into the RGBA16 buffer (3ds/tools/crash_page_test.c).
MSYS_NO_PATHCONV=1 docker run --rm --memory "${PM_BUILD_MEM:-4g}" -v "$ROOT:/w" -w /w pm3ds-host:1 sh -c 'gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/cp 3ds/tools/crash_page_test.c 3ds/src/pm_crash_page.c && /tmp/cp' >> "$TMPLOG" 2>&1 || fail=1
stage crash-page
cat "$TMPLOG"
if [ "$fail" = 0 ]; then
    echo "ND HOST TESTS PASS"
    mkdir -p build3ds/out/hostcache; rm -rf "$TC.tmp.$RUN"; mkdir -p "$TC.tmp.$RUN"
    { cat "$TMPLOG"; echo "ND HOST TESTS PASS"; } > "$TC.tmp.$RUN/out.txt"; echo 0 > "$TC.tmp.$RUN/rc.txt"; sha1sum "$TC.tmp.$RUN/out.txt" | cut -d' ' -f1 > "$TC.tmp.$RUN/out.sha1"
    rm -rf "$TC"; mv "$TC.tmp.$RUN" "$TC"
    exit 0
fi
echo "ND HOST TESTS FAIL"
exit 1
