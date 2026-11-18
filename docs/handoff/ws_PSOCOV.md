# ws_PSOCOV (Haiku): 2 more harvest rounds of missed pso keys from the fxchk scenes (pso table coverage; precondition of C9 model -2)
Designer: oracle (SendMessage). Worktree build3ds/wt/PSOCOV, branch PSOCOV from main. PM_ROW=PSOCOV PM_LANE=PSOCOV. Waits for nothing.
Edit only: 3ds/src/nd_pso_seen.c, and only through `python 3ds/tools/psoseen.py`. No NST chain file. Model: Haiku (runs and one tool).
## Objective and counter
- Counter: the `miss` value of the `psochk end` line in fxchk_a, fxchk_b, fxchk_c falls each round. No C1-C12 counter moves now.
  The table must cover the keys before the miss path (decide, decide_miss in nd_decide.c, C9 model 2) can leave the device.
- FACT: pso_seen_note logs at most 256 distinct keys per run (nd_comb.c:249). PSOSEEN harvested fxchk_a/b/c once (a6b7a1b09b).
- FACT: the fxchk_a_dschk log of build3ds/wt/NST2C (2026-11-17 20:45) holds 256 `psoseen 0x` lines with 743 keys in nd_pso_seen.c.
  So fxchk_a still misses 256 or more distinct keys. fxchk_b and fxchk_c: OPEN.
- FACT: gen_pso_tab.py makes a table key of every ND_PSO_DESC in src and 3ds/src (build.sh:190). A new desc turns its miss into a hit.
- Expected: drec stays byte-equal (HWFIX1 gate: the table decision equals nd_comb_core, FACT for 2 benches; GUESS for the new keys).
## Steps (each output to build3ds/out/, read the last 20 lines). Budget: 8 Azahar starts (6 capture, 2 gate), 3 builds.
1. `sh 3ds/build.sh > build3ds/out/psocov_b1.txt 2>&1` (bench.py run does not build, bench.py:1218-1225).
2. Round 1. For B in fxchk_a fxchk_b fxchk_c: `python 3ds/tools/bench.py run $B --row PSOCOV --why "PSOCOV round 1"`.
   After each run read the last record of 3ds/logs/perf/runs.jsonl. Copy its `log_path` file to build3ds/out/psocov_r1_$B.log.
3. For each log: `grep -c "psoseen 0x"` and the last `psochk end` line. Write the 3 pairs to build3ds/out/psocov_r1.txt.
4. `python 3ds/tools/psoseen.py build3ds/out/psocov_r1_*.log > build3ds/out/psocov_m1.txt`. Commit `PSOCOV 1: round 1, <keys> keys (+<new>)`.
5. `sh 3ds/build.sh > build3ds/out/psocov_b2.txt 2>&1`. Round 2: steps 2-4 with r2 and "round 2". Commit `PSOCOV 2: round 2, <keys> keys (+<new>)`.
6. `python 3ds/tools/bench.py base fxcull_drec`, the same for miscp_drec (base runs on the merge-base, not charged).
7. `sh 3ds/tools/rowloop.sh --row PSOCOV --bench fxcull_drec,miscp_drec --why "PSOCOV gate" > build3ds/out/psocov_loop.txt 2>&1`.
## Example of the one edit shape (psoseen.py writes it, never by hand). Before: no line. After, one line per key, sorted:
`ND_PSO_DESC(pso_seen_15cbef94, 0x2d127e24, 0xfffff3f9, 0x1e08ac00, 0x0, 0x0, 0x0, 0x0, 0x0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0);` (nd_pso_seen.c:5)
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, `cmp fxcull_drec` and `cmp miscp_drec` byte-equal against the merge-base.
- In each of the 3 scenes the round 2 `psochk end` miss is lower than the round 1 miss. psoseen.py prints `new` above 0 in round 1.
## Fix rights
- None in code. A build error after psoseen.py (gen_pso_tab), a DIFF, a miss that does not fall, a tool fault: stop.
  Send the output path to the oracle with SendMessage. Wait.
- psoseen.py prints `new 0` in round 1: stop. Hand back the 3 r1 pairs (fact: no new keys).
## Stop and hand-back
- Last commit `PSOCOV 3: gate. fxcull_drec, miscp_drec byte-equal; psochk miss a/b/c <r1> -> <r2>; keys <n>` deletes this page.
- One SubagentHandback: hashes, LOOP lines, the r1 and r2 pairs per scene, the keys total, and whether a round 2 log holds 256 lines.
