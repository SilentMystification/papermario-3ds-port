# Agent M4 (row M4-2): handoff

## STATE
branch: nds1 | record code on it: 6411d34ec (cherry-pick of dc2c8e3ee, not reverted)
done: gate 1 nd_host PASS; gate 2 mdldiff PASS (13000 runs, 0 diffs)
failed: gate 3 seam_pic frame 200 FAIL twice (windows 1 and 2 same binary, identical: 781 px, 26 regions, largest 144; limit 64). Frame offset check: base200 vs rec201 1002 px, largest 373; base201 vs rec200 781 px, largest 144. No pair passes. Diff area x 200-239, y 0-119 (pink building, top right). rec count 17640.
not run: gate 4 texdiff (gate 3 not passed)
open: field the record sets differently for the top-right model not identified. Next: modellog=<id> run for that model (one Azahar start) to get texture, rm and light state of the building, then compare with base
azahar starts used on M4-2: 6 of 8

## Notes
- mdldiff run 1 showed 299 diffs at 500 runs: bind cache tl_last was not in TS_FIELDS. rec_diff now clears tl_last before each run. Run 2: 0 diffs.
- Consumer rec_run: one word (w0 op | render mode index | texture list index | old word count; w1 bind list). Old word count is nonzero only under switch mdldiff.
- Scenario mac01_mdldiff and bench mdldiff added in dc2c8e3ee.
