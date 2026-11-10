# Row TOOLB2: shared run ids, and a cold gate that is cold (Haiku; after FFOG1, both edit azt_scenarios.json)

(`sh 3ds/tools/wt_new.sh TOOLB2`, PM_ROW=PM_LANE=TOOLB2. Output to build3ds/out/. Delete this row file in the completing commit.)

## Row TOOLB2 (Haiku): shared run ids, and a cold gate that is cold
1. 3ds/tools/bench.py runs_lock.__enter__: lock main_folder()/"build3ds"/"runs.lock" (mkdir parents) in place of PERF/"runs.lock".
2. 3ds/tools/bench.py append_runs: after `n = max(...)` add `for f in all_tree_runs_files(): n = max([n] + [json.loads(x).get("run_id", 0) for x in f.read_text(encoding="utf-8").splitlines() if x.strip()])`, so a new id is above every id of the main folder and of every worktree.
3. 3ds/tools/azt_scenarios.json battle_start_f182_cold: set "extra": ["-NoWarm"]. Its cap 60 and deadline 45 already fit a cold start. The other DET scenarios whose text says -NoWarm are left to a user decision (cost: about 11 s per run).
Gate:
(a) In a worktree, `python 3ds/tools/bench.py run nd_host --row TOOLB2 --why "run id check"` (host test, 0 Azahar starts). The printed run id is greater than the max run_id of the main folder's runs.jsonl and of every worktree runs.jsonl (a python one-liner over all_tree_runs_files()).
(b) The one Azahar start of this row is the next battle_start_f182_cold run of HC4e3 itself (no extra start): its record has warm 0 and run_azahar prints "Cold start (option)".
Stop: any gate fails, then hand back.
- Also (FFOG1 run, 2026-11-10): a run that reaches its cap with periodic shots still ends in SHOT COPY FAILED (76 of 77 full, 0076 short after 30 s) after AZTSTEREO. Close it in run_azahar.ps1 (after TOOLB1 merges, same file): at a timeout stop, a short unlogged last file is a write cut by the stop; drop it, and report only logged shots.
