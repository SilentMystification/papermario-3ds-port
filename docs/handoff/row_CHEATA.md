# Row CHEATA: cheat "Perfect action commands"

## Goal
A Cheats tab row "Perfect action commands" (default Off) and the launch token `cheats=action`. When on: every action command of Mario AND the partner succeeds, and every guard/block succeeds (user answers 2026-11-08). Plan: docs/handoff/plan_tours.md (Design, Cheat).

## Start state
Worktree build3ds/wt/cheata, branch cheata from nds1 7bbd2dfee+. Copy assets/us and ver/us/build into it.

## Premise and its check (oracle plans)
Premise: setting acs->autoSucceed at the end of action_command_init_status (src/battle/action_cmd.c:255-284) gives the same result as the Right On! badge, and also works for partner commands. Check first: grep the callers of action_command_init_status (partner actions must call it too) and read how autoSucceed is read in the action command scripts. Guard: check_block_input (src/battle/1A5830.c) does not read autoSucceed: it needs its own line (return a success when the cheat is on).

## Touch only
3ds/src/pm_dbg_game.c (bit CHB_ACTION, cheats= token `action`), 3ds/src/pm_options.c (Cheats tab row, as the other cheat rows), src/battle/action_cmd.c and src/battle/1A5830.c (one `#ifdef TARGET_3DS` line each, calling a pm_cheat getter). Leave showHud unchanged.

## Done criteria
1. Build; nd_host PASS.
2. battle_start_f182_cold byte-equal to the base with the cheat off.
3. One Azahar battle with `cheats=action` logs `cheat: perfect action commands active` and a hammer or jump result of Nice or Excellent in the log; one partner attack also succeeds; one enemy attack shows a guard success.
Affected scenarios (no others): battle_start_f182_cold, one battle scenario with cheats=action (add it to azt_scenarios.json and a bench to bench_defs.json in the same commit if none exists).

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=cheata PM_ROW=cheata on every build.sh and bench call. No git stash. No toggles, no stubs. Do not edit src/msg.c. Commit after each passed step.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=cheata PM_LANE=cheata sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=cheata PM_LANE=cheata python 3ds/tools/bench.py run <bench> --row cheata --lane cheata --why "<text>"
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget cheata

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "cheata ready <hash>".
