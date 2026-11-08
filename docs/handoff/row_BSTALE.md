# Row BSTALE: build only when the sources changed (user, 2026-11-08)

## User facts
User: "whenever I ask for a push to a console a REBUILD happens. This should only happen if its stale."
Facts (orchestrator trace):
- go.ps1 Invoke-Build (3ds/go.ps1:118-147) calls build.sh when any file in 3ds/src, 3ds/include, 3ds/shaders, src, include is NEWER (mtime) than the 3dsx. A merge or a git checkout writes files with a new mtime even when the content is equal, so the check says stale.
- build.sh then always spends about 51 s in "sync-host" (3ds/build.sh:43-60): `find` with mtimes over all SRC_DIRS, including assets/us and ver/us/build (thousands of files), before docker and ninja run. The push on 2026-11-08 15:xx: sync-host 51 s, changed 21 files.
- A content change in 3ds/include/pm_port.h (included by every file) rightly rebuilds all 3355 objects; that is not this bug.

## Goal
A push (and build.sh) skips the build when the build inputs have the same CONTENT as at the last successful build of that output folder, in a few seconds. Proposed key: `git rev-parse HEAD:` tree hashes of the tracked inputs (3ds/CMakeLists.txt, 3ds/*.cmake, 3ds/map_syms.lds, 3ds/ld, 3ds/shaders, 3ds/stub_include, 3ds/include minus pm_build.h, 3ds/src, src, include) plus a hash of `git diff HEAD -- <those>` (uncommitted changes such as src/msg.c and 3ds/src/map_restore_table.c) plus the size/mtime list of the untracked inputs assets/us and ver/us/build only if a cheap check shows they change. Stored next to the 3dsx (build3ds[/lane-x]/build_key.txt) after a successful build. go.ps1 and build.sh compare and skip. pm_build.h (build number from the commit count) must not make a doc-only commit stale. One check, one place used by both (no second path).
Also make sync-host faster when it does run (e.g. skip assets/us and ver/us/build in the find when their key did not change).

## Start state
Worktree build3ds/wt/bstale, branch bstale from nds1 (current). Tools only.

## Premise and its check
Check first: time `find $SRC_DIRS -type f -printf ...` alone (expect most of the 51 s), and time the git key computation (expect under 2 s).

## Touch only
3ds/go.ps1 (Invoke-Build), 3ds/build.sh (key check, sync), docs/handoff/test_catalog.md if it names the build steps.

## Done criteria
1. Two builds in a row with no change: the second prints "build up to date (key ...)" and takes under 5 s.
2. A doc-only commit, then a build: skipped.
3. A one-line change in a .c file: the build runs and ninja rebuilds that file only.
4. A lane build (PM_LANE) keeps its own key.
Affected scenarios: none (no Azahar start needed).

## Rules
CLAUDE.md rules 7, 9, 11. Never commit 3ds/src/map_restore_table.c. No git stash. No stubs.

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "bstale ready <hash>".
