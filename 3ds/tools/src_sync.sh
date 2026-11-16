#!/bin/sh
# SRCSYNC (VOLGEN): the content manifests and the source volume of a lane. Sourced by 3ds/build.sh (VOLHOST: also by
# 3ds/tools/nd_host_test.sh). Needs: OUT, IMG, SRCVOL, CODE_DIRS, ASSET_BASE, ASSET_DIRS (asset_manifest sets AHASH and AVOL).
# The lane source volume holds the code only. The assets live in one shared volume per asset content, pm3ds-assets-<16 hex>,
# linked into /src by two symlinks (/src/assets/us, /src/ver/us/build -> /a/c/...). Every id here is a content hash, never a
# time or a size (plan_buildspeed.md "Cache safety"). Host self-test: 3ds/tests/srcsync/run.sh.

# in_manifest <path>...: "path<TAB>id" of every file under the paths, sorted. id b:<blob> for a tracked file equal to the
# index (git ls-files -s), h:<git hash-object --no-filters> for every other file (modified, untracked, ignored).
# PM_BUILD_REV (an archive folder with the GIT_DIR of the main folder: the index is not this tree): every file gets h:.
in_manifest() {
    im=$OUT/.inm.$$
    : > $im.b; : > $im.h; : > $im.d
    if [ -n "$PM_BUILD_REV" ]; then
        find "$@" -type f | grep -v -E '(^|/)__pycache__/|\.pyc$|\.tmp\.' | LC_ALL=C sort > $im.h
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
    (cd "$ASSET_BASE" && find $ASSET_DIRS -type f | LC_ALL=C sort > "$oa/asset_paths.txt" \
        && git hash-object --no-filters --stdin-paths < "$oa/asset_paths.txt" | paste "$oa/asset_paths.txt" - > "$oa/asset_now.txt")
    [ -s "$OUT/asset_now.txt" ] || { echo "asset_manifest: no asset files under $ASSET_BASE ($ASSET_DIRS)" >&2; exit 1; }
    AHASH=$(sha1sum < "$OUT/asset_now.txt" | cut -c1-40)
    AVOL=pm3ds-assets-$(printf '%s' "$AHASH" | cut -c1-16)
    if [ "${PM_CACHE:-on}" = off ]; then AVOL=pm3ds-assets-cold$$; fi
}

# vol_state: one container reads the source volume id (line 1) and prints "assets-ok" when the asset volume holds this key.
vol_state() {
    MSYS_NO_PATHCONV=1 docker run --rm -v $SRCVOL:/src -v $AVOL:/a -e K=$AHASH $IMG sh -c \
        'cat /src/.pm_manifest_id 2>/dev/null; echo; [ -d /a/c ] && [ "$(cat /a/c/key.txt 2>/dev/null)" = "$K" ] && echo assets-ok; true'
}
# vol_match: the manifest is valid only while the volume id and $OUT/src_volid.txt match (g tag: a content-id sync, VOLGEN).
vol_match() { case "$VOLID" in g*) ;; *) return 1 ;; esac; [ "$VOLID" = "$(cat $OUT/src_volid.txt 2>/dev/null)" ]; }

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

# src_sync: makes the lane source volume equal to the code manifest. Sets SYNCID, writes $OUT/src_manifest2.txt.
src_sync() {
    MAN=$OUT/src_manifest2.txt
    # No id match: a new volume (a volume of an older sync can hold files that the manifest does not name).
    if vol_match; then SYNCID=$VOLID; else rm -f $MAN; SYNCID="g$(date +%s)_$$"; docker volume rm $SRCVOL >/dev/null 2>&1 || true; fi
    [ -f $MAN ] || : > $MAN
    [ "$AOK" = 1 ] || asset_vol_make
    in_manifest $CODE_DIRS > $OUT/src_now.txt
    awk -F'\t' 'FILENAME==ARGV[1]{old[$1]=$2; next} (old[$1] != $2){print $1}' $MAN $OUT/src_now.txt > $OUT/src_changed.txt
    awk -F'\t' 'FILENAME==ARGV[1]{now[$1]=1; next} !($1 in now){print $1}' $OUT/src_now.txt $MAN > $OUT/src_deleted.txt
    nchg=$(wc -l < $OUT/src_changed.txt); ndel=$(wc -l < $OUT/src_deleted.txt)
    if [ -s $OUT/src_changed.txt ]; then
        # STALEOBJ: -m stamps each copied file with the copy time, so it is newer than every object in the build volume.
        tar -cf - -T $OUT/src_changed.txt | MSYS_NO_PATHCONV=1 docker run --rm -i -v $SRCVOL:/src -e S="$SYNCID" $IMG sh -c '
            tar -xmf - -C /src
            for p in assets/us ver/us/build; do [ -L /src/$p ] || rm -rf /src/$p; mkdir -p /src/${p%/*}; ln -sfn /a/c/$p /src/$p; done
            printf %s "$S" > /src/.pm_manifest_id'
    fi
    if [ -s $OUT/src_deleted.txt ]; then
        MSYS_NO_PATHCONV=1 docker run --rm -i -v $SRCVOL:/src $IMG sh -c 'while read -r f; do rm -f "/src/$f"; done' < $OUT/src_deleted.txt
    fi
    cp -f $OUT/src_now.txt $MAN
    echo "$SYNCID" > $OUT/src_volid.txt
}
