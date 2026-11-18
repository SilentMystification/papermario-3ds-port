#!/bin/sh
# SRCSYNC (VOLGEN): the content manifests and the shared asset volume. Sourced by 3ds/build.sh (VOLHOST: also by
# 3ds/tools/nd_host_test.sh). Needs: OUT, IMG, ASSET_BASE, ASSET_DIRS, MAIN_ROOT (asset_manifest sets AHASH and AVOL).
# The assets live in one shared volume per asset content, pm3ds-assets-<16 hex>, linked into /src by two symlinks
# (/src/assets/us, /src/ver/us/build -> /a/c/...). Every id here is a content hash (plan_buildspeed.md "Cache safety").
# BUILDO: the source volumes belong to the build slots (3ds/build.sh ws_sync), not to the lanes. Self-test: 3ds/tests/srcsync/run.sh.

# in_manifest <path>...: "path<TAB>id" of every file under the paths, sorted. id b:<blob> for a tracked file equal to the
# index (git ls-files -s), h:<git hash-object --no-filters> for every other file (modified, untracked, ignored).
# PM_BUILD_REV (an archive of a commit): a file of the commit tree gets its blob id b:, as in a worktree build (git archive writes the bytes of a checkout); other files get h:.
in_manifest() {
    im=$OUT/.inm.$$
    : > $im.b; : > $im.h; : > $im.d
    if [ -n "$PM_BUILD_REV" ]; then
        find "$@" -type f | grep -v -E '(^|/)__pycache__/|\.pyc$|\.tmp\.' | LC_ALL=C sort > $im.all
        git -c core.quotepath=off ls-tree -r "$PM_BUILD_REV" -- "$@" | awk '$2 == "blob" { i = index($0, "	"); print substr($0, i + 1) "	b:" $3 }' > $im.t
        awk -F'	' 'FILENAME == ARGV[1] { have[$1] = 1; next } ($1 in have)' $im.all $im.t > $im.b
        awk -F'	' 'FILENAME == ARGV[1] { t[$1] = 1; next } !($1 in t)' $im.b $im.all > $im.h
        rm -f $im.all $im.t
    else
        git -c core.quotepath=off ls-files -s -- "$@" | awk '{ i = index($0, "\t"); print substr($0, i + 1) "\tb:" $2 }' > $im.b
        git -c core.quotepath=off ls-files -m -o -- "$@" | grep -v -E '(^|/)__pycache__/|\.pyc$|\.tmp\.' | LC_ALL=C sort -u > $im.mo || true
        while IFS= read -r f; do if [ -f "$f" ]; then printf '%s\n' "$f" >> $im.h; else printf '%s\n' "$f" >> $im.d; fi; done < $im.mo
    fi
    if [ -s $im.h ]; then git hash-object --no-filters --stdin-paths < $im.h | paste $im.h - | sed 's/\t/\th:/' > $im.hh; else : > $im.hh; fi
    awk -F'\t' 'FILENAME == ARGV[1] { del[$1] = 1; next } { id[$1] = $2 } END { for (p in id) if (!(p in del)) print p "\t" id[p] }' \
        $im.d $im.b $im.hh | LC_ALL=C sort
    rm -f $im.b $im.mo $im.h $im.d $im.hh
}

# asset_manifest: writes $OUT/asset_now.txt ("path<TAB>h:<blob>" of every asset file, sorted) and sets AHASH (its sha1) and
# AVOL (the shared asset volume of this content). PM_CACHE=off: AVOL is a new volume of this run (removed at the end).
asset_manifest() {
    oa=$(cd "$OUT" && pwd)
    # LOOP2: one python process with 8 hash threads (3ds/tools/asset_ids.py; the same bytes as find | sort | git hash-object,
    # asset_ids.py --check): FACT 3.1 s against 4.1 s on 18426 files.
    # BUILDO: a stat cache per asset base (asset_ids.py PM_ASSET_STAT, the git index rule; PM_CACHE=off hashes every file).
    if command -v py >/dev/null 2>&1; then aip="py -3"; else aip=python3; fi
    ast=$MAIN_ROOT/build3ds/asset_stat_$(printf '%s' "$(cd "$ASSET_BASE" && pwd)" | sha1sum | cut -c1-8).txt
    [ -n "$MAIN_ROOT" ] || ast=
    PM_ASSET_STAT=$ast $aip 3ds/tools/asset_ids.py "$ASSET_BASE" "$oa/asset_paths.txt" "$oa/asset_now.txt" $ASSET_DIRS
    [ -s "$OUT/asset_now.txt" ] || { echo "asset_manifest: no asset files under $ASSET_BASE ($ASSET_DIRS)" >&2; exit 1; }
    AHASH=$(sha1sum < "$OUT/asset_now.txt" | cut -c1-40)
    AVOL=pm3ds-assets-$(printf '%s' "$AHASH" | cut -c1-16)
    if [ "${PM_CACHE:-on}" = off ]; then AVOL=pm3ds-assets-cold$$; fi
}

# asset_vol_make: fills the asset volume once: the files, key.txt and out.sha1 go to a temp folder that one mv names /a/c.
asset_vol_make() {
    u="t$$_$(date +%s)"
    (cd "$ASSET_BASE" && tar -cf - $ASSET_DIRS) | MSYS_NO_PATHCONV=1 docker run --rm -i -v $AVOL:/a -e U=$u -e K=$AHASH $IMG sh -c '
        set -e
        if [ -d /a/c ] && [ "$(cat /a/c/key.txt 2>/dev/null)" != "$K" ]; then mv /a/c /a/bad.$U; rm -rf /a/bad.$U; fi
        mkdir -p /a/$U/c; tar -xf - -C /a/$U/c
        (cd /a/$U/c && find assets ver -type f | LC_ALL=C sort | xargs -d "\n" sha1sum) > /a/$U/c/out.sha1
        printf %s "$K" > /a/$U/c/key.txt
        mv -T /a/$U/c /a/c 2>/dev/null || true
        rm -rf /a/$U'
    # The assets must not change during the copy: a second hash after it, a change removes the volume.
    a1=$AHASH; asset_manifest
    if [ "$a1" != "$AHASH" ]; then docker volume rm $AVOL >/dev/null 2>&1; echo "asset files changed during the copy: build again" >&2; exit 1; fi
    echo "asset volume $AVOL made" >&2
}
