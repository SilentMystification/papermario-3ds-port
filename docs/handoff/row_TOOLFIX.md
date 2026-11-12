# Row TOOLFIX: the push stale check compares the build content key, and go.ps1 push names the file it sends

(Model: Haiku. Two scripts and their host test; exact edits. No console, no Azahar.)

## User facts (2026-11-12)
`push ... -NoBuild` refused a correct 3dsx: "STALE 3DSX: built eed4ffaf2, HEAD e4292fcc7", although build.sh had skipped the compile
(up to date) in 24 s. HEAD moves with every docs commit.

## Cause (ORACLE7 code read, main 5be532ef9)
- push_3ds.sh:169-177 compares PM_GIT_HASH of 3ds/include/pm_build.h with `git rev-parse --short=9 HEAD`. build.sh's stale rule is
  the content key (BSTALE, build.sh:111-129): key_now over KEY_IN (no docs), stored in $OUT/build_key.txt at a successful build;
  `PM_KEY_ONLY=1 sh 3ds/build.sh` prints the key of the tree and stops before docker (:128-129).
- The default output is build3ds/ only when PM_LANE is empty (build.sh:18-19); push_3ds.sh sends build3ds/pm_3ds.3dsx (:75). A
  caller shell with PM_LANE set builds into build3ds/lane-<lane>/ while the push sends the default file.

## Start
`sh 3ds/tools/wt_new.sh TOOLFIX`. PM_ROW=TOOLFIX.

## Touch only
3ds/push_3ds.sh (:169-177 only), 3ds/go.ps1 (the push verb block and Push-One's push_3ds.sh call only), 3ds/tests/push/run.sh (case (b)
and one new case before the summary). Not build.sh (REGDIR3 edits it).

## Exact edits
1. push_3ds.sh :169-177: when nobuild is 1 and PM_SRC3DSX is empty: `have=$(cat "${PM_BUILD_KEY_FILE:-build3ds/build_key.txt}" 2>/dev/null)`,
   `now=$(PM_LANE= PM_KEY_ONLY=1 sh 3ds/build.sh)`; if they differ or have is empty: `echo "STALE 3DSX: build key ${have:0:12}, tree
   key ${now:0:12} (a source, include or tool input changed since the build). Build first. Nothing was pushed."; exit 5`. Delete
   the PM_GIT_HASH comparison and the PM_BUILD_H test hook (replace by PM_BUILD_KEY_FILE in the comment).
2. push_3ds.sh, right after the check (and on the build path after build.sh): print
   `SEND FILE: <SRC3DSX> <size> bytes build <PM_BUILD_STR of pm_build.h> key <first 12 of build_key.txt>`.
3. go.ps1 push: before Invoke-Build and before Push-One, `Remove-Item Env:PM_LANE -ErrorAction SilentlyContinue` (a push always
   builds and sends the default output) and print `push: default output build3ds/pm_3ds.3dsx`.
4. run.sh case (b): use PM_BUILD_KEY_FILE pointing to a temp file holding `0000` (expect exit 5 and `STALE 3DSX`); new case (b2):
   PM_BUILD_KEY_FILE holding the output of `PM_LANE= PM_KEY_ONLY=1 sh 3ds/build.sh` (expect no STALE line, `SEND FILE:` printed,
   with the fake 3dslink of case (a)).

## Gate (expected values)
- `sh 3ds/tests/push/run.sh`: every case ok, last line `PUSH TESTS PASS`; hand back the last 14 lines.
- `sh 3ds/push_3ds.sh --dry --no-build`: rc 0 when build3ds/build_key.txt equals the tree key. Budget 0.

## Try 2 (ORACLE7, after 2ca2a3728: row spec fault, budget reset once)

- Why the worktree key differs from main's build_key.txt at the same HEAD (build.sh:119-126, key_now): besides the tracked tree it
  hashes files that each folder generates or holds itself: the untracked files under KEY_IN (`git ls-files -o`), the uncommitted
  diff, the generated 3ds/src/map_restore_table.c, 3ds/src/nd_gfx_tab.c, 3ds/ld/map_restore.ld, map_restore_bss.ld and
  3ds/include/texkey_patch.h of that folder (main's nd_gfx_tab.c is older than the worktree's), and the asset list with mtimes. So a
  key is valid only against the build_key.txt of the same folder and output. That is the rule, not a fault to remove: the 3dsx is
  the output of that folder's generated files.
- Rule for the check (push_3ds.sh): compare the key of the current folder (`PM_LANE= PM_KEY_ONLY=1 sh 3ds/build.sh` run in the folder
  that holds build3ds/pm_3ds.3dsx) with that folder's build3ds/build_key.txt. A push runs from the main folder (go.ps1 Set-Location
  $repo). Write this rule as the comment above the check.
- Test hook: the check runs when nobuild is 1 and (PM_SRC3DSX is empty or PM_BUILD_KEY_FILE is set). Case (b2): PM_BUILD_KEY_FILE =
  a temp file with the worktree's own key (the command above, run in the worktree), PM_SRC3DSX = the dummy 3dsx of run.sh,
  PM_3DSLINK = the fake of case (a): expect no STALE line, the `SEND FILE:` line and the fake's `Sending` line. Case (b): a key file
  holding `0000`: exit 5 and `STALE 3DSX`.
- Gate: `sh 3ds/tests/push/run.sh`: every case ok, `PUSH TESTS PASS`. No Azahar, no main-folder file used. Budget 0.

## Stop
A failing case: stop, commit, hand back. Delete this row file in the completing commit.
