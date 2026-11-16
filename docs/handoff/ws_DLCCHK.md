# ws_DLCCHK (Haiku): remove the stale bench dlcchk_mac01 from the shared registries (no counter change)
Designer: the oracle (SendMessage). Worktree build3ds/wt/DLCCHK, branch DLCCHK. PM_ROW=DLCCHK PM_LANE=DLCCHK. Start now.
This page allows edits of 3ds/tools/bench_defs.json and 3ds/tools/azt_scenarios.json (shared registries).
## Objective
- FACT (MAPCONV commit 9210f90a41): the dlcchk switch and the run-time compare are gone. The dlcpre line is now
  `dlcpre f%u pre %u miss %u full %u` (3ds/src/nd_shape.c:382). The until line of dlcchk_mac01 needs `rt` and `bad`
  (azt_scenarios.json:1245, bench_defs.json:3980): the bench can never end.
- FACT (git grep dlcchk): the only users are the 2 registry entries and old runs.jsonl lines (append-only: keep them).
- No listcen change. No build. No Azahar start.
## Files (all in docs/handoff/ws_DLCCHK.patch). Edit only these 2.
3ds/tools/bench_defs.json (remove the object at lines 3972-3985), 3ds/tools/azt_scenarios.json (remove the object at lines 1238-1258).
## Edit shape
Before (bench_defs.json): `    },` `    {` `      "name": "dlcchk_mac01",` ... `    },` `    {` `      "name": "fxpic_63",`
After: `    },` `    {` `      "name": "fxpic_63",`. Remove the whole object and its closing `},`. Nothing else.
## Loop
1. `tr -d '\r' < docs/handoff/ws_DLCCHK.patch | git apply -`. A reject: stop, send the output to the oracle.
2. `python -c "import json; json.load(open('3ds/tools/bench_defs.json')); json.load(open('3ds/tools/azt_scenarios.json')); print('json ok')"`.
   Pass: `json ok`.
3. `git grep -n dlcchk -- 3ds/tools` gives no line.
4. `python 3ds/tools/bench.py list > build3ds/out/dlcchk_list.txt 2>&1` (bench.py:2774). Pass: exit 0 and no `dlcchk` in the file.
5. Commit the 2 files and the page delete in one commit:
   `DLCCHK: remove the stale bench dlcchk_mac01 (its until line needs the dlcpre rt and bad fields that MAPCONV removed); ws_DLCCHK page deleted`.
## Fail
- A JSON error: compare the 2 files with the patch. 2 tries. Then write the output to build3ds/out/ and send the path to the oracle.
## Stop
Step 5 is committed.
## Hand-back (one SubagentHandback message)
The commit hash and the step 2 to 4 lines.
