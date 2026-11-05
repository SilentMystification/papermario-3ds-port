# Agent M4 (row M4-2): handoff

## STATE
branch: nds1 | HEAD: see git log (record commit cherry-picked as 6411d34ec, user approved restore)
done: gate 1 nd_host PASS; gate 2 mdldiff PASS (13000 runs, 0 diffs, record commit dc2c8e3ee, now 6411d34ec)
failed: gate 3 seam_pic region_diff FAIL on frame 200 (0000_1_2.ppm): 781 px (0.81%), 26 regions, largest 144 (limit 64). Diff cluster x 200-239, y 0-119 (top right sign/building). Frames 201, 202 identical. rec count 17640 (record path ran).
not run: gate 4 texdiff (handback at failed gate 3)
open: gate 3 fails the row's scattered-diff rule; row says a failed valid gate drops the row. Coordinator decides: drop (revert 6411d34ec) or diagnose the top-right sign texture (record bind list or rm list)
azahar starts used on M4-2: 5 of 8 (mdldiff x2, seam base, seam rec x2)
next: coordinator decision; if continue, diagnose top-right region then rerun seam_pic and texdiff

## Notes
- mdldiff run 1 showed 299 diffs at 500 runs: bind cache tl_last was not in TS_FIELDS. rec_diff now clears tl_last before each run. Run 2: 0 diffs.
- Consumer rec_run: one word (w0 op | render mode index | texture list index | old word count; w1 bind list). Old word count is nonzero only under switch mdldiff.
- Scenario mac01_mdldiff and bench mdldiff added in dc2c8e3ee.
