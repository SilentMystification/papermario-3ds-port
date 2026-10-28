#!/bin/sh
# Build the 3DS port inside the devkitpro image with ccache (docker/Dockerfile.build).
# Usage (repo root): sh 3ds/build.sh [extra ninja args]
# Output: build3ds/pm_3ds.3dsx (a lane: build3ds/lane-<PM_LANE>/), also copied to %APPDATA%\Azahar\sdmc\3ds\PaperMario\
#
# The container sees only the paths CMake reads. Those are read-only bind mounts at /src (same layout as the repo).
# Objects, the CMake dir and ccache live in named volumes. A lane (PM_LANE) has its own build volume and output folder.
# Hang guard: a watchdog kills this run's container after PM_BUILD_CAP seconds (default 600).
set -e
cd "$(dirname "$0")/.."
OUT=build3ds; VOL=pm3ds-build
if [ -n "$PM_LANE" ]; then OUT=build3ds/lane-$PM_LANE; VOL=pm3ds-build-$PM_LANE; fi
mkdir -p $OUT
NAME="pm3ds-build-${PM_LANE:-main}"
# BN1: one build number = V0 + (commit count - C0), V0 and C0 in 3ds/build_base.txt. No -dirty suffix (user decision 2026-10-19). Suffix -<lane> (PM_LANE set) or -laneB (branch lane-b).
# Written to 3ds/include/pm_build.h (ignored by git). The boot log and the Options panel show PM_BUILD_STR.
read V0 C0 <<EOF2
$(tr -d '\015' < 3ds/build_base.txt)
EOF2
BN=$((V0 + $(git rev-list --count HEAD) - C0))
if [ -n "$PM_LANE" ]; then BN="$BN-$PM_LANE"; elif [ "$(git branch --show-current)" = "lane-b" ]; then BN="$BN-laneB"; fi
printf '#define PM_BUILD_STR "%s"\n' "$BN" > 3ds/include/pm_build.h.tmp
# Rewrite pm_build.h only when the number changed: a new mtime would rebuild every file that includes it.
cmp -s 3ds/include/pm_build.h.tmp 3ds/include/pm_build.h && rm -f 3ds/include/pm_build.h.tmp || mv -f 3ds/include/pm_build.h.tmp 3ds/include/pm_build.h

# The inputs CMake reads (3ds/CMakeLists.txt, DECOMP = repo root). Nothing else is mounted.
INPUT_FILES="3ds/CMakeLists.txt 3ds/effects.cmake 3ds/battle.cmake 3ds/maps.cmake 3ds/map_syms.lds LaunchIcon.png"
INPUT_DIRS="3ds/ld 3ds/shaders 3ds/stub_include 3ds/include 3ds/src src include assets/us ver/us/build"
SRC_MODE=${PM_SRC_MODE:-mount}
SFX=""; [ -n "$PM_LANE" ] && SFX="-$PM_LANE"
if [ "$SRC_MODE" = rsync ]; then
    # Measurement variant: one rsync of an explicit file list into a volume.
    { for f in $INPUT_FILES; do echo "$f"; done; for d in $INPUT_DIRS; do find "$d" -type f; done; } > "$OUT/srclist.txt"
    SRCARGS="-v $(pwd):/host:ro -v pm3ds-src$SFX:/src"
    RSYNC_SRC=1
else
    SRCARGS=""
    for p in $INPUT_FILES $INPUT_DIRS; do
        [ -e "$p" ] || { echo "missing build input: $p" >&2; exit 1; }
        SRCARGS="$SRCARGS -v $(pwd)/$p:/src/$p:ro"
    done
    RSYNC_SRC=""
fi

# Hang guard: after PM_BUILD_CAP seconds the watchdog kills only this run's container, by name.
WD=${PM_BUILD_CAP:-600}
( sleep "$WD" >/dev/null 2>&1 & s=$!; trap 'kill $s 2>/dev/null' TERM; wait $s
  if [ -n "$(docker ps -q -f "name=^$NAME\$")" ]; then
      echo "BUILD TIMEOUT: container $NAME still running after $WD s, killed. Set PM_BUILD_CAP to change the cap." >&2
      docker kill "$NAME" >/dev/null 2>&1
  fi ) &
wdpid=$!
IMG=pm3ds-build:1
docker build -q -t $IMG -f docker/Dockerfile.build docker >/dev/null
rc=0
MSYS_NO_PATHCONV=1 docker run --rm --name "$NAME" $SRCARGS \
    -e CCACHE_DIR=/ccache -e RSYNC_SRC="$RSYNC_SRC" \
    -v "$(pwd)/$OUT:/out" \
    -v $VOL:/work \
    -v pm3ds-ccache:/ccache \
    $IMG sh -c '
    set -e -f
    t0=$(date +%s)
    stage() { now=$(date +%s); echo "STAGE $1 $((now - t0)) s" >&2; t0=$now; }
    if [ -n "$RSYNC_SRC" ]; then rsync -a -R --delete --files-from=/out/srclist.txt /host/ /src/; stage rsync; fi
    # Configure only when the build dir is missing, was made with ccache, or points at another source root.
    if ! grep -q "^CMAKE_C_COMPILER_LAUNCHER:STRING=ccache$" /work/build/CMakeCache.txt 2>/dev/null \
        || ! grep -q "^CMAKE_HOME_DIRECTORY:INTERNAL=/src/3ds$" /work/build/CMakeCache.txt 2>/dev/null; then
        cmake -G Ninja -S /src/3ds -B /work/build -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache
    fi
    stage configure
    status=0
    cmake --build /work/build -- -k 0 '"$*"' || status=$?
    stage compile-link
    cp -f /work/build/pm_3ds.3dsx /work/build/pm_3ds.smdh /work/build/pm_3ds.map /work/build/pm_3ds.elf /out/ 2>/dev/null || true
    stage copy-out
    exit $status
' || rc=$?
kill "$wdpid" 2>/dev/null || true
if [ "$rc" -ne 0 ]; then echo "BUILD FAILED exit $rc (container $NAME; stage timings above)" >&2; exit "$rc"; fi
# HW5: keep the ELF of each build under its build number, so a Luma crash dump can be decoded later (3ds/tools/decode_dump.py).
if [ -f $OUT/pm_3ds.elf ]; then
    mkdir -p 3ds/logs/builds
    cp -f $OUT/pm_3ds.elf "3ds/logs/builds/pm_3ds_$BN.elf"
    cp -f $OUT/pm_3ds.map "3ds/logs/builds/pm_3ds_$BN.map" 2>/dev/null || true
fi
# Azahar loads from its own SD card, not from build3ds/.
if [ -z "$PM_LANE" ] && [ -n "$APPDATA" ] && [ -f build3ds/pm_3ds.3dsx ]; then
    dest="$(cygpath -u "$APPDATA" 2>/dev/null || printf '%s' "$APPDATA")/Azahar/sdmc/3ds/PaperMario"
    mkdir -p "$dest"
    cp -f build3ds/pm_3ds.3dsx "$dest/pm_3ds.3dsx"
fi
