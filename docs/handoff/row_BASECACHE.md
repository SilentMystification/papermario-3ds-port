# Row BASECACHE: stored base shots per main commit (no build-at per row)

## User facts
User (2026-11-09): "Base builds per row - seems like we're still over testing, how to improve this." Approved: a shared base shot
store. Found: nearly every row built a build-at of main and ran the same 3 to 6 scenes again for its byte gates.

## Start state
Worktree build3ds/wt/BASECACHE, branch BASECACHE from main. PM_ROW=BASECACHE, PM_LANE=BASECACHE. One Haiku agent. Tools only.

## Steps
1. Store: build3ds/basecache/<commit>/<bench>/ holds the shots and the log of a det bench run on main at <commit>. A small index
   file (build3ds/basecache/index.json) maps commit -> benches stored.
2. bench.py: `bench.py base <bench> [--commit <c>]` returns the stored shots of the merge-base of the current branch with main;
   when they are missing it builds main at that commit ONCE in lane `base-<short>` (git worktree add --detach, gen_map_restore,
   build.sh, then run the bench) and stores them. Any later row with the same merge-base reuses them.
3. `bench.py cmp <bench> --row <row>` compares the row's last shots of that bench with the base store: byte-equal count, per-shot
   diff pixel count and bounding box, and writes one diff PNG per differing shot. Exit 0 when all equal.
4. The orchestrator's main build after a merge may fill the store for the standard byte scenes (spot_mac01_det,
   battle_start_f182_cold, mdl5_iwa10_det) with `bench.py base --fill` (no extra build: it runs the scenes on build3ds/pm_3ds.3dsx).
5. The base worktree and lane are removed after the store is filled (wt_gc must not keep them). The store keeps the last 5 commits.
6. docs: test_catalog.md and the GATES line in CLAUDE.md name `bench.py base` and `bench.py cmp`. Delete any doc text that tells an
   agent to make a build-at of main.

## Gates
- `bench.py base spot_mac01_det` on a clean store builds once and stores 6 shots; a second call returns at once with no build.
- `bench.py cmp spot_mac01_det --row BASECACHE` after a run of the same commit reports 6/6 equal.
- A one-pixel test edit (temporary, not committed) makes cmp report the diff count and bounding box.
- No game code change.
