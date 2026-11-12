# Row NAPICEN2: typed ops left per producer on main (measurement for S5 and S6, no code edit)

Model: Haiku (runs 3 existing scenes and the existing report tool; no code, no JSON edit).

## Start
`sh 3ds/tools/wt_new.sh NAPICEN2`; PM_ROW=NAPICEN2 PM_LANE=NAPICEN2. Build on main as it is (`sh 3ds/build.sh`).

## Touch only
Nothing in the repo. Outputs go to build3ds/out/ of the worktree.

## Commands
- `python 3ds/tools/bench.py run napicen_mac01,napicen_talk,napicen_btl --row NAPICEN2 --why "typed ops left per producer"` (one list,
  3 starts; the scenes exist: until `napicen f(6|7)\d\d done`).
- `python 3ds/tools/napicen.py <log> --map <worktree>/build3ds/pm_3ds.map --label <scene>` on each log (the NAPI0 report: typed ops per frame by file and function), output to
  build3ds/out/napicen2_<scene>.txt.

## Gate (measurement)
- Each of the 3 logs has the `napicen f... done` line and napicen.py prints a table. A missing line or a tool error: stop, hand back
  the exact output.
- Expected: hud_element.c and msg_draw.c glyph ops are lower than in the NAPI0 tables (NAPI2A and NAPI3A moved them to records);
  effects on nd_parts show only their pre and post units.

## Hand back
- Per scene: the 30 largest producers (file, function, ops per frame), and the total ops per frame.
- No commit is needed (no file changes). Delete this row file: the orchestrator deletes it at the hand-back.
