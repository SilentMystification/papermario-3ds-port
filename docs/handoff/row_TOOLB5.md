# Row TOOLB5: a run records under the bench that started it, not the first bench of its scenario

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (HC4f3, 2026-11-10)
`bench.py base spot_mac00_det` at a5d5afe62 (build3ds/wt/HC4f3/build3ds/out/base_mac00.txt): both runs LOG CHECKS PASS, then
"not stored (run results none)". runs.jsonl (main and worktrees) holds 0 records of bench spot_mac00_det and 62 of spot_mac00_det_pic.
## Cause (oracle code read)
- bench.py cmd_ingest_last :1916 picks the first Azahar bench in bench_defs.json whose scenario is the run's scenario. Three
  scenarios have two benches: spot_mac00_det (spot_mac00_det_pic at bench_defs.json:4513 comes before spot_mac00_det at :6440),
  nok11_splash (nok11_splash_pic, fb_nok11_splash_pic), msgbox_talk_nowide (msgbox_talk_nowide_pic, msgbox_talk_nowide).
- So every run of bench spot_mac00_det is recorded as spot_mac00_det_pic; base_records (:1440) filters on bench spot_mac00_det, finds
  none, and base_run prints "not stored (run results none)" (:1494-1495). cmp of the second bench of a pair fails the same way.
- "scene - windows 0 fps -" is not a fault: spot_mac00_det has no prof switch, so no prof window line.
## Start
`sh 3ds/tools/wt_new.sh TOOLB5` (build3ds/wt/TOOLB5, branch TOOLB5 from main). PM_ROW=TOOLB5 PM_LANE=TOOLB5 on every bench call.
Not at the same time as another row that edits 3ds/tools/bench.py.
## Touch only
3ds/tools/bench.py: run_azahar_list, the single Azahar path of cmd_run (:1176-1188), cmd_ingest_last (:1916).
## Exact edits
1. run_azahar_list: the tuple `("PM_BENCH_LABEL", "PM_BENCH_BUILD", "PM_BENCH_COMMIT")` gets `"PM_BENCH_NAMES"`; after that loop add
   `env["PM_BENCH_NAMES"] = ",".join(names)  # TOOLB5: ingest-last records each run under the bench of this list`.
2. cmd_run single Azahar path: the same tuple change; after the loop add `env["PM_BENCH_NAMES"] = bd["name"]  # TOOLB5`.
3. cmd_ingest_last: replace line 1916 with
   `want = [n.strip() for n in os.environ.get("PM_BENCH_NAMES", "").split(",") if n.strip()]  # TOOLB5: the bench that started the run`
   `cands = [b for b in load_defs()["benches"] if b.get("scenario") == s and b["kind"].startswith("azahar")]`
   `bd = next((b for b in cands if b["name"] in want), None) or (cands[0] if cands else None)`
## Gate
- `python 3ds/tools/bench.py run nd_host --row TOOLB5 --why "TOOLB5 import check"`: PASS (0 Azahar starts).
- `python 3ds/tools/bench.py base spot_mac00_det --row TOOLB5 > build3ds/out/toolb5_base.txt 2>&1`: "two runs equal, stored", and the
  two new runs.jsonl records have bench "spot_mac00_det". Two Azahar starts. Run alone, no azahar.exe open outside build3ds\azslots.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash and the gate results.
