#!/bin/sh
# wt_gc.sh: remove the build3ds disk use of finished work.
# Removes, for every worktree under build3ds/wt whose branch is merged into main (or whose HEAD is detached):
#   the worktree (git worktree remove --force), its lane folder build3ds/lane-<name>, and its docker volumes
#   pm3ds-src-<name> and pm3ds-build-<name>.
#   its pm_3ds_<name>.3dsx copies (the Azahar sdmc folder and build3ds/azslots/slot*/user/sdmc/3ds/PaperMario)
#   and its shot folders build3ds/azt/shots/*/<name>.
# Also removes build-at folders build3ds/at_* and lanes build3ds/lane-* that have no worktree, except a build-at folder
# whose .build_at_in_use marker is under 12 h old (a build-at is running on it).
# In kept worktrees, deletes the git-ignored copies assets/us and ver/us/build when the main worktree has them (MB printed).
# Keeps: worktrees with a .wt_keep file (an agent works there; it deleted core20 mid-row on 2026-11-08) or uncommitted
# tracked changes, worktrees whose branch has commits not in main (unmerged work), worktrees whose branch tip is on the
# first-parent line of main (a fresh branch with no commit of its own yet: an agent may work in it; ls3 was
# deleted that way on 2026-11-07; remove a fast-forwarded one with --only), names given with --keep, the main lane
# (build3ds itself), build3ds/azslots, build3ds/azt, build3ds/archive.
# Before a removal, the uncommitted diff of a dirty worktree is saved to build3ds/archive/wt_<name>_dirty.patch,
# and its untracked *.patch / *.diff files are copied to build3ds/archive/.
# Usage: sh 3ds/tools/wt_gc.sh            (dry run: prints what it would remove)
#        sh 3ds/tools/wt_gc.sh --apply    (removes)
#        sh 3ds/tools/wt_gc.sh --apply --keep fb4,ub1
#        sh 3ds/tools/wt_gc.sh --apply --only tdc   (only these names: other worktrees, stale folders, lanes and
#                                                    build-at folders are not touched; skips the first-parent check)
set -u
APPLY=0; KEEP=","; ONLY=""
while [ $# -gt 0 ]; do
    case "$1" in
        --apply) APPLY=1; shift ;;
        --dry-run) APPLY=0; shift ;;
        --keep) KEEP=",$2,"; shift 2 ;;
        --only) ONLY=",$2,"; shift 2 ;;
        *) echo "unknown option $1"; exit 2 ;;
    esac
done
cd "$(git rev-parse --show-toplevel)" || exit 1
mkdir -p build3ds/archive
run() { if [ $APPLY = 1 ]; then "$@"; else echo "would: $*"; fi; }
# CLEANB: a removed worktree or lane takes its Azahar copies and shot folders with it (one place for this).
drop_lane_volumes() {
    for v in "pm3ds-src-$1" "pm3ds-build-$1"; do
        if docker volume inspect "$v" >/dev/null 2>&1; then echo "remove volume $v"; run docker volume rm "$v"; fi
    done
}
drop_lane_copies() {
    n=$1
    az=""
    [ -n "${APPDATA:-}" ] && az="$(cygpath -u "$APPDATA" 2>/dev/null)/Azahar/sdmc/3ds/PaperMario"
    for d in "$az" build3ds/azslots/slot*/user/sdmc/3ds/PaperMario; do
        f="$d/pm_3ds_$n.3dsx"
        if [ -n "$d" ] && [ -f "$f" ]; then echo "remove copy $f"; run rm -f -- "$f"; fi
    done
    for s in build3ds/azt/shots/*/"$n"; do
        if [ -d "$s" ]; then echo "remove shots $s"; run rm -rf -- "$s"; fi
    done
}
keep_wt=","
for w in build3ds/wt/*/; do
    [ -d "$w" ] || continue
    n=$(basename "$w")
    # --only: every other folder is out of scope (stale folders too), not kept.
    if [ -n "$ONLY" ]; then case "$ONLY" in *",$n,"*) ;; *) continue ;; esac; fi
    case "$KEEP" in *",$n,"*) keep_wt="$keep_wt$n,"; echo "keep $n (--keep)"; continue ;; esac
    if [ -e "$w.wt_keep" ]; then keep_wt="$keep_wt$n,"; echo "keep $n (.wt_keep: an agent works here)"; continue; fi
    case "$(git -C "$w" rev-parse --absolute-git-dir 2>/dev/null)" in
        */worktrees/*) ;;
        *) echo "remove stale folder $n (not a registered worktree)"; run rm -rf -- "build3ds/wt/$n"; continue ;;
    esac
    if [ -n "$(git -C "$w" status --porcelain -uno -- . ':!3ds/logs/perf/runs.jsonl' 2>/dev/null)" ]; then
        keep_wt="$keep_wt$n,"; echo "keep $n (uncommitted tracked changes)"; continue
    fi
    if [ -n "$ONLY" ]; then case "$ONLY" in *",$n,"*) ;; *) keep_wt="$keep_wt$n,"; continue ;; esac; fi
    b=$(git -C "$w" symbolic-ref --short -q HEAD || echo DETACHED)
    if [ -z "$ONLY" ] && [ "$b" != DETACHED ] && git rev-list --first-parent main | grep -qx "$(git rev-parse "$b")"; then
        keep_wt="$keep_wt$n,"; echo "keep $n (branch $b has no commit of its own: fresh worktree, use --only to remove)"; continue
    fi
    if [ "$b" != DETACHED ] && ! git merge-base --is-ancestor "$b" main 2>/dev/null; then
        keep_wt="$keep_wt$n,"; echo "keep $n (branch $b has $(git rev-list --count main.."$b") commits not in main)"; continue
    fi
    if [ -n "$(git -C "$w" status --porcelain -- . ':!3ds/logs/perf/runs.jsonl' 2>/dev/null | grep -v '^??')" ]; then
        run sh -c "git -C '$w' diff -- . ':!3ds/logs/perf/runs.jsonl' > build3ds/archive/wt_${n}_dirty.patch"
    fi
    (cd "$w" && git ls-files --others --exclude-standard | grep -E '\.(patch|diff)$') | while read -r f; do run cp "$w$f" build3ds/archive/; done
    echo "remove worktree $n ($b)"
    run git worktree remove --force --force "build3ds/wt/$n"
    drop_lane_copies "$n"
    drop_lane_volumes "$n"
done
# WTASSETS: a kept worktree may still hold copies of the git-ignored inputs (assets/us, ver/us/build, made before wt_new.sh).
# build.sh reads them from the main worktree, so a copy goes when the main worktree has it. Only kept worktrees; tracked files untouched.
MAINW=$(cd "$(git rev-parse --git-common-dir)/.." && pwd)
asset_mb=0
for w in build3ds/wt/*/; do
    [ -d "$w" ] || continue
    n=$(basename "$w")
    case "$keep_wt" in *",$n,"*) ;; *) continue ;; esac
    for d in assets/us ver/us/build; do
        [ -d "$w$d" ] && [ -d "$MAINW/$d" ] || continue
        mb=$(du -sm "$w$d" 2>/dev/null | cut -f1); mb=${mb:-0}
        echo "asset copy $w$d ($mb MB)"
        run rm -rf -- "$w$d"
        asset_mb=$((asset_mb + mb))
    done
done
if [ $APPLY = 1 ]; then echo "asset copies removed from kept worktrees: $asset_mb MB"; else echo "asset copies in kept worktrees: $asset_mb MB would be freed"; fi
run git worktree prune
# Lane loop: skipped with --only (it names worktrees only). A build-at folder (bench.py scratch_build writes
# .build_at_in_use at its start, and removes the folder at its end) is kept while the marker is under 12 h old:
# a build-at running now uses its folder and its volumes (wt_gc --apply removed one mid-run on 2026-11-08).
for l in build3ds/lane-* build3ds/at_*; do
    [ -n "$ONLY" ] && break
    [ -d "$l" ] || continue
    n=$(basename "$l"); n=${n#lane-}
    case "$keep_wt" in *",$n,"*) continue ;; esac
    [ -d "build3ds/wt/$n" ] && continue
    # In use: the marker (new build-at runs) or a file written in the last 30 min (any build-at run, marker or not).
    if [ -n "$(find "$l" -maxdepth 1 -name .build_at_in_use -mmin -720 -print -quit 2>/dev/null)" ] || [ -n "$(find "$l" -maxdepth 3 -type f -mmin -30 -print -quit 2>/dev/null)" ]; then echo "keep $l (build-at in use)"; continue; fi
    echo "remove lane $l"
    run rm -rf -- "$l"
    drop_lane_copies "$n"
    drop_lane_volumes "$n"
done
[ $APPLY = 1 ] || echo "dry run: add --apply to remove"
