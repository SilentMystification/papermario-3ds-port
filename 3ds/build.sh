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
# WTASSETS: assets/us and ver/us/build are git-ignored ROM-derived inputs. A worktree has none: both come from one base,
# the main worktree (the parent of the git common dir), read only. No copy into the worktree. ASSET_BASE is "." when the
# worktree has both (the main worktree itself, or a folder with copies).
MAIN_ROOT=$(cd "$(git rev-parse --git-common-dir)/.." && pwd)
ASSET_BASE=.
if [ ! -d assets/us ] || [ ! -d ver/us/build ]; then ASSET_BASE=$MAIN_ROOT; fi
OUT=build3ds; VOL=pm3ds-build; SRCVOL=pm3ds-src
if [ -n "$PM_LANE" ]; then OUT=build3ds/lane-$PM_LANE; VOL=pm3ds-build-$PM_LANE; SRCVOL=pm3ds-src-$PM_LANE; fi
# HC0: PM_FNCOV=1 is the census lane <L>_fncov (needs PM_LANE): its own output folder and build volume, the draw TUs instrumented.
FNV=OFF
if [ "${PM_FNCOV:-0}" = 1 ]; then
    [ -n "$PM_LANE" ] || { echo "PM_FNCOV=1 needs PM_LANE" >&2; exit 1; }
    FNV=ON; OUT=build3ds/lane-${PM_LANE}_fncov; VOL=pm3ds-build-${PM_LANE}_fncov
fi
mkdir -p $OUT
# VOLGEN: the shared generator cache (3ds/tools/gencache.sh) and the content-id source sync (3ds/tools/src_sync.sh).
# PM_CACHE=off is the cold gate: no cache read or write, no skip, no ccache, a new build dir and a new asset volume.
# PM_CACHE=check (cachecheck, with PM_KEY_ONLY=1): the generators run cold and compare with their cache entries.
ASSET_DIRS="assets/us ver/us/build"
CODE_DIRS="3ds/CMakeLists.txt 3ds/effects.cmake 3ds/battle.cmake 3ds/maps.cmake 3ds/map_syms.lds LaunchIcon.png 3ds/ld 3ds/shaders 3ds/stub_include 3ds/include 3ds/src 3ds/tools/cov.d src include"
. 3ds/tools/gencache.sh
. 3ds/tools/src_sync.sh
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
# VOLGEN: a PM_BUILD_REV build (a committed tree) has no time tag, so a cold rebuild of a base gives the same bytes (cachecheck).
if [ -n "$PM_LANE" ] && [ -z "$PM_BUILD_REV" ]; then BN="${BN}_$(date +%H%M%S)$$"; fi

# MRTGEN: 3ds/src/map_restore_table.c and 3ds/ld/map_restore*.ld are generated from 3ds/map_restore.txt (git-ignored, never
# committed, never restored by hand). This runs before the key and the sync, so both see the new content. gen_map_restore.py
# writes a file only when its content changes.
gen_map_restore() {
    if command -v py >/dev/null 2>&1; then py -3 3ds/tools/gen_map_restore.py >/dev/null
    else python3 3ds/tools/gen_map_restore.py >/dev/null; fi || { echo "gen_map_restore.py failed" >&2; exit 1; }
}
# BUILDLOCK: the engine must answer (a dead engine stops here, not in a docker call), then at most 3 builds run at once.
# HOSTSLOT: the slot is taken before the generators (their docker runs are under the slot). A key-only call takes no slot.
if [ -z "$PM_KEY_ONLY" ]; then
. 3ds/tools/build_slot.sh
build_docker_ok || build_docker_restart || { echo "BUILD FAULT: docker engine not answering in 20 s (docker info), restart not done. Restart Docker Desktop / WSL, then build again." >&2; exit 3; }
build_slot_take
build_docker_ok || { build_slot_release; echo "BUILD FAULT: docker stopped answering after the slot was taken" >&2; exit 3; }
trap 'build_slot_release' EXIT
trap 'build_slot_release; exit 130' INT TERM
fi
gen_map_restore
IMG=pm3ds-build:1
# LINKFAST: docker build (FACT 1.4 s) only when the content ids of docker/ or the image id differ from the stamp.
DKEY=$(in_manifest docker | sha1sum | cut -c1-40); DSTAMP=$MAIN_ROOT/build3ds/img_build_stamp.txt
if [ "$(cat "$DSTAMP" 2>/dev/null)" != "$DKEY $(docker image inspect -f '{{.Id}}' $IMG 2>/dev/null)" ]; then
    docker build -q -t $IMG -f docker/Dockerfile.build docker >/dev/null
    mkdir -p "$MAIN_ROOT/build3ds"; printf '%s %s\n' "$DKEY" "$(docker image inspect -f '{{.Id}}' $IMG)" > "$DSTAMP.$$" && mv -f "$DSTAMP.$$" "$DSTAMP"
fi
IMGID=$(docker image inspect -f '{{json .RootFS.Layers}}' $IMG | sha1sum | cut -c1-40)
HIMGID=$(docker image inspect -f '{{json .RootFS.Layers}}' pm3ds-host:1 | sha1sum | cut -c1-40)
ROM=$ASSET_BASE/ver/us/baserom.z64; [ -f "$ROM" ] || { echo "missing $ROM" >&2; exit 1; }
tg=$(date +%s)
asset_manifest
ROMSHA=$(sha1sum < "$ROM" | cut -c1-40)
echo "STAGE asset-hash $(( $(date +%s) - tg )) s" >&2
tg=$(date +%s)

# TEXKEY2: 3ds/include/texkey_patch.h (git-ignored, generated) holds the palette fix of the map texture archives: the alpha-0
# key colours of each CI texture, computed from the ROM and the texture sources of ASSET_BASE (3ds/tools/texkey_bake.py, read only).
# nuPiReadRom (pm_os.c) applies it to the bytes the game reads. VOLGEN: cached (gencache texkey); the key below covers its content.
gen_texkey() {
    tkb=$ASSET_BASE; command -v cygpath >/dev/null 2>&1 && tkb=$(cygpath -m "$ASSET_BASE")
    if command -v py >/dev/null 2>&1; then PYX="py -3"; else PYX=python3; fi
    # Key inputs: the tool, the python version, the ROM, every asset file (the tool reads assets/us/mapfs/tex).
    tk_key="texkey v1
tool $(sha1sum < 3ds/tools/texkey_bake.py | cut -c1-40)
python $($PYX -c 'import sys; print(sys.version)')
rom $ROMSHA
assets $AHASH"
    if gc_get texkey "$tk_key" texkey_patch.h:3ds/include/texkey_patch.h; then echo "texkey_bake: cache hit"; return 0; fi
    $PYX 3ds/tools/texkey_bake.py "$tkb" 3ds/include/texkey_patch.h || { echo "texkey_bake.py failed" >&2; exit 1; }
    gc_put texkey "$tk_key" texkey_patch.h:3ds/include/texkey_patch.h || true
}
gen_texkey >&2 # PUSHKEY: stdout of build.sh is the key only (PM_KEY_ONLY)
# VOLGEN: the content ids of every code and tool file, read by the generator keys (gc_inputs). After texkey (texkey_patch.h is an input).
in_manifest $CODE_DIRS 3ds/tools > $OUT/in_now.txt
# GENSPEED: gen_gfx_tab and gen_pso_tab read a copy of their key folders (one tar file of this tree, unpacked in the container's
# own /work) and the assets from the shared asset volume (checked against its out.sha1 first: exit 7), not the C:\ bind mount.
# FACT (oracle timing 2026-11-16, gconv objects cached): gfx_tab 71 s on the bind mount (WSL2 9p), 2.4 s on ext4; pso_tab 28 s
# and 2.1 s. The container sees only the key folders, so a read outside the key fails the step. /w is mounted for the outputs.
GEN_AV='mkdir -p /work/ver/us; tar -xf - -C /work; (cd /a/c && sha1sum -c --status out.sha1) || { echo "ASSET VOLUME CHECK FAILED" >&2; exit 7; }; ln -s /a/c/assets /work/assets; ln -s /a/c/ver/us/build /work/ver/us/build'
# gen_run <name> <command> <key folder>...: makes the asset volume when it is missing (once per run), writes $OUT/gen_in.tar of
# the key folders and runs the command in the host image with the tar on stdin. Exit 7 removes the asset volume (as the build does).
gen_run() {
    gn=$1; gcmd=$2; shift 2
    if [ "$GEN_AOK" != 1 ]; then case "$(vol_state)" in *assets-ok*) ;; *) asset_vol_make ;; esac; GEN_AOK=1; fi
    tar -cf - --exclude=__pycache__ "$@" > $OUT/gen_in.tar || { rm -f $OUT/gen_in.tar; echo "$gn failed: tar of $*" >&2; exit 1; }
    w=$(pwd -W 2>/dev/null || pwd); grc=0
    MSYS_NO_PATHCONV=1 docker run --rm -i --memory "${PM_BUILD_MEM:-4g}" -v "$w:/w" -v $AVOL:/a:ro -v "pm3ds-gconvobj-$(docker image inspect -f '{{.Id}}' pm3ds-host:1 | cut -c8-23):/gobj" -e GCONV_OBJ=/gobj -e PM_CACHE="${PM_CACHE:-on}" -w /work pm3ds-host:1 sh -c "$gcmd" < $OUT/gen_in.tar || grc=$?
    rm -f $OUT/gen_in.tar
    if [ "$grc" -eq 7 ]; then docker volume rm $AVOL >/dev/null 2>&1 || true; echo "BUILD FAULT: the asset volume $AVOL failed its content check and was removed. Build again (the next build makes it new)." >&2; exit 7; fi
    [ "$grc" = 0 ] || { echo "$gn failed" >&2; exit 1; }
}

# HC4f1: host shape converter (3ds/tools/nd_shape_conv.c, host C, read only on ASSET_BASE). Writes $OUT/romfs/shape/<asset>.nds
# for every *_shape table entry: the Yay0 decode with pointer fields as shape offsets and their reloc list.
gen_shapes() {
    # Compiled and run in the host image pm3ds-host:1 (docker/Dockerfile.host), as nd_host_test.sh does. The ROM is a read only mount.
    rom=$ASSET_BASE/ver/us/baserom.z64; [ -f "$rom" ] || { echo "gen_shapes: missing $rom" >&2; exit 1; }
    w=$(pwd -W 2>/dev/null || pwd); od=$(cd "$OUT" && pwd -W 2>/dev/null || pwd)/romfs/shape
    rd=$(cd "$ASSET_BASE/ver/us" && pwd -W 2>/dev/null || pwd)
    # VOLGEN: cached (gencache shape). The lane marker $OUT/shape_entry.txt names the entry that $OUT/romfs/shape links to.
    # Key inputs: the command text, the host image, the ROM, the sources and every header the two compiles can read.
    SH_CMD='gcc -m32 -O2 -Wall -I /w/3ds/include -o /tmp/yay0_test /w/3ds/tools/yay0_test.c /w/3ds/src/nd_yay0.c && /tmp/yay0_test /ar && gcc -m32 -O2 -Wall -msse2 -mfpmath=sse -ffp-contract=off -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I /w/include -I /w/3ds/include -I /w/3ds/stub_include -o /tmp/nd_shape_conv /w/3ds/tools/nd_shape_conv.c /w/3ds/src/nd_seam.c /w/3ds/src/nd_dlc.c && /tmp/nd_shape_conv /ar /out'
    want="shape v1
cmd $SH_CMD
himg $HIMGID
rom $ROMSHA
$(gc_inputs 3ds/tools/nd_shape_conv.c 3ds/tools/yay0_test.c 3ds/src/nd_seam.c 3ds/src/nd_dlc.c 3ds/src/nd_yay0.c 3ds/include 3ds/stub_include include)"
    printf '%s' "$(printf '%s' "$want" | sha1sum | cut -c1-40)" > "$OUT/shape_key.txt" # the content key of the romfs shapes, in every cache mode (build key, 3dsx re-make)
    if gc_get_dir shape "$want" "$OUT/romfs/shape" "$OUT/shape_entry.txt"; then echo "gen_shapes: cache hit"; return 0; fi
    rm -rf "$OUT/romfs/shape" "$OUT/shape_entry.txt" # the old files can be hard links of a cache entry: a run writes new files
    mkdir -p "$OUT/romfs/shape"
    MSYS_NO_PATHCONV=1 docker run --rm --memory "${PM_BUILD_MEM:-4g}" -v "$w:/w:ro" -v "$od:/out" -v "$rd/baserom.z64:/ar/ver/us/baserom.z64:ro" -w /tmp pm3ds-host:1 sh -c "$SH_CMD" || { echo "gen_shapes failed" >&2; exit 1; }
    gc_put_dir shape "$want" "$OUT/romfs/shape" "$OUT/shape_entry.txt" || true
}
gen_shapes >&2 # PUSHKEY

# S7CONV: native tables of the static gfx lists (3ds/tools/gen_gfx_tab.sh, row S7CONV design note): 3ds/src/nd_gfx_tab.c (git-ignored, never
# committed, in the build key). Run in the host image by gen_run (GENSPEED: key folders copied, assets from the asset volume). Skip: the stamp
# holds the hashes of the tool, its headers and the gfx sources of the set; the file is replaced only when its content changes.
gen_gfx_tab() {
    # VOLGEN: cached (gencache gfx_tab). Key inputs: the command text, the host image, every asset file, and all of 3ds/tools,
    # 3ds/include, 3ds/stub_include, include, src (gen_gfx_tab.sh compiles with -I src -I include) and the 2 port sources.
    # GTIN (restored in VOLGEN 1: the patch dropped the GT_IN lines that gtin.sh checks). GT_IN is the key input list.
    GT_IN="3ds/tools 3ds/include 3ds/stub_include include"
    GT_IN="$GT_IN src 3ds/src/nd_scomp.c 3ds/src/nd_texwin.c" # GTIN: the own-TU sources of the set (under src)
    GT_CMD='
        set -e; '"$GEN_AV"'
        sh 3ds/tools/gen_gfx_tab.sh /w/3ds/src/nd_gfx_tab.c.new /tmp/gconv 2>&1 | grep -v "^gconv def"'
    want="gfx_tab v2
cmd $GT_CMD
himg $HIMGID
assets $AHASH
$(gc_inputs $GT_IN)"
    GT_OUT="nd_gfx_tab.c:3ds/src/nd_gfx_tab.c nd_ntab_descs.c:3ds/src/nd_ntab_descs.c ntab_inherit.txt:build3ds/out/ntab_inherit.txt ntab_unsup.txt:build3ds/out/ntab_unsup.txt ntab_tables.txt:build3ds/out/ntab_tables.txt ntab_entries.txt:build3ds/out/ntab_entries.txt ntab_pairs.txt:build3ds/out/ntab_pairs.txt gset_ok.txt:build3ds/out/gset_ok.txt"
    if gc_get gfx_tab "$want" $GT_OUT; then echo "gen_gfx_tab: cache hit"; return 0; fi
    mkdir -p "$OUT" build3ds/out # gen_gfx_tab.sh writes its logs to build3ds/out only when the folder exists
    rm -f 3ds/src/nd_gfx_tab.c.new
    gen_run gen_gfx_tab "$GT_CMD" $GT_IN
    [ -s 3ds/src/nd_gfx_tab.c.new ] || { echo "gen_gfx_tab failed: no output" >&2; exit 1; }
    if cmp -s 3ds/src/nd_gfx_tab.c.new 3ds/src/nd_gfx_tab.c; then rm -f 3ds/src/nd_gfx_tab.c.new; else mv -f 3ds/src/nd_gfx_tab.c.new 3ds/src/nd_gfx_tab.c; fi
    gc_put gfx_tab "$want" $GT_OUT || true
}
# NPSO3: the build-time pipeline table (3ds/tools/gen_pso_tab.py): every ND_PSO_DESC of src and 3ds/src decided once on the host
# in the host image, written to 3ds/src/nd_pso_tab.c (git-ignored, in the build key). Skip: the stamp holds the hashes of the two
# tools and the sources of the scan; the file is replaced only when its content changes.
gen_pso_tab() {
    # VOLGEN: cached (gencache pso_tab). Key inputs: the command text, the host image, every asset file, all of 3ds/tools,
    # 3ds/include, 3ds/stub_include, include, src, 3ds/src, and the 2 gen_gfx_tab outputs in 3ds/src (written just before).
    PT_IN="3ds/tools 3ds/include 3ds/stub_include include src 3ds/src" # GENSPEED: the key folders, also the tar list of gen_run
    PT_CMD='
        set -e; '"$GEN_AV"'
        python3 3ds/tools/gen_pso_tab.py /w/3ds/src/nd_pso_tab.c.new src 3ds/src'
    want="pso_tab v2
cmd $PT_CMD
himg $HIMGID
assets $AHASH
gfxtab $(sha1sum < 3ds/src/nd_gfx_tab.c | cut -c1-40) $(sha1sum < 3ds/src/nd_ntab_descs.c | cut -c1-40)
$(gc_inputs $PT_IN)"
    if gc_get pso_tab "$want" nd_pso_tab.c:3ds/src/nd_pso_tab.c; then echo "gen_pso_tab: cache hit"; return 0; fi
    mkdir -p "$OUT"
    rm -f 3ds/src/nd_pso_tab.c.new
    gen_run gen_pso_tab "$PT_CMD" $PT_IN
    [ -s 3ds/src/nd_pso_tab.c.new ] || { echo "gen_pso_tab failed: no output" >&2; exit 1; }
    if cmp -s 3ds/src/nd_pso_tab.c.new 3ds/src/nd_pso_tab.c; then rm -f 3ds/src/nd_pso_tab.c.new; else mv -f 3ds/src/nd_pso_tab.c.new 3ds/src/nd_pso_tab.c; fi
    gc_put pso_tab "$want" nd_pso_tab.c:3ds/src/nd_pso_tab.c || true
}
gen_gfx_tab >&2 # PUSHKEY
gen_pso_tab >&2 # NPSO3
echo "STAGE gen $(( $(date +%s) - tg )) s" >&2
if [ "${PM_CACHE:-on}" = check ]; then
    if [ "$GC_FAIL" = 0 ]; then echo "CACHECHECK gens ok" >&2; else echo "CACHECHECK gens FAIL" >&2; exit 1; fi
fi

# The inputs CMake reads (3ds/CMakeLists.txt, DECOMP = repo root). Nothing else reaches the build.
# CODE_DIRS are read from this worktree; ASSET_DIRS from ASSET_BASE (see above).
for p in $CODE_DIRS; do
    [ -e "$p" ] || { echo "missing build input: $p" >&2; exit 1; }
done
for p in $ASSET_DIRS; do
    [ -e "$ASSET_BASE/$p" ] || { echo "missing build input: $ASSET_BASE/$p (assets come from the main worktree)" >&2; exit 1; }
done
# BSTALE: skip the build when the inputs have the same content as at the last successful build of this output folder.
# The key covers the tracked inputs by git tree id and by their uncommitted diff, the untracked files by content, the
# docker recipe, and the size and mtime of the ignored inputs (assets/us, ver/us/build: not in git). pm_build.h is not
# in the key: a commit that only changes the build number does not rebuild. A PM_BUILD_REV build (bench build-at) always builds.
# The skip also needs the source volume to carry the id of the last sync (SRCID, below): a recreated volume rebuilds.
# build.sh is the only stale check: go.ps1 Invoke-Build and push_3ds.sh call it and never compare times themselves.
KEY_IN="3ds/CMakeLists.txt 3ds/effects.cmake 3ds/battle.cmake 3ds/maps.cmake 3ds/map_syms.lds LaunchIcon.png 3ds/ld 3ds/shaders 3ds/stub_include 3ds/include 3ds/src 3ds/tools/cov.d src include docker/Dockerfile.build 3ds/build.sh"
KEY_FILE=$OUT/build_key.txt
key_now() {
    {
        git ls-tree -r HEAD -- $KEY_IN
        git diff HEAD -- $KEY_IN
        git ls-files -o --exclude-standard -- $KEY_IN | while read -r f; do sha1sum "$f"; done
        sha1sum 3ds/src/map_restore_table.c 3ds/src/nd_gfx_tab.c 3ds/src/nd_pso_tab.c 3ds/ld/map_restore.ld 3ds/ld/map_restore_bss.ld 3ds/include/texkey_patch.h
        cat $OUT/asset_now.txt; cat $OUT/shape_key.txt # asset content ids and the content key of the romfs shapes (not the cache marker: it exists only with the cache on)
        echo "img $IMGID $HIMGID"
    } | sha1sum | cut -d' ' -f1
}
# TOOLB1: PM_KEY_ONLY=1 prints the key of the tree (bench.py build_stale compares it with build_key.txt) and stops.
if [ -n "$PM_KEY_ONLY" ]; then key_now; if [ "${PM_CACHE:-on}" = off ]; then docker volume rm $AVOL >/dev/null 2>&1 || true; fi; exit 0; fi # GENSPEED: gen_run can make the cold asset volume
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
# SRCID: the source volume keeps the id of its last sync in /src/.pm_manifest_id, and $VID holds the id the manifest was
# written with. The manifest is valid only while both match. A missing, recreated or foreign source volume has no id
# (or another one): the manifest is dropped and every file is copied. Also the skip check needs the match (BSTALE).
# VOLGEN: vol_state (src_sync.sh) reads the source volume id and the asset volume key in one container.
VS=$(vol_state); VOLID=$(printf '%s\n' "$VS" | sed -n 1p); AOK=$(printf '%s\n' "$VS" | grep -c '^assets-ok$' || true)
# Cache safety: the skip also needs the 3dsx to match the hash written with the key ($OUT/build_out.sha1). PM_CACHE=off never skips.
if [ -z "$PM_BUILD_REV" ]; then
    KEY=$(key_now)
    if [ "${PM_CACHE:-on}" = on ] && [ -f $OUT/pm_3ds.3dsx ] && [ "$(cat $KEY_FILE 2>/dev/null)" = "$KEY" ] && vol_match && [ "$AOK" = 1 ] \
        && [ "$(cat $OUT/build_out.sha1 2>/dev/null)" = "$(sha1sum < $OUT/pm_3ds.3dsx | cut -c1-40)" ]; then
        echo "--- build up to date (key $(echo "$KEY" | cut -c1-12)), skipping ---"
        publish_3dsx
        memcheck_line
        [ "$FNV" = OFF ] && hotset_line
        exit 0
    fi
fi
# pm_build.h describes the 3dsx that is built (push_3ds.sh archives from it, bench.py checks its mtime), so it is written
# here, after the skip check: a skipped build keeps the header of the 3dsx it kept. Rewritten only when the number changed,
# since a new mtime would rebuild every file that includes it. The tmp name is per process: two pushes at once shared one name.
PMB_TMP=3ds/include/pm_build.h.tmp.$$
printf '#define PM_BUILD_STR "%s"\n' "$BN" > "$PMB_TMP"
printf '#define PM_GIT_HASH "%s"\n' "$(git rev-parse --short "$REV")" >> "$PMB_TMP"
cmp -s "$PMB_TMP" 3ds/include/pm_build.h && rm -f "$PMB_TMP" || mv -f "$PMB_TMP" 3ds/include/pm_build.h
# A failed build must not leave the old key behind: the key file is written only after a successful build.
rm -f $KEY_FILE $OUT/build_out.sha1

# BENCH4: a build of another commit drops the shared source volume first, so no file of the main folder stays in it. The
# main sync compares against its own manifest, so the volume is dropped again after the build (the next main sync copies all).
if [ -n "$PM_BUILD_REV" ]; then docker volume rm $SRCVOL >/dev/null 2>&1 || true; VOLID=""; fi
# Sync (VOLGEN, 3ds/tools/src_sync.sh): copy only the code files whose content id changed; the assets come from the shared volume.
t0=$(date +%s)
src_sync
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
# VOLGEN: the asset volume at /a (read only, checked against its out.sha1 first: exit 7 on a mismatch). The ccache volume is
# keyed by the build image id. A build dir of another image, or PM_CACHE=off, starts empty. A new shape entry re-makes the 3dsx.
MSYS_NO_PATHCONV=1 docker run --rm --memory "${PM_BUILD_MEM:-4g}" --name "$NAME" -v $SRCVOL:/src:ro -v $AVOL:/a:ro \
    -e CCACHE_DIR=/ccache -e CCACHE_MAXSIZE="${PM_CCACHE_MAX:-2G}" -e PM_BUILD_NUM="$BN" -e PM_FNCOV_V="$FNV" \
    -e IMGID="$IMGID" -e PM_CACHE_V="${PM_CACHE:-on}" -e SHAPE_E="$(cat $OUT/shape_key.txt)" \
    -v "$(pwd)/$OUT:/out" \
    -v $VOL:/work \
    -v pm3ds-ccache-$(printf '%s' "$IMGID" | cut -c1-12):/ccache \
    $IMG sh -c '
    set -e -f
    t0=$(date +%s)
    stage() { now=$(date +%s); echo "STAGE $1 $((now - t0)) s" >&2; t0=$now; }
    if ! (cd /a/c && sha1sum -c --status out.sha1); then echo "ASSET VOLUME CHECK FAILED" >&2; exit 7; fi
    stage asset-check
    if [ "$(cat /work/.img 2>/dev/null)" != "$IMGID" ] || [ "$PM_CACHE_V" = off ]; then rm -rf /work/build; printf %s "$IMGID" > /work/.img; fi
    if [ "$PM_CACHE_V" = off ]; then export CCACHE_DISABLE=1; fi
    if [ "$(cat /work/.shape 2>/dev/null)" != "$SHAPE_E" ]; then rm -f /work/build/pm_3ds.3dsx; printf %s "$SHAPE_E" > /work/.shape; fi
    # LINKFAST: the romfs in the build volume. Key: the shape content key. Every build checks the files (romfs.sha1); a new key,
    # a failed check or PM_CACHE=off copies it again (temp folder, check against /out/romfs, one mv) and removes the 3dsx.
    if [ "$PM_CACHE_V" = off ] || [ "$(cat /work/.romfs_key 2>/dev/null)" != "$SHAPE_E" ] || ! (cd /work/romfs 2>/dev/null && sha1sum -c --status /work/romfs.sha1 2>/dev/null); then
        rm -rf /work/romfs /work/romfs.tmp /work/romfs.sha1 /work/.romfs_key /work/build/pm_3ds.3dsx; mkdir -p /work/romfs.tmp
        cp -a /out/romfs/. /work/romfs.tmp/
        (cd /work/romfs.tmp && find . -type f | LC_ALL=C sort | xargs -r -d "\n" sha1sum) > /work/romfs.sha1
        (cd /out/romfs && sha1sum -c --status /work/romfs.sha1) || { echo "ROMFS COPY CHECK FAILED" >&2; exit 1; }
        mv -T /work/romfs.tmp /work/romfs; printf %s "$SHAPE_E" > /work/.romfs_key
    fi
    stage romfs
    # Configure only when the build dir is missing, was made with ccache, points at another source root, or has another build number (the smdh description).
    if ! grep -q "^CMAKE_C_COMPILER_LAUNCHER:STRING=ccache$" /work/build/CMakeCache.txt 2>/dev/null \
        || ! grep -q "^PM_FNCOV:BOOL=$PM_FNCOV_V\$" /work/build/CMakeCache.txt 2>/dev/null \
        || ! grep -q "^CMAKE_HOME_DIRECTORY:INTERNAL=/src/3ds$" /work/build/CMakeCache.txt 2>/dev/null \
        || ! grep -q "^PM_BUILD_NUM:STRING=$PM_BUILD_NUM\$" /work/build/CMakeCache.txt 2>/dev/null; then
        cmake -G Ninja -S /src/3ds -B /work/build -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/3DS.cmake -DCMAKE_C_COMPILER_LAUNCHER=ccache -DPM_BUILD_NUM=$PM_BUILD_NUM -DPM_FNCOV=$PM_FNCOV_V
    fi
    stage configure
    status=0
    cmake --build /work/build --parallel "${PM_BUILD_JOBS:-8}" -- -k 0 '"$*"' || status=$?
    stage compile-link
    cp -f /work/build/pm_3ds.3dsx /work/build/pm_3ds.smdh /work/build/pm_3ds.map /work/build/pm_3ds.elf /out/ 2>/dev/null || true
    # The objects stay: ninja then rebuilds only the sources whose mtime is newer (BSTALE). ccache is the cache for a clean build.
    stage copy-out
    exit $status
' || rc=$?
kill "$wdpid" 2>/dev/null || true
if [ -n "$PM_BUILD_REV" ]; then docker volume rm $SRCVOL >/dev/null 2>&1 || true; fi
if [ "${PM_CACHE:-on}" = off ]; then docker volume rm $AVOL >/dev/null 2>&1 || true; fi
if [ "$rc" -eq 7 ]; then docker volume rm $AVOL >/dev/null 2>&1 || true; echo "BUILD FAULT: the asset volume $AVOL failed its content check and was removed. Build again (the next build makes it new)." >&2; exit 7; fi
if [ "$rc" -ne 0 ]; then echo "BUILD FAILED exit $rc (container $NAME; stage timings above; memory limit ${PM_BUILD_MEM:-4g}, ${PM_BUILD_JOBS:-8} jobs: a \"Killed signal\" above is an out-of-memory kill)" >&2; exit "$rc"; fi
if [ -z "$PM_BUILD_REV" ]; then sha1sum < $OUT/pm_3ds.3dsx | cut -c1-40 > $OUT/build_out.sha1; echo "$KEY" > $KEY_FILE; fi
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
# LINKFAST: one docker volume inspect for all candidates (FACT oracle FASTLOOP: about 0.7 s per inspect call, 11 calls 7.9 s).
VL=$(docker volume ls -q | sed -n -E 's/^pm3ds-build-(.+)$/\1/p' | while read -r l; do printf '%s\n' "$LIVE" | grep -qx -- "$l" || echo "pm3ds-build-$l"; done)
[ -z "$VL" ] || docker volume inspect --format '{{.CreatedAt}} {{.Name}}' $VL | sed 's/ pm3ds-build-/ /' | sort -r | awk '{print $2}' | {
    n=0
    while read -r l; do
        n=$((n + 1))
        if [ "$n" -gt "$KEEP" ] && [ "$l" != "${PM_LANE:-}" ]; then
            docker volume rm pm3ds-build-$l pm3ds-src-$l pm3ds-work-$l >/dev/null 2>&1 || true
        fi
    done
}
