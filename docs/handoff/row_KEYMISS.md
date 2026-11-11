# Row KEYMISS (step 2): census window at the frame end, mac_01 until, logs per scene

(Model: Haiku. Continue on branch KEYMISS (worktree build3ds/wt/KEYMISS, commit f3675da07). The agent reads only this file and CLAUDE.md.)

## Done at f3675da07 (step 1)
pm_keymiss.c (switch keymiss), the rec_miss hook (nd_recs.c), CMakeLists, scenarios and benches keymiss_mac01, keymiss_sbk56,
keymiss_kzn08, keymiss_hos01. Gates passed: build, memcheck, nd_host, spot_mac01_det 6/6 byte-equal. Azahar starts used: 6 of 8.

## Facts of step 1 and their cause (oracle code read)
- keymiss_mac01 rc 10, no until line in 30 s. Cause: km_print runs only inside pm_km_note, that is only at a miss or a refused hit.
  A standing mac_01 scene has no miss after its first frames (all hits), so no window line is ever printed. Fix: the window check
  moves to the frame end (pm_gpu_end_w, pm_gpu.c, the walker's end of every frame, beside the mfr_frame line).
- The 4 runs share build3ds/lane-KEYMISS/slot1 last_log.txt (last writer wins). The harness keeps one copy per scene: the `LOG:` line
  of each run, build3ds/azt/logs/<scenario>.txt in the worktree (test_catalog.md "Logs"). Read those copies only.

## Start
In build3ds/wt/KEYMISS: `git merge main` (KEYFIX1 and BATUI merged at f4d7a8061; no conflict expected: pm_gpu.c is not edited by
step 1). PM_ROW=KEYMISS PM_LANE=KEYMISS on every build and bench call.

## Touch only
3ds/src/pm_keymiss.c; 3ds/src/pm_gpu.c pm_gpu_end_w (one line) and one prototype line above it; 3ds/tools/azt_scenarios.json and
3ds/tools/bench_defs.json entry keymiss_mac01 only.

## Exact edits
1. pm_keymiss.c: new `__attribute__((cold, noinline)) void pm_km_frame(unsigned f)`: if (!km) { km = calloc(KM_N, sizeof *km);
   if (!km) return; km_f0 = f; while (km_next <= f) km_next += 300u; return; } if (f >= km_next) { km_print(f);
   while (km_next <= f) km_next += 300u; }. In pm_km_note delete the first-call allocation and the km_next print block; it starts with
   `if (!km) return;` (counting begins after the first frame end with the switch on).
2. pm_gpu.c: above `static void pm_gpu_end_w(void) {` add `void pm_km_frame(unsigned f); /* KEYMISS census window (pm_keymiss.c) */`.
   In pm_gpu_end_w after the line `if (__builtin_expect(mfr_on, 0)) mfr_frame(); ...` add:
   `if (__builtin_expect(PMDL("keymiss"), 0)) pm_km_frame(pm_frame_count()); /* KEYMISS: the census window line every 300 frames */`
3. keymiss_mac01 (scenario and bench): cap 45, deadline 40. Until stays `keymiss f(6|7)\\d\\d ` (with edit 1 the line comes at frame
   600 whether or not a miss happens). Then `python 3ds/tools/bench.py rebuild`.

## Gate (CLAUDE.md 2; the last 2 Azahar starts of the budget: one list)
- `PM_ROW=KEYMISS PM_LANE=KEYMISS sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `bench.py run nd_host --row KEYMISS --why "pm_km_frame"`: PASS.
- `bench.py base spot_mac01_det` (merge-base after the merge of main; stored by the orchestrator's base fill, else it builds it:
  a start that does not fit the budget is a stop), then one list
  `bench.py run spot_mac01_det,keymiss_mac01 --row KEYMISS --why "switch off byte-equal; mac_01 census window at the frame end"`;
  `bench.py cmp spot_mac01_det --row KEYMISS`: 6/6 byte-equal; keymiss_mac01 rc 0.
- The census lines: from build3ds/azt/logs/keymiss_mac01.txt (this run) and build3ds/azt/logs/keymiss_sbk56.txt, keymiss_kzn08.txt,
  keymiss_hos01.txt (step 1 runs; their counting code is unchanged). A missing copy or one without a `keymiss f6..` or `keymiss f7..`
  line: stop and hand back (tool fault, CLAUDE.md 4). No new run of those three.

## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, hand back the exact output (CLAUDE.md 4-5).
Delete this row file in the completing commit. Hand back: the hash, gate results, memcheck line, and for each of the 4 log copies (path)
the keymiss, keymissw and keymisso lines of the window that ends at frame 600-799, copied as they are. No interpretation.
