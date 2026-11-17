# ws_POSTASYNC (Haiku): the post-merge check runs detached; the merge lane does not wait (counter T4)
Designer: the oracle (SendMessage). Worktree build3ds/wt/POSTASYNC, PM_ROW=POSTASYNC PM_LANE=POSTASYNC. Start now, parallel with
VOLHOST, GCONVSPLIT and HOSTD2 (no shared file).
## Objective
- FACT (MAINBIN hand-back): merge_row.sh step 8 waits for post_merge.sh: 842 s. Cold build 540 s, shot rerun 234 s.
- Target: merge_row.sh returns after the queue call. FACT (oracle run, temp repo): a whole merge_row.sh run took 2 s, test (i).
- Check cost (GUESS from the FACT split): 70 s every merge, plus 540 s on merge count mod 3 = 0, plus 234 s on mod 3 = 1.
## Design (all in docs/handoff/ws_POSTASYNC.patch, 9 files; edit only these)
3ds/tools/postmerge_bg.sh (new), post_merge.sh, merge_row.sh, bench.py (base_check split), 3ds/push_3ds.sh,
3ds/tests/postmerge/run.sh (new), 3ds/tests/mergerow/run.sh, 3ds/tests/push/run.sh, docs/handoff/plan_buildspeed.md (rule 5).
- merge_row.sh step 8 calls `postmerge_bg.sh --queue <HEAD>`. It writes build3ds/out/postmerge_q/<time>.job and starts the
  runner with nohup. The runner holds build3ds/out/postmerge.lock. It checks each queued commit, oldest first, in the worktree
  build3ds/wt/postmerge (detached, .wt_keep). The main folder stays clean. A merge can run at the same time.
- rc != 0 writes build3ds/out/postmerge_FAIL.txt. merge_row.sh exits 8 and push_3ds.sh exits 8 while it exists.
  A full pass (cold 1, shots 1) on a newer descendant commit removes it. Every job runs full while the file exists.
- Cache cover: generators and gconv objects cold vs entry, key sweep: every merge. ccache, build dir, asset volume, basebin
  bytes: the cold build, every 3rd merge. Stored shots: the shot rerun, every 3rd merge. All caches also check hashes on each use.
## Edit shape. Before (merge_row.sh:140): `sh "${POST_MERGE:-3ds/tools/post_merge.sh}" > "build3ds/out/post_merge_$mhash.txt" ...`. After: `if pq=$(sh "${POSTMERGE_BG:-3ds/tools/postmerge_bg.sh}" --queue "$(git rev-parse HEAD)" 2>&1); then pmtxt=...`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_POSTASYNC.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. `sh 3ds/tests/postmerge/run.sh > build3ds/out/pa_pm.txt 2>&1`: `postmerge: 4/4 PASS`.
3. `sh 3ds/tests/mergerow/run.sh > build3ds/out/pa_mr.txt 2>&1`: `mergerow: 11/11 PASS`.
4. `sh 3ds/tests/push/run.sh > build3ds/out/pa_push.txt 2>&1`: `ok   post-merge FAIL file: exit 8, nothing sent`, `PUSH TESTS PASS`.
5. Commit the 9 files: `POSTASYNC 1: the post-merge check runs detached (queue, lock, FAIL file stops merge and push)`.
6. Gate, the real check of your commit (the row has no other build): `sh 3ds/tools/postmerge_bg.sh --queue --full $(git rev-parse HEAD)`.
   It prints `postmerge: queued <short>` at once. M = the main folder. Wait in ONE call (timeout 590000), repeat it at most 3 times:
   `until test -f M/build3ds/out/post_merge_<short>.done; do sleep 10; done`.
7. Pass: the .done line is `post_merge <hash> rc 0 cold 1 shots 1`. M/build3ds/out/post_merge_<short>.txt has `post_merge ok`,
   `cachecheck basebin <short> ok: a cold build has the same 3dsx bytes` and `cachecheck shots ... ok`. `git -C M status --porcelain`
   prints nothing. No M/build3ds/out/postmerge_FAIL.txt.
Budget: 1 gate run (2 builds: warm prebuild, cold check; 2 Azahar starts). No nd_host_test.sh run (no host.d test reads these files).
## Fail
- Steps 2-4: 2 tries per cause, then send the output path to the oracle. Do not change a test.
- Step 7 fail, or `cachecheck shots: no stored bench`: stop. Remove M/build3ds/out/postmerge_FAIL.txt if the gate wrote it.
  Send the .txt path to the oracle.
## Merge note (orchestrator)
The merge of this row runs the old merge_row.sh step 8: it ends with exit 7 (the new post_merge.sh needs a commit argument).
Then run `sh 3ds/tools/postmerge_bg.sh --queue` in the main folder once. HANDOFF.md: drop the PM_POSTMERGE_EVERY line (gone).
## Stop and hand-back. Last commit `POSTASYNC 2: gate` deletes ws_POSTASYNC.md and .patch. One SubagentHandback: commits, the 3
test result lines, the .done line, the 3 pass lines of step 7, the gate wait time.
