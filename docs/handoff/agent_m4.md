# Agent M4 (row M4-2): handoff

## STATE
branch: nds1 | HEAD: 5f6d782f5 (M4-2 code is NOT in HEAD, see open 1)
record commit: dc2c8e3ee "M4-2: producer record for plain opaque map models" (model.c, pm_gbi.c, gbi_custom.h, bench_defs.json, azt_scenarios.json)
done: step 1 proof (mdldiff, 13000 runs, 0 diffs), step 2 record write and step 3 consumer (in dc2c8e3ee), host nd_host PASS
open 1: commit db42729fa (13:10, same author, 1 min after dc2c8e3ee) removed the M4-2 lines from src/model.c, 3ds/src/pm_gbi.c and include/gbi_custom.h. Not restored by this agent. Decide: restore (git cherry-pick dc2c8e3ee on top) or keep the revert
open 2: gate 3 seam_pic region_diff NOT valid yet. Baseline shots of d01f8ea62 saved in scratchpad seam_base. The seam_rec run was built from the reverted tree: discard it
open 3: gate 4 texdiff not run. Lights and matrix stay words (not in record, F3DEX2 light macros unverified)
azahar starts used on M4-2: 4 of 8 (mdldiff x2, seam baseline, seam rec)
next: after restore, build (PM_ROW=M4-2 sh 3ds/build.sh, restore map_restore_table.c md5 437b90ef...), then python 3ds/tools/bench.py run seam_pic --row M4-2 --why "picture of Toad Town with records", region_diff baseline vs record shots, then bench.py run texdiff --row M4-2 --why "bind lists still equal"

## Notes
- mdldiff run 1 showed 299 diffs at 500 runs: bind cache tl_last was not in TS_FIELDS. rec_diff now clears tl_last before each run. Run 2: 0 diffs.
- Consumer rec_run: one word (w0 op | render mode index | texture list index | old word count; w1 bind list). Old word count is nonzero only under switch mdldiff.
- Scenario mac01_mdldiff and bench mdldiff added in dc2c8e3ee.
