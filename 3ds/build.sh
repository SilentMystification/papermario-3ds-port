#!/bin/sh
# Build the 3DS port inside the devkitpro image with ccache (docker/Dockerfile.build).
# Usage (repo root): sh 3ds/build.sh [extra ninja args]
# Output: build3ds/pm_3ds.3dsx (a lane: build3ds/lane-<PM_LANE>/), also copied to %APPDATA%\Azahar\sdmc\3ds\PaperMario\
# The stages, keys and rules: docs/handoff/build_pipeline.md (BUILDO). Short form:
# 1 key (no docker run, no slot): content ids of the code (in_manifest), the assets (stat-cached ids), the generator outputs
#   (gencache hits) and the shape key. Equal to the lane's build_key.txt and the 3dsx hash checks: "build up to date", exit.
# 2 generators that miss run in parallel (texkey | shapes | gfx_tab then pso_tab), under a build slot.
# 3 one container in the build workspace of the slot (pm3ds-src-ws<k>, pm3ds-build-ws<k>): asset check, romfs from the shape
#   volume, sync of the changed code files (content ids against the slot manifest), configure when needed, ninja, copy-out.
#   The workspace is shared by every lane, so a new lane is incremental. /src and /work stay the paths (bytes stay equal).
# PM_CACHE=off is the cold gate: no cache read or write, no skip, no ccache, new workspace, asset and shape volumes.
# PM_CACHE=check (cachecheck, with PM_KEY_ONLY=1): the generators run cold and compare with their cache entries.
# Hang guard: a watchdog kills this run's container after PM_BUILD_CAP seconds (default 600).
set -e
cd "$(dirname "$0")/.."
TB=$(date +%s)
# WTASSETS: assets/us and ver/us/build are git-ignored ROM-derived inputs, read from the main worktree when this one has none.
MAIN_ROOT=$(cd "$(git rev-parse --git-common-dir)/.." && pwd)
ASSET_BASE=.
if [ ! -d assets/us ] || [ ! -d ver/us/build ]; then ASSET_BASE=$MAIN_ROOT; fi
OUT=build3ds
[ -n "$PM_LANE" ] && OUT=build3ds/lane-$PM_LANE
# HC0: PM_FNCOV=1 is the census lane <L>_fncov (needs PM_LANE): its own output folder, the draw TUs instrumented.
FNV=OFF
if [ "${PM_FNCOV:-0}" = 1 ]; then
    [ -n "$PM_LANE" ] || { echo "PM_FNCOV=1 needs PM_LANE" >&2; exit 1; }
    FNV=ON; OUT=build3ds/lane-${PM_LANE}_fncov
fi
mkdir -p $OUT
CACHE=${PM_CACHE:-on}
ASSET_DIRS="assets/us ver/us/build"
CODE_DIRS="3ds/CMakeLists.txt 3ds/effects.cmake 3ds/battle.cmake 3ds/maps.cmake 3ds/map_syms.lds LaunchIcon.png 3ds/ld 3ds/shaders 3ds/stub_include 3ds/include 3ds/src 3ds/tools/cov.d src include"
# The build key covers the code dirs, the build recipe and this script.
KEY_IN="$CODE_DIRS docker/Dockerfile.build 3ds/build.sh"
# Generator outputs and the build header inside CODE_DIRS: their ids are taken after the generators (gen_ids).
GEN_FILES="3ds/src/nd_gfx_tab.c 3ds/src/nd_ntab_descs.c 3ds/src/nd_pso_tab.c 3ds/include/texkey_patch.h"
. 3ds/tools/gencache.sh
. 3ds/tools/src_sync.sh
. 3ds/tools/readkey.sh
. 3ds/tools/build_slot.sh
stage() { now=$(date +%s); echo "STAGE $1 $((now - TB)) s" >&2; TB=$now; }
if command -v py >/dev/null 2>&1; then PYX="py -3"; else PYX=python3; fi
# BN1: build number V0 + (commit count - C0) (3ds/build_base.txt); suffix -<lane> or -laneB. PM_BUILD_REV: the commit to build.
read V0 C0 <<EOF2
$(tr -d '\015' < 3ds/build_base.txt)
EOF2
REV=${PM_BUILD_REV:-HEAD}
BN=$((V0 + $(git rev-list --count "$REV") - C0))
if [ -n "$PM_LANE" ]; then BN="$BN-$PM_LANE"; elif [ -z "$PM_BUILD_REV" ] && [ "$(git branch --show-current)" = "lane-b" ]; then BN="$BN-laneB"; fi
# AZT9c-1: a lane build carries a time tag (the boot line proves the new ROM); a PM_BUILD_REV build has none (same bytes cold).
if [ -n "$PM_LANE" ] && [ -z "$PM_BUILD_REV" ]; then BN="${BN}_$(date +%H%M%S)$$"; fi

# SLOT: a build slot is taken only for docker work (a generator miss or the build), never for the key or a skip.
SLOT_HELD=0
slot_need() {
    [ "$SLOT_HELD" = 1 ] && return 0
    [ -n "$PM_KEY_ONLY" ] && return 0
    tw=$(date +%s)
    build_slot_take >&2
    trap 'build_slot_release' EXIT
    trap 'build_slot_release; exit 130' INT TERM
    SLOT_HELD=1
    echo "STAGE slot-wait $(( $(date +%s) - tw )) s" >&2
}

# GENSPEED: gen_gfx_tab and gen_pso_tab read a tar of their key folders and the assets from the shared asset volume (checked
# against its out.sha1 first: exit 7). The container sees only the key folders, so a read outside the key fails the step.
GEN_AV='mkdir -p /work/ver/us; tar -xf - -C /work; (cd /a/c && sha1sum -c --status out.sha1) || { echo "ASSET VOLUME CHECK FAILED" >&2; exit 7; }; ln -s /a/c/assets /work/assets; ln -s /a/c/ver/us/build /work/ver/us/build'
# GENTMP: gen_put <new> <dest>: a generator output goes to its source path by one mv, only when its bytes changed.
gen_put() { if cmp -s "$1" "$2"; then rm -f "$1"; else mv -f "$1" "$2"; fi; }
# gen_run <name> <command> <paths file>: runs the command in the host image with the tar of <paths file> on stdin (GENKEY).
# Exit 7 (asset volume missing or bad): the volume is made again and the step runs once more.
gen_run() {
    gn=$1; gcmd=$2; gpl=$3
    tar -cf $OUT/gen_in_$gn.tar -T "$gpl" || { rm -f $OUT/gen_in_$gn.tar; echo "$gn failed: tar of $gpl" >&2; exit 1; }
    w=$(pwd -W 2>/dev/null || pwd)
    for try in 1 2; do
        grc=0; rm -f "$OUT/rk_$gn.log"; rm -rf "$OUT/gen_out_$gn"; mkdir -p "$OUT/gen_out_$gn"
        MSYS_NO_PATHCONV=1 docker run --rm -i --memory "${PM_GEN_MEM:-6g}" -v "$w:/w" -v $AVOL:/a:ro -v "pm3ds-gconvobj-$(printf '%s' "$HIMG_FULL" | cut -c8-23):/gobj" -e GCONV_OBJ=/gobj -e GCONV_JOBS="${PM_GEN_JOBS:-16}" -e PM_CACHE="$CACHE" -e RKO="/w/$OUT/rk_$gn.log" -e GOUT="/w/$OUT/gen_out_$gn" -w /work pm3ds-host:1 sh -c "$gcmd" < $OUT/gen_in_$gn.tar || grc=$?
        [ "$grc" = 7 ] && [ "$try" = 1 ] || break
        docker volume rm $AVOL >/dev/null 2>&1 || true; asset_vol_make
    done
    rm -f $OUT/gen_in_$gn.tar
    if [ "$grc" -eq 7 ]; then docker volume rm $AVOL >/dev/null 2>&1 || true; echo "BUILD FAULT: the asset volume $AVOL failed its content check twice and was removed." >&2; exit 7; fi
    [ "$grc" = 0 ] || { echo "$gn failed" >&2; exit 1; }
    [ -s "$OUT/rk_$gn.log" ] || { echo "GENKEY FAULT: $gn wrote no read trace ($OUT/rk_$gn.log)" >&2; exit 1; }
}

# TEXKEY2: 3ds/include/texkey_patch.h (git-ignored): the alpha-0 key colours of each CI texture of the map archives, from the
# ROM and the texture sources (3ds/tools/texkey_bake.py, read only). Cached (gencache texkey). No other generator reads it.
tk_run() {
    tkb=$ASSET_BASE; command -v cygpath >/dev/null 2>&1 && tkb=$(cygpath -m "$ASSET_BASE")
    $PYX 3ds/tools/texkey_bake.py "$tkb" 3ds/include/texkey_patch.h || { echo "texkey_bake.py failed" >&2; exit 1; }
    gc_put texkey "$(cat $OUT/tk_key.txt)" texkey_patch.h:3ds/include/texkey_patch.h || true
}

# HC4f1: the shape converter (3ds/tools/nd_shape_conv.c, host C) writes one .nds/.dlc/.seam set per *_shape asset; yay0_test
# checks the Yay0 decoder on every asset first. BUILDO: the outputs live in the docker volume pm3ds-shape, one folder per key
# (/sh/<key sha1>/ with key.txt, out.sha1, f/); the build container copies the folder into the romfs of its workspace.
# The key: the command, the host image, the ROM and the content id of every file the two compiles can read (3ds/tools/inc_scan.py:
# the sources and every header an include line names, in all -I folders). A header the converter does not include does not miss.
SH_CMD='gcc -m32 -O2 -Wall -I /w/3ds/include -o /tmp/yay0_test /w/3ds/tools/yay0_test.c /w/3ds/src/nd_yay0.c && /tmp/yay0_test /ar && gcc -m32 -O2 -Wall -msse2 -mfpmath=sse -ffp-contract=off -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I /w/include -I /w/3ds/include -I /w/3ds/stub_include -o /tmp/nd_shape_conv /w/3ds/tools/nd_shape_conv.c /w/3ds/src/nd_seam.c /w/3ds/src/nd_dlc.c && /tmp/nd_shape_conv /ar /out'
SH_GROUPS="-I 3ds/include 3ds/tools/yay0_test.c,3ds/src/nd_yay0.c|-I include,3ds/include,3ds/stub_include 3ds/tools/nd_shape_conv.c,3ds/src/nd_seam.c,3ds/src/nd_dlc.c"
SHVOL=pm3ds-shape
[ "$CACHE" = off ] && SHVOL=pm3ds-shape-cold$$
# sh_run: makes the entry in the shape volume (temp folder, one mv). PM_CACHE=check compares with the entry of the key (exit 5 = differs).
sh_run() {
    w=$(pwd -W 2>/dev/null || pwd); rd=$(cd "$ASSET_BASE/ver/us" && pwd -W 2>/dev/null || pwd)
    shrc=0; MSYS_NO_PATHCONV=1 docker run --rm -i --memory "${PM_GEN_MEM:-6g}" -v "$w:/w:ro" -v "$rd/baserom.z64:/ar/ver/us/baserom.z64:ro" -v $SHVOL:/sh \
        -e E="$SHAPE_E" -e U="t$$_$(date +%s)" -e C="$CACHE" -e SEED="${PM_SHAPE_SEED:-}" -e SH_CMD="$SH_CMD" -w /tmp pm3ds-host:1 sh -c '
        set -e; T=/sh/.tmp.$U; rm -rf $T; mkdir -p $T/f; cat > $T/key.txt; ln -s $T/f /out
        sh -c "$SH_CMD" > /tmp/so 2>&1 || { cat /tmp/so; rm -rf $T; echo "gen_shapes failed" >&2; exit 1; }
        cat /tmp/so
        n=$(sed -n "s/^shapeconv maps \([0-9]*\) .*/\1/p" /tmp/so); [ -n "$SEED" ] && n=$((n + 1)) # SHAPECACHE seed
        c=$(ls $T/f | grep -c "\.nds$" || true)
        if [ -z "$n" ] || [ "$c" != "$n" ]; then rm -rf $T; echo "gen_shapes: $c .nds files, the converter wrote $n maps" >&2; exit 1; fi
        [ "$(sha1sum < $T/key.txt | cut -c1-40)" = "$E" ] || { rm -rf $T; echo "gen_shapes: key text and key differ" >&2; exit 1; }
        (cd $T && sha1sum f/* > out.sha1)
        rc=0
        if [ "$C" = check ]; then
            if [ -d /sh/$E ]; then
                if (cd /sh/$E && sha1sum -c --status out.sha1) && diff -r -q /sh/$E/f $T/f >/dev/null; then echo "cachecheck shape ok"
                else echo "CACHECHECK FAIL shape: the entry /sh/$E differs from a cold run, replaced"; rm -rf /sh/$E; rc=5; fi
            else echo "cachecheck shape: no entry for this key"; fi
        fi
        # a valid entry of another run stays; a bad one is replaced; a lost mv race keeps the winner
        if [ -d /sh/$E ]; then (cd /sh/$E && sha1sum -c --status out.sha1) || { rm -rf /sh/$E; mv -T $T /sh/$E 2>/dev/null || true; }
        else mv -T $T /sh/$E 2>/dev/null || true; fi
        rm -rf $T
        ls -1dt /sh/*/ 2>/dev/null | tail -n +13 | xargs -r rm -rf # the newest 12 keys stay
        exit $rc' < $OUT/shape_keytext.txt || shrc=$?
    [ "$shrc" = 0 ] || [ "$shrc" = 5 ] || { echo "gen_shapes failed (exit $shrc)" >&2; exit 1; }
    [ "$CACHE" = off ] || { mkdir -p "$GC_ROOT/shapevol"; : > "$GC_ROOT/shapevol/$SHAPE_E"; }
    [ "$shrc" = 0 ] || exit 5
}

# S7CONV: native tables of the static gfx lists (3ds/tools/gen_gfx_tab.sh): 3ds/src/nd_gfx_tab.c and nd_ntab_descs.c (git-ignored).
# GENKEY (3ds/tools/readkey.sh): K0 (command, host image, assets, trace tool, the path list of GT_IN) plus the content id of
# every file the step read (traced). texkey_patch.h stays out of the tar (texkey runs at the same time; no generator reads it).
GT_IN="3ds/tools 3ds/include 3ds/stub_include include"
GT_IN="$GT_IN src 3ds/src/nd_scomp.c 3ds/src/nd_texwin.c" # GTIN: the own-TU sources of the set (under src)
GT_CMD='
    set -e; '"$GEN_AV"'; '"$RK_SO"'
    '"$RK_TRACE"' sh 3ds/tools/gen_gfx_tab.sh "$GOUT/nd_gfx_tab.c" /tmp/gconv 2>&1 | grep -v "^gconv def"
    cp /tmp/rt/log "$RKO"'
GT_OUT="nd_gfx_tab.c:3ds/src/nd_gfx_tab.c nd_ntab_descs.c:3ds/src/nd_ntab_descs.c ntab_inherit.txt:build3ds/out/ntab_inherit.txt ntab_unsup.txt:build3ds/out/ntab_unsup.txt ntab_tables.txt:build3ds/out/ntab_tables.txt ntab_entries.txt:build3ds/out/ntab_entries.txt ntab_pairs.txt:build3ds/out/ntab_pairs.txt gset_ok.txt:build3ds/out/gset_ok.txt"
gt_run() {
    mkdir -p build3ds/out # gen_gfx_tab.sh writes its logs to build3ds/out only when the folder exists
    gen_run gen_gfx_tab "$GT_CMD" $OUT/rk_gfx_tab_paths.txt
    G=$OUT/gen_out_gen_gfx_tab
    [ -s $G/nd_gfx_tab.c ] && [ -s $G/nd_ntab_descs.c ] || { echo "gen_gfx_tab failed: no output" >&2; exit 1; }
    gen_put $G/nd_gfx_tab.c 3ds/src/nd_gfx_tab.c; gen_put $G/nd_ntab_descs.c 3ds/src/nd_ntab_descs.c
    rk_reads $OUT/rk_gen_gfx_tab.log $OUT/rk_gfx_tab_paths.txt $OUT/rk_gfx_tab_reads.txt 3ds/tools/gen_gfx_tab.sh 3ds/tools/nd_gfx_conv.c 3ds/include/nd_rmdef.h
    gc_put "$GT_RKDIR" "$(rk_full "$(cat $OUT/gt_k0.txt)" $OUT/in_now.txt $OUT/rk_gfx_tab_reads.txt)" $GT_OUT reads.txt:$OUT/rk_gfx_tab_reads.txt || true
    gc_trim gfx_tab
}
# NPSO3: the build-time pipeline table (3ds/tools/gen_pso_tab.py) -> 3ds/src/nd_pso_tab.c (git-ignored). GENKEY: K0 as gfx_tab
# plus the 2 gfx_tab outputs and the .c files that hold ND_PSO_DESC (the scan set); the python reads of other .c files leave.
PT_IN="3ds/tools 3ds/include 3ds/stub_include include src 3ds/src"
PT_CMD='
    set -e; '"$GEN_AV"'; '"$RK_SO"'
    '"$RK_TRACE"' python3 3ds/tools/gen_pso_tab.py "$GOUT/nd_pso_tab.c" src 3ds/src
    cp /tmp/rt/log "$RKO"'
pt_run() {
    gen_run gen_pso_tab "$PT_CMD" $OUT/rk_pso_tab_paths.txt
    G=$OUT/gen_out_gen_pso_tab
    [ -s $G/nd_pso_tab.c ] || { echo "gen_pso_tab failed: no output" >&2; exit 1; }
    gen_put $G/nd_pso_tab.c 3ds/src/nd_pso_tab.c
    awk -F'\t' '!($1 ~ /^python/ && $2 ~ /^(src|3ds\/src)\/.*\.c$/)' $OUT/rk_gen_pso_tab.log > $OUT/rk_pso_tab_log2.txt
    rk_reads $OUT/rk_pso_tab_log2.txt $OUT/rk_pso_tab_paths.txt $OUT/rk_pso_tab_reads.txt 3ds/tools/gen_pso_tab.py 3ds/tools/gen_pso_main.c 3ds/src/nd_decide.c 3ds/src/nd_psokey.c
    gc_put "$PT_RKDIR" "$(rk_full "$(cat $OUT/pt_k0.txt)" $OUT/in_now.txt $OUT/rk_pso_tab_reads.txt)" nd_pso_tab.c:3ds/src/nd_pso_tab.c reads.txt:$OUT/rk_pso_tab_reads.txt || true
    gc_trim pso_tab
}

# BUILDO: the key phase is one python process (3ds/tools/build_key.py: map_restore files, code ids, image ids, asset ids,
# generator cache lookups, shape key, build key, skip). It writes $OUT/key_env.sh. Exit 10: the image stamp differs; 11: no image.
export OUT MAIN_ROOT ASSET_BASE CODE_DIRS GEN_FILES GT_CMD PT_CMD SH_CMD SH_GROUPS GT_IN PT_IN GT_OUT PM_CACHE PM_BUILD_REV PM_KEY_ONLY
export PID=$$
bk() { $PYX 3ds/tools/build_key.py "$@"; }
IMG=pm3ds-build:1
for try in 1 2 3; do
    r=0; bk pre || r=$?
    [ "$r" = 0 ] && break
    if [ "$r" = 11 ]; then
        build_docker_ok || build_docker_restart || { echo "BUILD FAULT: docker engine not answering in 20 s (docker info), restart not done. Restart Docker Desktop / WSL, then build again." >&2; exit 3; }
        docker image inspect pm3ds-host:1 >/dev/null 2>&1 || { echo "BUILD FAULT: no image pm3ds-host:1 (docker/Dockerfile.host)" >&2; exit 3; }
        docker image inspect $IMG >/dev/null 2>&1 && continue
        r=10
    fi
    [ "$r" = 10 ] || { echo "BUILD FAULT: build_key.py pre exit $r" >&2; exit 1; }
    # LINKFAST: docker build only when the content ids of docker/ or the image id differ from the stamp.
    . $OUT/key_env.sh
    docker build -q -t $IMG -f docker/Dockerfile.build docker >/dev/null
    DSTAMP=$MAIN_ROOT/build3ds/img_build_stamp.txt; mkdir -p "$MAIN_ROOT/build3ds"
    printf '%s %s\n' "$DKEY" "$(docker image inspect -f '{{.Id}}' $IMG)" > "$DSTAMP.$$" && mv -f "$DSTAMP.$$" "$DSTAMP"
done
[ "$r" = 0 ] || { echo "BUILD FAULT: build_key.py pre exit $r after docker build" >&2; exit 1; }
. $OUT/key_env.sh
SHVOL=pm3ds-shape
[ "$CACHE" = off ] && SHVOL=pm3ds-shape-cold$$
stage key-inputs

# The generator phase. build_key.py did the cache lookups; the misses run at the same time under one build slot:
# texkey (host python) | shapes (host image) | gfx_tab then pso_tab (host image). Each job writes its log and its exit code.
if [ "$TK_MISS$SH_MISS$GT_MISS$PT_MISS" != 0000 ]; then
    slot_need
    [ "$TK_MISS" = 1 ] && { ( tk_run ) > $OUT/gen_tk.log 2>&1; echo $? > $OUT/gen_tk.rc; } &
    [ "$SH_MISS" = 1 ] && { ( sh_run ) > $OUT/gen_sh.log 2>&1; echo $? > $OUT/gen_sh.rc; } &
    ( set -e
      if [ "$GT_MISS" = 1 ]; then gt_run; bk post pso; . $OUT/key_env.sh; fi
      if [ "$PT_MISS" = 1 ]; then pt_run; fi ) > $OUT/gen_gp.log 2>&1 || { cat $OUT/gen_gp.log >&2; wait; echo "gen_gfx_tab/gen_pso_tab failed" >&2; exit 1; }
    cat $OUT/gen_gp.log >&2
    wait
    for j in tk sh; do
        [ -f $OUT/gen_$j.rc ] || continue
        cat $OUT/gen_$j.log >&2; r=$(cat $OUT/gen_$j.rc); rm -f $OUT/gen_$j.rc
        [ "$r" = 5 ] && { GC_FAIL=1; continue; }
        [ "$r" = 0 ] || { echo "generator $j failed (exit $r, log $OUT/gen_$j.log)" >&2; exit 1; }
    done
    # cachecheck of texkey, gfx_tab and pso_tab: gc_put printed its line; a FAIL line in the logs marks the run
    if [ "$CACHE" = check ] && grep -q '^CACHECHECK FAIL' $OUT/gen_gp.log $OUT/gen_tk.log 2>/dev/null; then GC_FAIL=1; fi
    bk post; . $OUT/key_env.sh
fi
stage gen
if [ "$CACHE" = check ]; then
    if [ "$GC_FAIL" = 0 ]; then echo "CACHECHECK gens ok" >&2; else echo "CACHECHECK gens FAIL" >&2; exit 1; fi
fi
for p in $CODE_DIRS; do [ -e "$p" ] || { echo "missing build input: $p" >&2; exit 1; }; done
for p in $ASSET_DIRS; do [ -e "$ASSET_BASE/$p" ] || { echo "missing build input: $ASSET_BASE/$p (assets come from the main worktree)" >&2; exit 1; }; done
# BSTALE: the build key (build_key.py build_key: content ids of the code dirs, docker/Dockerfile.build and this script, the
# generator outputs, the asset ids, the shape key, both image ids; pm_build.h is out). TOOLB1: PM_KEY_ONLY=1 prints it and stops.
KEY_FILE=$OUT/build_key.txt
if [ -n "$PM_KEY_ONLY" ]; then echo "$KEY"; [ "$CACHE" = off ] && docker volume rm $SHVOL $AVOL >/dev/null 2>&1; exit 0; fi
publish_3dsx() {
    if [ -z "$PM_LANE" ] && [ -n "$APPDATA" ] && [ -f build3ds/pm_3ds.3dsx ]; then
        dest="$(cygpath -u "$APPDATA" 2>/dev/null || printf '%s' "$APPDATA")/Azahar/sdmc/3ds/PaperMario"
        mkdir -p "$dest"
        cp -f build3ds/pm_3ds.3dsx "$dest/pm_3ds.3dsx"
    fi
}
# MEMGATE: the console memory check of this build's map (3ds/tools/memcheck.py). OVER is a warning here (push_3ds.sh stops it).
memcheck_line() { $PYX 3ds/tools/memcheck.py --map $OUT/pm_3ds.map > $OUT/memcheck_line.txt 2>&1 || true; cat $OUT/memcheck_line.txt; }
# HC0/LOOP2: the hot code line runs detached; it goes to $OUT/hotset_line.txt (temp name, one mv).
hotset_line() {
    ht=$OUT/hotset_line.tmp.$$
    ( $PYX 3ds/tools/hotset.py --line > $ht 2>&1 || echo "hotset: failed (the build is not affected)" >> $ht; mv -f $ht $OUT/hotset_line.txt ) < /dev/null > /dev/null 2>&1 &
    echo "hotset: detached, its line goes to $OUT/hotset_line.txt"
}
# Cache safety: the skip needs the 3dsx to match the hash written with the key (build_key.py skip_ok). PM_CACHE=off and
# PM_BUILD_REV never skip.
if [ "$SKIP" = 1 ]; then
    echo "--- build up to date (key $(echo "$KEY" | cut -c1-12)), skipping ---"
    stage key
    publish_3dsx
    if [ -s $OUT/memcheck_line.txt ] && [ $OUT/memcheck_line.txt -nt $OUT/pm_3ds.map ]; then cat $OUT/memcheck_line.txt; else memcheck_line; fi
    [ "$FNV" = OFF ] && hotset_line
    exit 0
fi
stage key
# pm_build.h describes the 3dsx that is built; rewritten only when its text changes (a new mtime rebuilds its 3 includers).
PMB_TMP=3ds/include/pm_build.h.tmp.$$
printf '#define PM_BUILD_STR "%s"\n' "$BN" > "$PMB_TMP"
printf '#define PM_GIT_HASH "%s"\n' "$(git rev-parse --short "$REV")" >> "$PMB_TMP"
cmp -s "$PMB_TMP" 3ds/include/pm_build.h && rm -f "$PMB_TMP" || mv -f "$PMB_TMP" 3ds/include/pm_build.h
rm -f $KEY_FILE $OUT/build_out.sha1 # written only after a successful build

slot_need
# The workspace of the slot (cold: its own new volumes). The slot manifest (path, content id) and its id live in the main
# folder; the source volume holds the id of its last finished sync in /src/.pm_manifest_id ("dirty" during a sync).
if [ "$CACHE" = off ]; then WS=cold$$; else WS=ws$BUILD_SLOT; fi
SRCVOL=pm3ds-src-$WS; VOL=pm3ds-build-$WS; NAME=pm3ds-$WS
WSM=$MAIN_ROOT/build3ds/wsmeta; mkdir -p $WSM
bk src_now
JOBS=${PM_BUILD_JOBS:-16}; MEM=${PM_BUILD_MEM:-6g}
[ "$CACHE" = off ] && { JOBS=${PM_BUILD_JOBS:-28}; MEM=${PM_BUILD_MEM:-10g}; }
WD=${PM_BUILD_CAP:-600}
( sleep "$WD" >/dev/null 2>&1 & s=$!; trap 'kill $s 2>/dev/null' TERM; wait $s
  if [ -n "$(docker ps -q -f "name=^$NAME\$")" ]; then
      echo "BUILD TIMEOUT: container $NAME still running after $WD s, killed. Set PM_BUILD_CAP to change the cap." >&2
      docker kill "$NAME" >/dev/null 2>&1
  fi ) &
wdpid=$!
# ws_run: one container: asset check (exit 7), romfs from the shape volume (exit 8), the sync (exit 9 on an id mismatch), then
# configure and ninja. Exits 7-9 come before any write to /src, so the slot manifest stays true.
ws_run() {
    OLDID=$(cat $WSM/$WS.id 2>/dev/null || true); [ -n "$OLDID" ] && [ -f $WSM/$WS.manifest ] || { OLDID=; : > $WSM/$WS.manifest.new; mv -f $WSM/$WS.manifest.new $WSM/$WS.manifest; }
    NEWID="g$(date +%s)_$$"
    awk -F'\t' 'FILENAME==ARGV[1]{old[$1]=$2; next} (old[$1] != $2){print $1}' $WSM/$WS.manifest $OUT/src_now.txt > $OUT/src_changed.txt
    awk -F'\t' 'FILENAME==ARGV[1]{now[$1]=1; next} !($1 in now){print $1}' $OUT/src_now.txt $WSM/$WS.manifest > $OUT/src_deleted.txt
    # a new or a removed path re-runs configure (the CMake globs)
    RECONF=$(awk -F'\t' 'FILENAME==ARGV[1]{old[$1]=1; next} !($1 in old){n++} END{print (n > 0) ? 1 : 0}' $WSM/$WS.manifest $OUT/src_now.txt)
    [ -s $OUT/src_deleted.txt ] && RECONF=1
    nchg=$(grep -c . $OUT/src_changed.txt || true); ndel=$(grep -c . $OUT/src_deleted.txt || true)
    echo "sync: $nchg changed, $ndel deleted (workspace $WS)" >&2
    rm -f $OUT/sync_done.txt
    # STALEOBJ: tar -m stamps each copied file with the copy time, so it is newer than every object in the build volume.
    tar -cf - -T $OUT/src_changed.txt | MSYS_NO_PATHCONV=1 docker run --rm -i --memory "$MEM" --name "$NAME" -v $SRCVOL:/src -v $AVOL:/a:ro -v $SHVOL:/sh:ro \
        -e CCACHE_DIR=/ccache -e CCACHE_MAXSIZE="${PM_CCACHE_MAX:-4G}" -e PM_BUILD_NUM="$BN" -e PM_FNCOV_V="$FNV" -e AHASH="$AHASH" \
        -e IMGID="$IMGID" -e PM_CACHE_V="$CACHE" -e SHAPE_E="$SHAPE_E" -e OLDID="$OLDID" -e NEWID="$NEWID" -e RECONF="$RECONF" -e JOBS="$JOBS" \
        -v "$(pwd)/$OUT:/out" -v $VOL:/work -v pm3ds-ccache-$(printf '%s' "$IMGID" | cut -c1-12):/ccache \
        $IMG sh -c '
    set -e -f
    t0=$(date +%s)
    stage() { now=$(date +%s); echo "STAGE $1 $((now - t0)) s" >&2; t0=$now; }
    { [ "$(cat /a/c/key.txt 2>/dev/null)" = "$AHASH" ] && (cd /a/c && sha1sum -c --status out.sha1); } || { echo "ASSET VOLUME CHECK FAILED" >&2; exit 7; }
    stage asset-check
    if [ "$(cat /work/.img 2>/dev/null)" != "$IMGID" ] || [ "$PM_CACHE_V" = off ]; then rm -rf /work/build; printf %s "$IMGID" > /work/.img; fi
    if [ "$PM_CACHE_V" = off ]; then export CCACHE_DISABLE=1; fi
    if [ "$(cat /work/.shape 2>/dev/null)" != "$SHAPE_E" ]; then rm -f /work/build/pm_3ds.3dsx; printf %s "$SHAPE_E" > /work/.shape; fi
    # LINKFAST: the romfs in the build volume. Key: the shape content key. Every build checks the files (romfs.sha1); a new key,
    # a failed check or PM_CACHE=off copies the shape entry again (temp folder, check against the entry, one mv), removes the 3dsx.
    if [ "$PM_CACHE_V" = off ] || [ "$(cat /work/.romfs_key 2>/dev/null)" != "$SHAPE_E" ] || ! (cd /work/romfs/shape 2>/dev/null && sha1sum -c --status /work/romfs.sha1 2>/dev/null); then
        rm -rf /work/romfs /work/romfs.tmp /work/romfs.sha1 /work/.romfs_key /work/build/pm_3ds.3dsx
        S=/sh/$SHAPE_E
        { [ "$(sha1sum < $S/key.txt 2>/dev/null | cut -c1-40)" = "$SHAPE_E" ] && (cd $S && sha1sum -c --status out.sha1); } || { echo "SHAPE ENTRY MISSING OR BAD: $S" >&2; exit 8; }
        mkdir -p /work/romfs.tmp/shape; cp -a $S/f/. /work/romfs.tmp/shape/
        (cd /work/romfs.tmp/shape && find . -type f | LC_ALL=C sort | xargs -r -d "\n" sha1sum) > /work/romfs.sha1
        (cd $S/f && sha1sum -c --status /work/romfs.sha1) || { echo "ROMFS COPY CHECK FAILED" >&2; exit 1; }
        mv -T /work/romfs.tmp /work/romfs; printf %s "$SHAPE_E" > /work/.romfs_key
    fi
    stage romfs
    [ "$(cat /src/.pm_manifest_id 2>/dev/null)" = "$OLDID" ] || { echo "SOURCE VOLUME ID MISMATCH (want \"$OLDID\")" >&2; exit 9; }
    printf dirty > /src/.pm_manifest_id
    tar -xmf - -C /src
    if [ -s /out/src_deleted.txt ]; then while IFS= read -r f; do rm -f "/src/$f"; done < /out/src_deleted.txt; fi
    for p in assets/us ver/us/build; do [ -L /src/$p ] || rm -rf /src/$p; mkdir -p /src/${p%/*}; ln -sfn /a/c/$p /src/$p; done
    printf %s "$NEWID" > /src/.pm_manifest_id; printf %s "$NEWID" > /out/sync_done.txt
    stage sync
    # Configure when the build dir is missing, has other flags, another build number (the smdh text) or a path was added or removed.
    if [ "$RECONF" = 1 ] || ! grep -q "^CMAKE_C_COMPILER_LAUNCHER:STRING=ccache$" /work/build/CMakeCache.txt 2>/dev/null \
        || ! grep -q "^PM_FNCOV:BOOL=$PM_FNCOV_V\$" /work/build/CMakeCache.txt 2>/dev/null \
        || ! grep -q "^CMAKE_HOME_DIRECTORY:INTERNAL=/src/3ds$" /work/build/CMakeCache.txt 2>/dev/null \
        || ! grep -q "^PM_BUILD_NUM:STRING=$PM_BUILD_NUM\$" /work/build/CMakeCache.txt 2>/dev/null; then
        cmake -G Ninja -S /src/3ds -B /work/build -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache -DPM_BUILD_NUM=$PM_BUILD_NUM -DPM_FNCOV=$PM_FNCOV_V
    fi
    stage configure
    status=0
    cmake --build /work/build --parallel "$JOBS" -- -k 0 '"$*"' || status=$?
    stage compile-link
    cp -f /work/build/pm_3ds.3dsx /work/build/pm_3ds.smdh /work/build/pm_3ds.map /work/build/pm_3ds.elf /out/ 2>/dev/null || true
    stage copy-out
    exit $status
' || return $?
}
rc=0; done7=0; done8=0; done9=0
while :; do
    rc=0; ws_run "$@" || rc=$?
    if [ "$(cat $OUT/sync_done.txt 2>/dev/null)" = "$NEWID" ]; then
        cp -f $OUT/src_now.txt $WSM/$WS.manifest.new && mv -f $WSM/$WS.manifest.new $WSM/$WS.manifest && printf '%s\n' "$NEWID" > $WSM/$WS.id
    fi
    if [ "$rc" = 125 ] && [ -n "$(docker ps -a -q -f "name=^$NAME\$")" ]; then
        # a container of a dead holder of this slot still runs: it goes, the id check then decides the sync
        echo "workspace $WS: old container $NAME removed" >&2; docker rm -f "$NAME" >/dev/null 2>&1 || true; continue
    fi
    if [ "$rc" = 7 ] && [ "$done7" = 0 ]; then done7=1; docker volume rm $AVOL >/dev/null 2>&1 || true; asset_vol_make; continue; fi
    if [ "$rc" = 8 ] && [ "$done8" = 0 ]; then done8=1; rm -f "$GC_ROOT/shapevol/$SHAPE_E"; echo "shape entry $SHAPE_E missing in $SHVOL: made again" >&2; ( sh_run ) >&2 || { rc=1; break; }; continue; fi
    if [ "$rc" = 9 ] && [ "$done9" = 0 ]; then done9=1; echo "workspace $WS: source volume id mismatch, full sync" >&2; rm -f $WSM/$WS.id; docker volume rm $SRCVOL >/dev/null 2>&1 || true; continue; fi
    break
done
kill "$wdpid" 2>/dev/null || true
if [ "$CACHE" = off ]; then docker volume rm $SRCVOL $VOL $SHVOL $AVOL >/dev/null 2>&1 || true; rm -f $WSM/$WS.id $WSM/$WS.manifest; fi
if [ "$rc" -eq 7 ]; then docker volume rm $AVOL >/dev/null 2>&1 || true; echo "BUILD FAULT: the asset volume $AVOL failed its content check and was removed. Build again (the next build makes it new)." >&2; exit 7; fi
if [ "$rc" -ne 0 ]; then echo "BUILD FAILED exit $rc (container $NAME; stage timings above; memory limit $MEM, $JOBS jobs: a \"Killed signal\" above is an out-of-memory kill)" >&2; exit "$rc"; fi
TB=$(date +%s)
if [ -z "$PM_BUILD_REV" ]; then sha1sum < $OUT/pm_3ds.3dsx | cut -c1-40 > $OUT/build_out.sha1; echo "$KEY" > $KEY_FILE; fi
# HW5: keep the ELF of each build under its build number (crash dumps: 3ds/tools/decode_dump.py). CLEANB: the newest 3 pairs stay.
if [ -f $OUT/pm_3ds.elf ]; then
    mkdir -p 3ds/logs/builds
    cp -f $OUT/pm_3ds.elf "3ds/logs/builds/pm_3ds_$BN.elf"
    cp -f $OUT/pm_3ds.map "3ds/logs/builds/pm_3ds_$BN.map" 2>/dev/null || true
    ls -t 3ds/logs/builds/pm_3ds_*.elf 2>/dev/null | tail -n +4 | while read -r old; do rm -f "$old" "${old%.elf}.map"; done
    ls -t 3ds/logs/builds/pm_3ds_*.map 2>/dev/null | tail -n +4 | while read -r old; do rm -f "$old"; done
fi
publish_3dsx
memcheck_line
[ "$FNV" = OFF ] && hotset_line
stage tail
