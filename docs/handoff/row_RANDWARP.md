# Row RANDWARP: boot to a random spot when no scene is under test

## User facts
User (2026-11-09): "unless a specific scenario mechanic or thing is being tested, when we go to boot the game into a level to test we
should boot to a random level instead of a specific one. When we need to test something in a level go to that one, but when not we
should go to a random spot."

## Goal
A debug switch `warp=random[:<seed>]` that warps to one random spot from the compiled spot table, reproducible from its seed, and the
tools use it as the default boot spot for runs that test no specific scene.

## Start state
Worktree build3ds/wt/RANDWARP, branch RANDWARP from main (hash in the first commit). PM_ROW=RANDWARP, PM_LANE=RANDWARP.

## Touch only
3ds/src/pm_warp.c (warp= parse, tour_spot at about 342, pm_tour_spots from pm_tour_lists.h at 358), 3ds/go.ps1, 3ds/push_3ds.sh if it
sets a default warp, 3ds/tools/azt_scenarios.json and bench_defs.json (only scenarios that test no specific scene), docs/handoff/test_catalog.md.

## Plan
1. pm_warp.c: `warp=random` or `warp=random:<seed>`. Seed: the given value, else svcGetSystemTick() low 32 bits. Pick spot index
   seed % PM_TOUR_SPOTS_N (one division, cold, once at boot) from pm_tour_spots (the outdoor surface spots of POS1, every map, no tst_).
   Use the tour story value (the one tourlist uses) unless story= is given. Log one line: `warp: random seed %u -> <map>:<entry>@x:y:z`,
   so any run can be repeated with warp=random:<seed>. Then the normal warp path with that spot (pos too).
2. go.ps1: push o/n/a without any warp= or tour= in the game args adds `warp=random`. A given warp= wins.
3. Scenarios: find azt_scenarios.json entries whose subject is not a scene (boot and memory line, menu/options, prof generic, tool
   self-tests) and that warp only to have a map loaded; switch them to warp=random. Scene, det/byte-equal, picture and perf-compare
   benches keep their fixed spot (a byte gate or a before/after comparison needs the same scene). List the changed ones in the commit.
4. test_catalog.md: one rule sentence: no scene under test -> warp=random; the run log seed line makes it repeatable.

## Done criteria
1. nd_host PASS.
2. One Azahar run with warp=random:<n>: the seed line prints, the shot shows Mario on that map. A second run with the same seed gives the
   same map line.
3. memcheck OK.
Affected scenarios (no others): the one random run.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash. Commit after each passed step. Never commit 3ds/src/map_restore_table.c.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "RANDWARP ready <hash>".
