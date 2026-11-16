#!/bin/sh
# GFXINC gate tool: runs a gen_gfx_tab.sh script in pm3ds-host:1 with the nd_host_test.sh layout and writes its 8 outputs to <out dir>.
# Usage (repo root): sh 3ds/tests/gfxinc/run_gen.sh <script> <out dir> <cache volume | none>
# PM_CACHE (on, off, check) goes to the container. Prints "run_gen: <s> s rc <rc>" last; the gconvobj lines are in <out dir>/log.txt.
S=$1; O=$2; V=$3
ROOT=$(pwd -W 2>/dev/null || pwd)
AR=$ROOT
if [ ! -d assets/us ] || [ ! -d ver/us/build ]; then AR=$(cd "$(git rev-parse --git-common-dir)/.." && pwd -W 2>/dev/null || pwd); fi
rm -rf "$O"; mkdir -p "$O"
CV=""; [ "$V" != none ] && CV="-v $V:/gobj -e GCONV_OBJ=/gobj"
t0=$(date +%s)
MSYS_NO_PATHCONV=1 docker run --rm --memory "${PM_BUILD_MEM:-4g}" -v "$ROOT:/w" -v "$AR/assets:/ar/assets:ro" -v "$AR/ver/us/build:/ar/ver/us/build:ro" \
    $CV -e PM_CACHE="${PM_CACHE:-on}" -e S="$S" -e O="$O" -w /work pm3ds-host:1 sh -c '
    set -e; mkdir -p /work/ver/us /tmp/o
    for d in 3ds include src; do ln -s "/w/$d" "/work/$d"; done
    ln -s /ar/assets /work/assets; ln -s /ar/ver/us/build /work/ver/us/build
    sh "/w/$S" /tmp/o/nd_gfx_tab.c /tmp/o
    for f in nd_gfx_tab.c nd_ntab_descs.c ntab_inherit.txt ntab_unsup.txt ntab_tables.txt ntab_entries.txt ntab_pairs.txt gset_ok.txt; do cp /tmp/o/$f "/w/$O/$f"; done' > "$O/log.txt" 2>&1
rc=$?
echo "run_gen: $(( $(date +%s) - t0 )) s rc $rc"
exit $rc
