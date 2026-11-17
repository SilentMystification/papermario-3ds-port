# ws_BASEKEEP2 (Haiku): base --check --shots removes no stored base of another checkout; a failed check rerun keeps the entry
Designer: the oracle (SendMessage). Worktree build3ds/wt/BASEKEEP2, branch BASEKEEP2 from main, PM_ROW=BASEKEEP2 PM_LANE=BASEKEEP2.
Main at release: 358070e292. Start: now, parallel with GFXPOS7 and THEATVP (no shared file). Edit only the 3 files of
docs/handoff/ws_BASEKEEP2.patch: 3ds/tools/bench.py, new 3ds/tests/host.d/basecheck.sh, new 3ds/tests/basebin/test_basecheck.py.
## Objective (tool fault, counter: none; it stops lost bases that cost base runs)
- FACT (build3ds/out/post_merge_f1216481d1.txt:8978 and post_merge_0610a41868.txt:4517): `base entself_fx_drec at b514cebcd:
  stored for other inputs (content key differs): stale, removed`. The post-merge check removed the ENTSELF2 base. The coder saw no line.
- Cause (FACT, code): post_merge.sh:23 runs `bench.py base --check --shots` in build3ds/wt/postmerge at the merged main commit.
  base_check_shots (bench.py:2049) calls stored() on every index entry. stored() (bench.py:1514) computes shots_key with the bench and
  scenario definitions of that checkout. A bench of a row branch (bench.d/ENTSELF2.json) is not on that commit: the key differs and
  stored() calls drop_stored (bench.py:1526).
- Second path (FACT, code): base_check_shots drops the chosen entry (bench.py:2058) before its rerun. A rerun that stores nothing
  (a tool fault) loses the entry: `keep` is removed in the finally block (bench.py:2063).
- Fix: stored_why() (no removal) picks the candidates. stored() keeps its prints and removal. A failed rerun renames `keep` back and
  index_add() lists the bench again. Flags CHECK_PURE and RESTORE_KEPT are the seeded faults.
- FACT (oracle host run, main + this patch + GFXPOS7 + THEATVP): `basecheck ok` (real case and both seeded faults), ND HOST TESTS PASS.
## Edit shape (bench.py:2049). Before: `    cands = [(c, b) for c in live for b in (idx.get(c) or {}).get("benches", []) if stored(c, b)]`
After: `    check = (lambda c, b: not stored_why(c, b)) if CHECK_PURE else stored` and `... if check(c, b)]`.
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_BASEKEEP2.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. `python 3ds/tests/basebin/test_basecheck.py > build3ds/out/basecheck.txt 2>&1`. Pass: last line `basecheck ok`.
3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/basekeep2_host.txt 2>&1`. Pass: `ND HOST TESTS PASS` and a line `basecheck ok`.
4. Commit the 3 files, delete ws_BASEKEEP2.md and ws_BASEKEEP2.patch in the same commit:
   `BASEKEEP2: base --check --shots reads candidates without removal (stored_why); a failed rerun keeps the entry; host basecheck ok`.
No build and no Azahar run: the change is in a host tool. The host test is the proof.
## Tests that decide
- `basecheck ok` in both outputs of steps 2 and 3. Any `basecheck FAIL` line fails.
- `python 3ds/tools/bench.py base --help > /dev/null` exits 0 (the module loads).
## What you may edit to fix a fail
Only the 3 files. A `basecheck FAIL`: 1 try on the line it names. Then stop and send build3ds/out/basecheck.txt to the oracle.
## Stop and hand-back
One SubagentHandback: commit hash, the `basecheck ok` line, the ND HOST TESTS line.
