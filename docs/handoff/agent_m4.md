# Agent M4 (row M4-2): handoff

## STATE
branch: nds1 | record code: 6411d34ec (cherry-pick of dc2c8e3ee)
done: gate 1 nd_host PASS; gate 2 mdldiff PASS (13000 runs, 0 diffs)
failed: gate 3 seam_pic frame 200 FAIL (781 px, 26 regions, largest 144), 2 windows identical
ndref f200 (base d01f8ea62 vs rec): 61 draws each, 43 bytes differ. Draw 9 TevKey plan_gen (cache counter) differs; draw 23 palette pcomp odd floats differ (base 0x3f32ffed, rec 0x3f339e40). Cause not yet explained.
fix tries: 0 of 2. gate 4 not run.
azahar starts: 9 (budget reset done at start 8)
next: explain pal_tab game matrix difference for draw 23 (pm_gpu.c pal_tab, pcomp_gen cache), then fix try 1, seam_pic once

## Notes
- mdldiff run 1 showed 299 diffs at 500 runs: bind cache tl_last was not in TS_FIELDS. rec_diff now clears tl_last before each run. Run 2: 0 diffs.
- Consumer rec_run: one word (w0 op | render mode index | texture list index | old word count; w1 bind list). Old word count is nonzero only under switch mdldiff.
- Scenario mac01_mdldiff and bench mdldiff added in dc2c8e3ee.
