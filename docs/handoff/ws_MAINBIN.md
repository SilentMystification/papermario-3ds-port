# ws_MAINBIN (Haiku): post-merge base build and cachecheck at every merge; the cold gate (counters T3, T4)
Designer: the oracle (SendMessage). Worktree build3ds/wt/MAINBIN, branch MAINBIN, PM_ROW=MAINBIN PM_LANE=MAINBIN.
Start after Merge BASEBIN and Merge VOLGEN (post_merge.sh calls `bench.py base --prebuild/--check` and `PM_CACHE=check`).
## Objective
- After each merge, merge_row.sh runs 3ds/tools/post_merge.sh: 1 generator cachecheck, 2 base build of main HEAD into basebin,
  3 basebin and shot cachecheck, 4 commit of its runs.jsonl lines. A row from that main then needs 0 base builds.
- Cache safety rule 5 (cachecheck at every merge) and rule 6 (cold gate at the end of the build speed phase).
- Cost per merge: GUESS 12-18 min (step 1 cold generators, step 3 one build with no ccache, 2-4 Azahar starts).
## Files (all in docs/handoff/ws_MAINBIN.patch). Edit only these 3.
3ds/tools/merge_row.sh (step 8, exit 7), 3ds/tools/post_merge.sh (new), 3ds/tests/mergerow/run.sh (POST_MERGE stub, case h).
## Cache keys
No new cache. post_merge.sh checks the caches of VOLGEN (gencache) and BASEBIN (basebin, base shots) with their own keys.
## Edit shape (the patch does it)
Before (merge_row.sh, step 7): `echo "merge_row: merge $mhash of $row; ...; branch $row $bstate"`.
After: step 8 first runs `sh "${POST_MERGE:-3ds/tools/post_merge.sh}" > "build3ds/out/post_merge_$mhash.txt" 2>&1 || pmrc=$?`,
then the line ends with `; post_merge rc $pmrc`, and `pmrc` not 0 gives exit 7.
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_MAINBIN.patch | git apply -`. A reject: stop, send the output to the oracle.
2. `sh 3ds/tests/mergerow/run.sh > build3ds/out/mb_mr.txt 2>&1`: `mergerow: 8/8 PASS`. `sh 3ds/tools/nd_host_test.sh > build3ds/out/mb_host.txt 2>&1`: `ND HOST TESTS PASS`.
3. Commit the 3 files: `MAINBIN 1: post_merge.sh after each merge: generator cachecheck, basebin prebuild, basebin and shot cachecheck`.
4. `sh 3ds/tools/post_merge.sh > build3ds/out/mb_post.txt 2>&1`. Pass: rc 0, `CACHECHECK gens ok`, `basebin ... ready`,
   `cachecheck basebin ... ok`, and `cachecheck shots ... ok` or `no stored bench to check`, and `post_merge ok`.
5. Cold gate: `PM_CACHE=off PM_BUILD_CAP=1800 PM_ROW=MAINBIN PM_LANE=COLD sh 3ds/build.sh > build3ds/out/mb_cold.txt 2>&1` (rc 0, no `cache hit` line),
   then `PM_LANE=COLD python 3ds/tools/bench.py run spot_mac01_drec --row MAINBIN --why "MAINBIN cold gate"` and
   `python 3ds/tools/bench.py cmp spot_mac01_drec --row MAINBIN --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal.
Budget: 3 builds (step 4: prebuild and a cold build; step 5: a cold build), 5 Azahar starts.
## Fail
- `CACHECHECK FAIL` in step 4: stop. Send build3ds/out/mb_post.txt to the oracle (a cache gave other bytes than a cold run).
- A shell error that a step names in one of the 3 files: fix that line only. 2 tries. Then send the output path to the oracle.
## Stop and hand-back
All steps pass. The last commit `MAINBIN 2: gate` deletes ws_MAINBIN.md and ws_MAINBIN.patch. Hand back (one SubagentHandback):
commits, the step 4 pass lines with the wall time of post_merge.sh, the cold build wall time, the cmp line.
