# Row WTASSETS: worktrees share one read-only asset folder (no 100 MB copies)

## User facts
User (2026-11-09): approved "one shared read-only asset folder for the worktrees in place of copies". Found: every agent copied
assets/us and ver/us/build (about 100 MB, git-ignored) into its worktree, because build.sh needs them and the junctions failed
inside the docker container copy.

## Start state
Worktree build3ds/wt/WTASSETS, branch WTASSETS from main. PM_ROW=WTASSETS, PM_LANE=WTASSETS. One Haiku agent. Tools only.

## Steps
1. Read 3ds/build.sh: find where the lane copies or mounts the source into docker (sync-host stage) and where assets/us and
   ver/us/build are read. Name why the junction failed (the copy step does not follow junctions, or docker sees the junction target
   path that does not exist in the container).
2. Fix at that point: build.sh takes the asset folders from the main repo root (the git common dir's work tree,
   `git rev-parse --git-common-dir`) when the worktree has none, and mounts or syncs them read-only into the lane volume. No copy
   into the worktree.
3. A new script 3ds/tools/wt_new.sh <row>: `git worktree add build3ds/wt/<row> -b <row> main`, touch .wt_keep, and nothing else
   (no asset copy). The orchestrator uses it from now on.
4. wt_gc.sh: also delete stray assets/us and ver/us/build copies in worktrees it keeps (they are git-ignored, so they are safe to
   delete) and report the MB freed.
5. docs: orchestrator_rules.md "Agents and worktrees" names wt_new.sh.

## Gates
- A fresh worktree from wt_new.sh with no asset copies builds with PM_LANE=<row> (memcheck line printed) and nd_host passes.
- The main build (no PM_LANE) is unchanged: same memcheck line.
- wt_gc.sh --dry-run lists the asset copies it would delete.
