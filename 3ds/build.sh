#!/bin/sh
# Build the 3DS port inside the devkitpro image with ccache (docker/Dockerfile.build).
# Usage (repo root): sh 3ds/build.sh [extra ninja args]
# Output: build3ds/pm_3ds.3dsx (a lane: build3ds/lane-<PM_LANE>/), also copied to %APPDATA%\Azahar\sdmc\3ds\PaperMario\
#
# The container reads the CMake inputs from the named volume pm3ds-src at /src (same layout as the repo).
# Only changed input files are copied into it. The manifest in $OUT/src_manifest.txt holds path, size, mtime.
# Objects, the CMake dir and ccache live in named volumes. A lane (PM_LANE) has its own volumes and output folder.
# Hang guard: a watchdog kills this run's container after PM_BUILD_CAP seconds (default 600).
set -e
cd "$(dirname "$0")/.."
OUT=build3ds; VOL=pm3ds-build; SRCVOL=pm3ds-src
if [ -n "$PM_LANE" ]; then OUT=build3ds/lane-$PM_LANE; VOL=pm3ds-build-$PM_LANE; SRCVOL=pm3ds-src-$PM_LANE; fi
# HC0: PM_FNCOV=1 is the census lane <L>_fncov (needs PM_LANE): its own output folder and build volume, the draw TUs instrumented.
FNV=OFF
if [ "${PM_FNCOV:-0}" = 1 ]; then
    [ -n "$PM_LANE" ] || { echo "PM_FNCOV=1 needs PM_LANE" >&2; exit 1; }
    FNV=ON; OUT=build3ds/lane-${PM_LANE}_fncov; VOL=pm3ds-build-${PM_LANE}_fncov
fi
mkdir -p $OUT
NAME="pm3ds-build-${PM_LANE:-main}"
# BN1: one build number = V0 + (commit count - C0), V0 and C0 in 3ds/build_base.txt. No -dirty suffix (user decision 2026-10-19). Suffix -<lane> (PM_LANE set) or -laneB (branch lane-b).
# Written to 3ds/include/pm_build.h (ignored by git). The boot log and the Options panel show PM_BUILD_STR.
read V0 C0 <<EOF2
$(tr -d '\015' < 3ds/build_base.txt)
EOF2
# BENCH4: PM_BUILD_REV = the commit to build (bench.py build-at: a git archive folder, GIT_DIR set by the caller). Unset: HEAD.
REV=${PM_BUILD_REV:-HEAD}
BN=$((V0 + $(git rev-list --count "$REV") - C0))
if [ -n "$PM_LANE" ]; then BN="$BN-$PM_LANE"; elif [ -z "$PM_BUILD_REV" ] && [ "$(git branch --show-current)" = "lane-b" ]; then BN="$BN-laneB"; fi
# AZT9c-1: a lane build carries a unique tag (time and pid): a dirty build with the same number has its own build id, so a warm restart proves the new ROM by the boot line
if [ -n "$PM_LANE" ]; then BN="${BN}_$(date +%H%M%S)$$"; fi

# The inputs CMake reads (3ds/CMakeLists.txt, DECOMP = repo root). Nothing else reaches the build.
SRC_DIRS="3ds/CMakeLists.txt 3ds/effects.cmake 3ds/battle.cmake 3ds/maps.cmake 3ds/map_syms.lds LaunchIcon.png 3ds/ld 3ds/shaders 3ds/stub_include 3ds/include 3ds/src src include assets/us ver/us/build"
for p in $SRC_DIRS; do
    [ -e "$p" ] || { echo "missing build input: $p" >&2; exit 1; }
done
# BSTALE: skip the build when the inputs have the same content as at the last successful build of this output folder.
# The key covers the tracked inputs by git tree id and by their uncommitted diff, the untracked files by content, the
# docker recipe, and the size and mtime of the ignored inputs (assets/us, ver/us/build: not in git). pm_build.h is not
# in the key: a commit that only changes the build number does not rebuild. A PM_BUILD_REV build (bench build-at) always builds.
# The skip also needs the source volume to carry the id of the last sync (SRCID, below): a recreated volume rebuilds.
# build.sh is the only stale check: go.ps1 Invoke-Build and push_3ds.sh call it and never compare times themselves.
KEY_IN="3ds/CMakeLists.txt 3ds/effects.cmake 3ds/battle.cmake 3ds/maps.cmake 3ds/map_syms.lds LaunchIcon.png 3ds/ld 3ds/shaders 3ds/stub_include 3ds/include 3ds/src src include docker/Dockerfile.build 3ds/build.sh"
KEY_FILE=$OUT/build_key.txt
key_now() {
    {
        git ls-tree -r HEAD -- $KEY_IN
        git diff HEAD -- $KEY_IN
        git ls-files -o --exclude-standard -- $KEY_IN | while read -r f; do sha1sum "$f"; done
        find assets/us ver/us/build -type f -printf '%p %s %T@\n' | sort
    } | sha1sum | cut -d' ' -f1
}
# Azahar loads from its own SD card, not from build3ds/.
publish_3dsx() {
    if [ -z "$PM_LANE" ] && [ -n "$APPDATA" ] && [ -f build3ds/pm_3ds.3dsx ]; then
        dest="$(cygpath -u "$APPDATA" 2>/dev/null || printf '%s' "$APPDATA")/Azahar/sdmc/3ds/PaperMario"
        mkdir -p "$dest"
        cp -f build3ds/pm_3ds.3dsx "$dest/pm_3ds.3dsx"
    fi
}
# MEMGATE: the console memory check of this build's map (3ds/tools/memcheck.py, mem_budget.json). OVER is a warning here:
# push_3ds.sh stops an OVER push unless --mem-ok.
memcheck_line() {
    if command -v py >/dev/null 2>&1; then py -3 3ds/tools/memcheck.py --map $OUT/pm_3ds.map || true
    else python3 3ds/tools/memcheck.py --map $OUT/pm_3ds.map || true; fi
}
# HC0: one hot code line after the memcheck line (3ds/tools/hotset.py: M1 per class and union, M2 linker group). Normal lane only;
# a failure prints a note and does not fail the build.
hotset_line() {
    if command -v py >/dev/null 2>&1; then py -3 3ds/tools/hotset.py --line || echo "hotset: failed (the build is not affected)"
    else python3 3ds/tools/hotset.py --line || echo "hotset: failed (the build is not affected)"; fi
}
IMG=pm3ds-build:1
docker build -q -t $IMG -f docker/Dockerfile.build docker >/dev/null
# SRCID: the source volume keeps the id of its last sync in /src/.pm_manifest_id, and $VID holds the id the manifest was
# written with. The manifest is valid only while both match. A missing, recreated or foreign source volume has no id
# (or another one): the manifest is dropped and every file is copied. Also the skip check needs the match (BSTALE).
MAN=$OUT/src_manifest.txt
VID=$OUT/src_volid.txt
volid_now() { MSYS_NO_PATHCONV=1 docker run --rm -v $SRCVOL:/src $IMG sh -c 'cat /src/.pm_manifest_id 2>/dev/null; true'; }
VOLID=$(volid_now)
vol_match() { [ -n "$VOLID" ] && [ "$VOLID" = "$(cat $VID 2>/dev/null)" ]; }
if [ -z "$PM_BUILD_REV" ]; then
    KEY=$(key_now)
    if [ -f $OUT/pm_3ds.3dsx ] && [ "$(cat $KEY_FILE 2>/dev/null)" = "$KEY" ] && vol_match; then
        echo "--- build up to date (key $(echo "$KEY" | cut -c1-12)), skipping ---"
        publish_3dsx
        memcheck_line
        [ "$FNV" = OFF ] && hotset_line
        exit 0
    fi
fi
# pm_build.h describes the 3dsx that is built (push_3ds.sh archives from it, bench.py checks its mtime), so it is written
# here, after the skip check: a skipped build keeps the header of the 3dsx it kept. Rewritten only when the number changed,
# since a new mtime would rebuild every file that includes it.
printf '#define PM_BUILD_STR "%s"\n' "$BN" > 3ds/include/pm_build.h.tmp
printf '#define PM_GIT_HASH "%s"\n' "$(git rev-parse --short "$REV")" >> 3ds/include/pm_build.h.tmp
cmp -s 3ds/include/pm_build.h.tmp 3ds/include/pm_build.h && rm -f 3ds/include/pm_build.h.tmp || mv -f 3ds/include/pm_build.h.tmp 3ds/include/pm_build.h
# A failed build must not leave the old key behind: the key file is written only after a successful build.
rm -f $KEY_FILE

# BENCH4: a build of another commit drops the shared source volume first, so no file of the main folder stays in it. The
# main sync compares against its own manifest, so the volume is dropped again after the build (the next main sync copies all).
if [ -n "$PM_BUILD_REV" ]; then docker volume rm $SRCVOL >/dev/null 2>&1 || true; VOLID=""; fi
# Sync: copy only files whose size or mtime changed since the last sync. No id match: copy everything (SRCID).
t0=$(date +%s)
if vol_match; then SYNCID=$VOLID; else rm -f $MAN; SYNCID="$(date +%s)_$$"; fi
[ -f $MAN ] || : > $MAN
find $SRC_DIRS -type f -printf '%p\t%s\t%T@\n' | sort > $OUT/src_now.txt
# Changed or new: not equal to the manifest. Deleted: in the manifest, not in the current list.
awk -F'\t' 'FILENAME==ARGV[1]{old[$1]=$2 "\t" $3; next} (old[$1] != $2 "\t" $3){print $1}' $MAN $OUT/src_now.txt > $OUT/src_changed.txt
awk -F'\t' 'FILENAME==ARGV[1]{now[$1]=1; next} !($1 in now){print $1}' $OUT/src_now.txt $MAN > $OUT/src_deleted.txt
nchg=$(wc -l < $OUT/src_changed.txt); ndel=$(wc -l < $OUT/src_deleted.txt)
if [ -s $OUT/src_changed.txt ]; then
    tar -cf - -T $OUT/src_changed.txt | MSYS_NO_PATHCONV=1 docker run --rm -i -v $SRCVOL:/src $IMG sh -c "tar -xf - -C /src && printf %s '$SYNCID' > /src/.pm_manifest_id"
fi
if [ -s $OUT/src_deleted.txt ]; then
    MSYS_NO_PATHCONV=1 docker run --rm -i -v $SRCVOL:/src $IMG sh -c 'while read -r f; do rm -f "/src/$f"; done' < $OUT/src_deleted.txt
fi
cp -f $OUT/src_now.txt $MAN
echo "$SYNCID" > $VID
echo "STAGE sync-host $(( $(date +%s) - t0 )) s (changed $nchg, deleted $ndel)" >&2

# Hang guard: after PM_BUILD_CAP seconds the watchdog kills only this run's container, by name.
WD=${PM_BUILD_CAP:-600}
( sleep "$WD" >/dev/null 2>&1 & s=$!; trap 'kill $s 2>/dev/null' TERM; wait $s
  if [ -n "$(docker ps -q -f "name=^$NAME\$")" ]; then
      echo "BUILD TIMEOUT: container $NAME still running after $WD s, killed. Set PM_BUILD_CAP to change the cap." >&2
      docker kill "$NAME" >/dev/null 2>&1
  fi ) &
wdpid=$!
rc=0
MSYS_NO_PATHCONV=1 docker run --rm --name "$NAME" -v $SRCVOL:/src:ro \
    -e CCACHE_DIR=/ccache -e CCACHE_MAXSIZE="${PM_CCACHE_MAX:-2G}" -e PM_BUILD_NUM="$BN" -e PM_FNCOV_V="$FNV" \
    -v "$(pwd)/$OUT:/out" \
    -v $VOL:/work \
    -v pm3ds-ccache:/ccache \
    $IMG sh -c '
    set -e -f
    t0=$(date +%s)
    stage() { now=$(date +%s); echo "STAGE $1 $((now - t0)) s" >&2; t0=$now; }
    # Configure only when the build dir is missing, was made with ccache, points at another source root, or has another build number (the smdh description).
    if ! grep -q "^CMAKE_C_COMPILER_LAUNCHER:STRING=ccache$" /work/build/CMakeCache.txt 2>/dev/null \
        || ! grep -q "^PM_FNCOV:BOOL=$PM_FNCOV_V\$" /work/build/CMakeCache.txt 2>/dev/null \
        || ! grep -q "^CMAKE_HOME_DIRECTORY:INTERNAL=/src/3ds$" /work/build/CMakeCache.txt 2>/dev/null \
        || ! grep -q "^PM_BUILD_NUM:STRING=$PM_BUILD_NUM\$" /work/build/CMakeCache.txt 2>/dev/null; then
        cmake -G Ninja -S /src/3ds -B /work/build -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache -DPM_BUILD_NUM=$PM_BUILD_NUM -DPM_FNCOV=$PM_FNCOV_V
    fi
    stage configure
    status=0
    cmake --build /work/build --parallel "$(nproc)" -- -k 0 '"$*"' || status=$?
    stage compile-link
    cp -f /work/build/pm_3ds.3dsx /work/build/pm_3ds.smdh /work/build/pm_3ds.map /work/build/pm_3ds.elf /out/ 2>/dev/null || true
    # The objects stay: ninja then rebuilds only the sources whose mtime is newer (BSTALE). ccache is the cache for a clean build.
    stage copy-out
    exit $status
' || rc=$?
kill "$wdpid" 2>/dev/null || true
if [ -n "$PM_BUILD_REV" ]; then docker volume rm $SRCVOL >/dev/null 2>&1 || true; fi
if [ "$rc" -ne 0 ]; then echo "BUILD FAILED exit $rc (container $NAME; stage timings above)" >&2; exit "$rc"; fi
if [ -z "$PM_BUILD_REV" ]; then echo "$KEY" > $KEY_FILE; fi
# HW5: keep the ELF of each build under its build number, so a Luma crash dump can be decoded later (3ds/tools/decode_dump.py).
if [ -f $OUT/pm_3ds.elf ]; then
    mkdir -p 3ds/logs/builds
    cp -f $OUT/pm_3ds.elf "3ds/logs/builds/pm_3ds_$BN.elf"
    cp -f $OUT/pm_3ds.map "3ds/logs/builds/pm_3ds_$BN.map" 2>/dev/null || true
    # CLEANB: keep the newest 3 .elf/.map pairs (maps are capped alone too). The .3dsx archives are capped by push_3ds.sh.
    ls -t 3ds/logs/builds/pm_3ds_*.elf 2>/dev/null | tail -n +4 | while read -r old; do
        rm -f "$old" "${old%.elf}.map"
    done
    ls -t 3ds/logs/builds/pm_3ds_*.map 2>/dev/null | tail -n +4 | while read -r old; do
        rm -f "$old"
    done
fi
publish_3dsx
memcheck_line
[ "$FNV" = OFF ] && hotset_line
# Cap the lane volume sets: keep the newest PM_LANE_KEEP lanes (default 5), by creation time.
# A lane with a registered worktree (build3ds/wt/<name>, from git worktree list) is live: never removed, not counted.
# The base volumes (pm3ds-build, pm3ds-src, pm3ds-work, pm3ds-ccache) and the current lane are never removed.
KEEP=${PM_LANE_KEEP:-5}
# A build-at folder (build3ds/at_<short> of the main folder, marker .build_at_in_use under 12 h old) is live too.
MAIN=$(git worktree list --porcelain | sed -n '1s|^worktree ||p')
LIVE=$( (git worktree list --porcelain | sed -n 's|^worktree .*/build3ds/wt/||p'; find "$MAIN/build3ds" -maxdepth 2 -name .build_at_in_use -mmin -720 2>/dev/null | sed -n 's|.*/build3ds/\(at_[^/]*\)/.build_at_in_use$|\1|p') )
docker volume ls -q | sed -n -E 's/^pm3ds-build-(.+)$/\1/p' | while read -r l; do
    printf '%s\n' "$LIVE" | grep -qx -- "$l" && continue
    echo "$(docker volume inspect --format '{{.CreatedAt}}' pm3ds-build-$l) $l"
done | sort -r | awk '{print $2}' | {
    n=0
    while read -r l; do
        n=$((n + 1))
        if [ "$n" -gt "$KEEP" ] && [ "$l" != "${PM_LANE:-}" ]; then
            docker volume rm pm3ds-build-$l pm3ds-src-$l pm3ds-work-$l >/dev/null 2>&1 || true
        fi
    done
}
