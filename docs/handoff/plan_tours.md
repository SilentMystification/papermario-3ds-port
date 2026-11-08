# Plan TOURS: map tour, boss battle tour, perfect action cheat (oracle 2026-11-08, base nds1 5c84d16e7)

## User requirements (2026-11-08)
- Map tour: every map in ONE game session (no reboot), Azahar bench and console prof push. Fixed seed at each warp.
- A prof window opens after the map load settles and closes before the next warp: no window holds two maps. Time per warp: one setting (`tourtime=<s>`, default 4 s).
- The full tour finds the laggy maps. Curated list = laggy maps plus 2 warps per area. The curated tour is the epic-end "everything is good" check; the full tour runs only when the user asks.
- Battle tour: every boss (35, pm_boss_table.h) in one session, a few A presses each, fixed seed, cheat `cheats=action,hp`.
- Cheat "Perfect action commands" (Cheats tab, default Off): Mario AND partner actions, AND guard/block timing (user answers). Off unless the tab row or `cheats=action` sets it.
- Full tour story: one value, `story=ch0_toad_town` (user answer).

## Reuse
pm_warp.c: `tour=`/`tour=@file` (:90-97, 364-381), tour_step (:961-1000), pm_warp_world_mode (:942-956), TOUR_FRAMES 40 (:93, delete), pm_boss_launch (:715), bosstest= (:1282), bossend= (:894), boss_restore_tick. 3ds/sweep/boss.txt, autoa (pm_input.c:278). Seed: gRandSeed (src/43F0.c:28); fixedseed/det only fix the boot salt (pm_os.c:52). Prof: pf_report (pm_phase.c:419-457), 300-frame report (:505), scene drop (:411), pm_ph_scene_moved (:404); bench.py ingest drops windows across a scene change. Cheats: pm_dbg_game.c:177-215, pm_cheat_tick, tab rows pm_options.c:395/531/570/620. Badge: action_command_init_status (src/battle/action_cmd.c:255-284) sets acs->autoSucceed; check_block_input (src/battle/1A5830.c) does not read it.

## Design
- Words: `tourlist=all|curated|@file` (all = built from gAreas, entry 0, no tst_; curated = compiled header), `tourtime=<s>`, `battletour=all|i,j,..`, cheat token `action`.
- Map state machine in pm_warp.c (pm_warp_tick): WARP (gRandSeed = PM_TOUR_SEED; tour_step) -> SETTLE (constant frames from the check of the load gap, no window) -> MEASURE (pm_ph_window_open) -> CLOSE (pm_ph_window_close: pf_report with pf_w < 300, line `prof f<n> w<n> tour <i>/<n> <map>:<entry>`). End line `tour: done items <n>`. In tour mode pm_phase's 300-frame report and scene drop are off. No tour: no change.
- Battle state machine: LAUNCH (seed; pm_boss_launch) -> WAITBATTLE (GAME_MODE_BATTLE, 20 s timeout -> `battletour: <label> FAIL`, next) -> PLAY (window open, autoa for tourtime) -> END (btl_set_state(BATTLE_STATE_END_BATTLE), bossend made a function) -> RESTORE (boss_restore_tick to world + 10 frames) -> CLOSE (`battle <i>/<n> <label>`). End line `battletour: done`.
- Cheat: bit CHB_ACTION (pm_dbg_game.c), tab row, `cheats=action`. `#ifdef TARGET_3DS` hook at the end of action_command_init_status: autoSucceed = true (showHud unchanged), Mario and partner; one line in check_block_input for a perfect guard.
- Perf DB: bench.py ingest reads the tour lines, one window record per map with tour_idx and kind (map/battle). Tags tour_full, tour_curated, tour_battle. `bench.py tour-report <run>`: one row per map (ft_ms, wk, o34, gbi, draws, tris), ranked by ft_ms.
- Curated: `bench.py tour-curate <run> [--slow-ms 33.3] [--top 20] [--per-area 2]` -> 3ds/tours/curated.txt (header: source run and build). gen_tour_lists.py -> 3ds/src/pm_tour_lists.h (CMake regenerates).

## Rows (order)
1. TOUR-1 (Opus): window API, map state machine, tourtime, settle, seed, tourlist=all; delete TOUR_FRAMES.
2. CHEAT-A (Haiku, src line reviewed by the orchestrator): cheat bit, row, token, hooks. Parallel to TOUR-1.
3. TOUR-2 (Haiku): ingest of tour lines, tour-report, benches tour_full / tour_full_prof, scenario tour_full (soak cap with reason).
4. BT-1 (Opus): battle tour, benches tour_battle / tour_battle_prof.
5. CUR-1 (Haiku): tour-curate, gen_tour_lists.py, 3ds/tours/, tourlist=curated, benches tour_curated / tour_curated_prof.
6. Full run (user asks): tour_full in Azahar, tour_full_prof on the console, then tour-curate and commit curated.txt.
7. Docs (orchestrator): test_catalog.md "Tours" section; rule 5 wording to the user.
