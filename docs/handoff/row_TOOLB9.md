# Row TOOLB9: a base compares two warm runs (one warm-up first); cmp names a cold row run

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (DETBASE2, 0a93193ee)
Bases at f978553f0, 7992565a8, 4bbd73043: every pair of warm runs equal. The pairs that differed were a cold run (warm 0) against a
warm run (warm 1): the base fill at 4bbd73043 (runs 273723 warm 0, 274545 warm 1). A cold boot draws a different first shot than a
warm restart (row COLDBOOT note), so a byte gate must compare runs of one warm state.
## Cause in the tool (oracle code read)
- bench.py base_run (:1476-1509) runs the bench BASE_TWICE (2) times in one list and compares the two runs; the first run of a list is
  cold when the slot has no kept Azahar of that lane, the second is a warm restart. The records carry "warm" (0 or 1, azt.ps1).
- bench.py cmd_cmp compares the row's last run with the stored base and does not look at the warm state of either.
## Start
`sh 3ds/tools/wt_new.sh TOOLB9` (build3ds/wt/TOOLB9, branch TOOLB9 from main). No running row edits bench.py.
## Touch only
3ds/tools/bench.py: base_run and the BASE_TWICE constant above it (:1473-1509), and cmd_cmp (the print after it finds the row run).
## Exact edits
1. Above base_run: `BASE_RUNS = 3  # TOOLB9: a warm-up run, then two warm runs are compared` (keep BASE_TWICE = 2).
2. base_run: the list is `",".join([name] * BASE_RUNS)` in place of `[name] * BASE_TWICE`. After `first, last = recs[-2], recs[-1]`
   add: `if first.get("warm") != 1 or last.get("warm") != 1:` / `    print("base %s: the compared runs are not both warm (warm %s, %s), not stored." % (bd["name"], first.get("warm"), last.get("warm")))` /
   `    bad += 1` / `    continue`. The check `len(recs) < BASE_TWICE` and the result test over `recs[-BASE_TWICE:]` stay (the last two runs).
3. cmd_cmp: after the line that prints "cmp %s: row %s run at commit ...", add
   `if rr.get("warm") == 0: print("cmp %s: the row run was a cold start (warm 0); the base is two warm runs. Run the bench once more (the slot keeps its Azahar: warm), then cmp." % a.bench); return 1  # TOOLB9`
   (rr is the row record the function already reads.)
## Gate (tool row)
- `python 3ds/tools/bench.py run nd_host --row TOOLB9 --why "TOOLB9"`: PASS; `python -c "import ast; ast.parse(open('3ds/tools/bench.py',encoding='utf-8').read())"`.
- One base on a commit with no stored base of the bench: `python 3ds/tools/bench.py base spot_mac01_det --row TOOLB9 --commit <the
  TOOLB9 merge-base>`: three runs; "two runs equal, stored"; the records of the last two have warm 1. Three Azahar starts (base runs,
  not counted). No azahar.exe outside build3ds\azslots during the run.
## Stop and end
A failed gate or a tool fault: commit and hand back the exact output (CLAUDE.md 4-5). At the end delete this row file in the completing
commit; tell the orchestrator: run `bench.py base --fill` again after the merge.
