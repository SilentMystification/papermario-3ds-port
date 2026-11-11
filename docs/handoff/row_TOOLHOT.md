# Row TOOLHOT: hotset prints M1-hot per class (the size that a cold split or a fast path can move)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Why (oracle code read)
hotset.py class sizes (run_report :240-241) sum the whole call closure of each class, cold callees included (.text.unlikely), so a
cold split or a fast path (HC7 steps 1-6, HC8 steps 3-7) does not move them; only the union has an M1-hot figure (:232-233).
The class targets of plan_hotcode.md section 2 need a per-class hot figure. M3 (fncov) stays the bytes-run measure.
## Start
`sh 3ds/tools/wt_new.sh TOOLHOT` (build3ds/wt/TOOLHOT, branch TOOLHOT from main). Runs beside HC8: no shared file.
## Touch only
3ds/tools/hotset.py (run_report only), 3ds/tools/bench_defs.json (the "log_metrics" list of bench "hotset_size" only, append).
Another row may edit other benches in bench_defs.json: do not move or reformat other lines; `git merge main` before the completing commit.
## Exact edits
1. hotset.py run_report: after `cb = {c: nbytes(fns, mem[c]) for c in CLASSES}` add
   `ch = {c: nbytes(fns, set(a for a in mem[c] if not fns[a]["cold"])) for c in CLASSES}  # TOOLHOT: per class without .text.unlikely`
   In the `if line:` print, after the class list `" ".join("%s %.1f" % (c, cb[c] / 1024.0) for c in CLASSES)` nothing changes.
   After the loop `for c in CLASSES: print("hotset M1 class %s ..."` add a second loop:
   `for c in CLASSES:`
   `    print("hotset M1hot class %s %d B %s" % (c, ch[c], CFG["classes"][c]["name"]))`
2. bench_defs.json, bench hotset_size, "log_metrics": append nine entries after the last one, one per class A..I:
   `{"name": "m1hot_A", "regex": "hotset M1hot class A (\\d+) B", "pick": "first"}` (and B, C, D, E, F, G, H, I the same way).
## Gate (tool row: no Azahar start)
- Main lane map (no build in this row): `python 3ds/tools/hotset.py > build3ds/out/hotset_toolhot.txt 2>&1` in the worktree with
  PM_LANE unset reads build3ds/pm_3ds.map of the worktree; if it has none, `PM_ROW=TOOLHOT PM_LANE=TOOLHOT sh 3ds/build.sh >
  build3ds/out/build.txt 2>&1` (memcheck OK), then `PM_LANE=TOOLHOT python 3ds/tools/bench.py run hotset_size --row TOOLHOT --lane
  TOOLHOT --why "TOOLHOT per class hot"`: rc 0, and the output has nine "hotset M1hot class" lines, each value <= its "M1 class" value.
- `python 3ds/tools/bench.py run nd_host --row TOOLHOT --why "TOOLHOT import check"`: PASS.
Write the nine M1hot values in the commit message.
## Stop and end
A failed gate or an edit that does not fit the code: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the nine values.
