# Row VOLMAN: lane source volume out of sync with the build manifest

## Facts (agents PERF2 and STARFX, 2026-11-08)
- A lane's docker volumes (pm3ds-src-<lane>, pm3ds-build-<lane>) were deleted several times while the worktree was alive (likely wt_gc.sh or another clean-up removing volumes of a lane it judged dead, or a docker prune).
- build.sh (3ds/build.sh:43-60) copies only files whose size/mtime changed since the manifest build3ds/lane-<x>/src_manifest.txt. It drops the manifest only when the volume is missing at sync time (`docker volume inspect $SRCVOL || rm -f $MAN`). A volume recreated empty in between, or a volume that lost files, keeps the old manifest: the sync copies nothing and cmake fails ("/src/3ds has no CMakeLists.txt"). Workaround used: delete src_manifest.txt by hand.

## Goal
1. Find who deletes live lanes' volumes: read 3ds/tools/wt_gc.sh (the lane loop, the `[ -d build3ds/wt/$n ]` check, names with a dash or the main-repo vs worktree build3ds paths: a worktree's lane is build3ds/wt/<x>/build3ds/lane-<x>, not build3ds/lane-<x>), CLEANB's clean-up, BSTALE's build key, any `docker volume rm`/prune in 3ds/tools and 3ds/*.sh. Name the file and line. Fix it so a live worktree's volumes are never removed.
2. Make the sync robust: build.sh checks a marker file inside the source volume (e.g. /src/.pm_manifest_id holding a random id also stored in the manifest); if missing or different, drop the manifest and copy everything. One check, no second path.
3. The build key (BSTALE) must not skip a build when the lane's volume or 3dsx is gone.

## Start state
Worktree build3ds/wt/volman, branch volman from main. Tools only.

## Done criteria
1. Cause named (file:line).
2. Test: build lane volman; delete its pm3ds-src-volman volume (docker volume rm) and recreate it empty (docker volume create); build again: the sync copies everything and the build passes, with no manual step.
3. wt_gc.sh dry run with live worktrees lists none of their volumes for removal.

## Rules
CLAUDE.md rules 7, 9, 11. PM_LANE=volman. No git stash. No stubs. Never touch another lane's volumes. Never commit 3ds/src/map_restore_table.c.

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "volman ready <hash>".
