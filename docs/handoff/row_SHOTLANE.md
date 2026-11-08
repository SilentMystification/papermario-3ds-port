# Row SHOTLANE: shot archive per build, not per scenario only

## User facts (tool bug)
Found by CUR1 (2026-11-08): azt.ps1 keeps a run's shots in build3ds/azt/shots/<scenario> (azt.ps1:414, 469, 558) under the tree root ($repo, :43). A base build-at run of the same scenario from the same tree (other -Build lane) deletes and overwrites them (:558), so a byte compare of "row shots vs base shots" can compare a folder with itself or lose the row shots. Reproduce: in one worktree run spot_mac01_det with -Build <lane A>, then with -Build <lane B>; the first run's shots are gone.

## Goal
Each run keeps its shots in a folder named by scenario AND build (lane name, or the build-at name), e.g. build3ds/azt/shots/<scenario>/<lane>. Every reader (bench.py --shots, the byte compare helpers, test_catalog.md commands, any tool reading build3ds/azt/shots/<scenario>) uses the new path. One path only: the old folder layout is deleted, not kept as a fallback.

## Start state
Worktree build3ds/wt/shotlane, branch shotlane from nds1 (current). Tools only, no game source.

## Premise and its check
Check first: grep -rn "azt.shots\|azt\\shots\|azt/shots" 3ds/tools docs/handoff; list every writer and reader.

## Touch only
3ds/tools/azt.ps1, 3ds/tools/bench.py, other 3ds/tools readers the grep finds, docs/handoff/test_catalog.md (path text).

## Done criteria
1. azt.ps1 self-test (if it has one) passes.
2. Two runs of spot_mac01_det in one start sequence, -Build of two lanes (the main lane and this lane): both shot sets exist after the second run, in two folders; bench.py records each run's own --shots path.
3. A byte compare of the two folders gives equal (same source).
Affected scenarios (no others): spot_mac01_det.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=shotlane PM_ROW=shotlane. No git stash. No stubs. Commit after each passed step.

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "shotlane ready <hash>".
