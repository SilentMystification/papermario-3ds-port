# Row BT1: boss battle tour in one session

## Goal
`battletour=all|i,j,..` runs every boss of pm_boss_table.h (35) in one game session: fixed seed, launch, a few seconds of play with A presses, end, restore, next. One prof window per battle. Plan: docs/handoff/plan_tours.md (Design, Battle state machine; row 4). The perfect action cheat (row CHEATA, may land during this row) is set by the scenario with `cheats=action,hp`.

## Start state
Worktree build3ds/wt/bt1, branch bt1 from nds1 d48d62cf2+. Copy assets/us and ver/us/build into it. The TOUR1 engine (pm_warp.c state machine, pm_ph_window_open/close, pm_ph_tour_mode) is merged: reuse it.

## Premise and its check
Premise: pm_boss_launch(i) (pm_warp.c) reaches GAME_MODE_BATTLE from the world for each boss in one session, and the bossend path (BATTLE_STATE_END_BATTLE) plus boss_restore_tick returns to the world. Check first: in the first Azahar start, battletour=0,17,34; all three reach battle and return. If one cannot launch from the world, hand back with the boss and the line.

## Touch only
3ds/src/pm_warp.c (battle states LAUNCH, WAITBATTLE (20 s timeout -> `battletour: <label> FAIL`, next), PLAY (window open, autoa for tourtime), END (bossend made a function), RESTORE (to world + 10 frames), CLOSE (`battle <i>/<n> <label>`)), end line `battletour: done items <n> fail <k>`; azt_scenarios.json and bench_defs.json: tour_battle (Azahar, `battletour=all tourtime=4 cheats=action,hp autoa prof=4`, cap with a stated soak reason) and tour_battle_prof (console). Seed: gRandSeed = TOUR_SEED at LAUNCH.

## Done criteria
1. Build; nd_host PASS.
2. tour_battle reaches `battletour: done items 35` in one start, fail 0, or each FAIL is named with its cause traced in the code.
3. spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015 byte-equal to the base (battletour off there).
4. One shot inside one battle window shows the battle scene.
Affected scenarios (no others): tour_battle, a 3-boss check, the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=bt1 PM_ROW=bt1 on every build.sh and bench call. No git stash. No toggles, no stubs. Commit after each passed step.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=bt1 PM_LANE=bt1 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=bt1 PM_LANE=bt1 python 3ds/tools/bench.py run <bench> --row bt1 --lane bt1 --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "bt1 ready <hash>".
