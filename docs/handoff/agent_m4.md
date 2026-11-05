# Agent M4 (row M4-2): handoff

## STATE
branch: nds1 | record code: 6411d34ec (cherry-pick of dc2c8e3ee)
done: gate 1 nd_host PASS; gate 2 mdldiff PASS (13000 runs, 0 diffs)
failed: gate 3 seam_pic frame 200 FAIL, deterministic over 2 windows: 781 px, 26 regions, largest 144 (limit 64). Frames 201, 202 pass. Cross-frame checks fail too.
diag: record bind-list misses = 0 (no walker fallback). Record path calls the same tb_run then rm_apply as the old G_PM_TEX/G_PM_RM words. Differences left: gv_epoch bumps 1 instead of 2 per model, and the pink building model id is unknown (no modellog run yet).
not run: gate 4 texdiff. Fix tries: 0 of 2 (no field identified).
azahar starts on M4-2: 7 of 8 (budget reset allowed by coordinator)
next: find pink building id (modellog), run base vs record field compare, then fix and seam_pic once

## Notes
- mdldiff run 1 showed 299 diffs at 500 runs: bind cache tl_last was not in TS_FIELDS. rec_diff now clears tl_last before each run. Run 2: 0 diffs.
- Consumer rec_run: one word (w0 op | render mode index | texture list index | old word count; w1 bind list). Old word count is nonzero only under switch mdldiff.
- Scenario mac01_mdldiff and bench mdldiff added in dc2c8e3ee.
