# Agent M4 (row M4-2): handoff

## STATE
branch: nds1 | record code: 6411d34ec (cherry-pick of dc2c8e3ee)
done: gate 1 nd_host PASS; gate 2 mdldiff PASS (13000 runs, 0 diffs)
gate 3 PASS BY USER DECISION: seam_pic frame 200 region_diff 781 px (0.81 %), 26 regions, largest 144 (limit 64); frames 201, 202 identical. Two windows identical. The world draws correctly.
open note: frame 200 matrix table order differs (draw 23 palette entry gm 7, pcomp floats differ slightly; all 64 G_MTX source words equal, no interp). Cause not named, possibly load timing.
gate 4 texdiff: runs 6000, diffs 0 (bench texdiff, record build). Scenario picture check reported no shot (the run stops at 6000 binds before frame 200): not a gate.
azahar starts: about 11 on M4-2 since the budget reset
next: orchestrator (texdiff and mdldiff pass; seam pass by user). Prof run on console is orchestrator's step.

## Notes
- mdldiff run 1 showed 299 diffs at 500 runs: bind cache tl_last was not in TS_FIELDS. rec_diff now clears tl_last before each run. Run 2: 0 diffs.
- Consumer rec_run: one word (w0 op | render mode index | texture list index | old word count; w1 bind list). Old word count is nonzero only under switch mdldiff.
- Scenario mac01_mdldiff and bench mdldiff added in dc2c8e3ee.
