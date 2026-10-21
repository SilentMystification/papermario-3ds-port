#!/bin/sh
# Build the 3DS port inside the devkitpro/devkitarm Docker image.
# Usage (from repo root): sh 3ds/build.sh [extra ninja args]
# Output: build3ds/pm_3ds.3dsx, also copied to %APPDATA%\Azahar\sdmc\3ds\PaperMario\
#
# Sources are rsynced into a Docker volume and compiled there. Compiling
# straight from a Windows bind mount is I/O-bound (~10% CPU use); the volume
# is a native Linux filesystem, so ninja can use every core.
set -e
cd "$(dirname "$0")/.."
# PM_LANE (empty = main lane): own Docker volume and own output folder, no copy to the real SD card.
OUT=build3ds; VOL=pm3ds-work
if [ -n "$PM_LANE" ]; then OUT=build3ds/lane-$PM_LANE; VOL=pm3ds-work-$PM_LANE; fi
mkdir -p $OUT
# BN1: one build number = V0 + (commit count - C0), V0 and C0 in 3ds/build_base.txt. No -dirty suffix (user decision 2026-10-19). Suffix -<lane> (PM_LANE set) or -laneB (branch lane-b).
# Written to 3ds/include/pm_build.h (ignored by git). The boot log and the Options panel show PM_BUILD_STR.
read V0 C0 <<EOF
$(tr -d '\015' < 3ds/build_base.txt)
EOF
BN=$((V0 + $(git rev-list --count HEAD) - C0))
if [ -n "$PM_LANE" ]; then BN="$BN-$PM_LANE"; elif [ "$(git branch --show-current)" = "lane-b" ]; then BN="$BN-laneB"; fi
printf '#define PM_BUILD_STR "%s"
' "$BN" > 3ds/include/pm_build.h
MSYS_NO_PATHCONV=1 docker run --rm \
    -v "$(pwd):/host:ro" \
    -v "$(pwd)/$OUT:/out" \
    -v $VOL:/work \
    devkitpro/devkitarm:latest sh -c '
    set -e
    rsync -a --delete --exclude .git --exclude build3ds --exclude reference --exclude orig --exclude "pc/build*" /host/ /work/src/
    cmake -G Ninja -S /work/src/3ds -B /work/build -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake
    status=0
    cmake --build /work/build -- -k 0 '"$*"' || status=$?
    cp -f /work/build/pm_3ds.3dsx /work/build/pm_3ds.smdh /work/build/pm_3ds.map /work/build/pm_3ds.elf /out/ 2>/dev/null || true
    exit $status
'
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
