# agent_con: epic CON state (from plan_con_gate.md, console push build 1579, HEAD f56fc77ad)

Rules: CLAUDE.md 2, 5, 7; docs/handoff/hw_guidelines.md; plan_con_gate.md sections 4 and 5.

## Commits
- None. LB1b is INCOMPLETE (Azahar budget). MR1 not started.

## State
| Row | State | Change | Azahar mac01t_fsplit | Shots |
|---|---|---|---|---|
| LB1b | INCOMPLETE, code in tree, uncommitted | 3ds/src/pm_nusys.c: bg task (no swap, pm_interp_on <= 0) recorded in bg_list, run at the swap task after the main list build; pm_gfx_bg_flush at the end of pm_gfx_retrace and pm_gfx_retrace_skip | HEAD (lane conhead) and LB1b (lane conlb1b), run 1 each: log 4 of 4 each; tk 2.00 both; gwait 0.0 both | 2 of 2 top shots byte-equal (0000_1_2, 0001_1_2) |
| MR1 | NOT STARTED | G_PM_MODEL (plan_con_gate section 5) | n/a | n/a |

## LB1b gate status
- Done: HEAD run 1, LB1b run 1, 2 of 2 top shots byte-equal.
- Not done: HEAD-against-HEAD (run 2 of HEAD lane conhead). Plan requires it first.
- Not done: second pair (run 2 of LB1b). Gate as written is 2 of 2.
- Budget: LB1b row 8 of 8 starts (count is case-insensitive, so LB1B history counts too). Orchestrator must reset with `run_azahar.ps1 -ResetBudget LB1b` before the next start.

## Finding: fsplit labels shift (not a pixel change)
- HEAD: gbi 6.7 ms, rest 23.6 ms. LB1b: gbi 0.0 ms, rest 30.4 ms. Sum 30.3 and 30.4 ms: same frame time.
- Cause: pm_gpu_begin_wait (inside gbi_task) calls FS_MARK(FS_GWAIT) and FS_MARK(FS_REST). The deferred gbi now runs after the wait inside the swap task, so its work is charged to rest.
- Candidate fix (not built, not tested): in gbi_task (pm_nusys.c), add `FS_MARK(FS_GBI);` after `pm_gpu_begin_wait();`. Pixels do not change. Gate it with one fsplit run before commit.
- Console note: the console gbi share used by the MR1 gate (gbi 55 %) reads wrong until this fix is in.

## Open items
- Reset LB1b budget. Run HEAD lane conhead once more. Run LB1b lane conlb1b once more. Compare.
- Decide the fsplit label fix (above). If it is added, test it in the same run.
- Commit LB1b only after the gate passes (2 of 2 equal to HEAD, HEAD reproduces). If the shot differs, revert pm_nusys.c.
- MR1 after LB1b is committed or reverted. MR1 budget: 0 of 8 starts used.
- map_restore_table.c is the user file. Backup: C:\Users\Andrew\AppData\Local\Temp\claude\c--Users-Andrew-Documents-GitHub-papermario-3ds-port\d6d00d1c-7fdf-4fe7-b586-9f6c1655ea55\scratchpad\map_restore_table.user.c. It was restored after each build. Never commit it.
- Other uncommitted files in the tree belong to other work. Not mine.

## Commands
- Build: `/c/Python314/python 3ds/tools/gen_map_restore.py`, then `PM_ROW=LB1b sh 3ds/build.sh`, then restore map_restore_table.c from the backup above. Copy build3ds/pm_3ds.3dsx to build3ds/lane-<name>/.
- Lanes now present: build3ds/lane-conhead (HEAD f56fc77ad), build3ds/lane-conlb1b (LB1b, uncommitted diff).
- Test: `PM_ROW=LB1b powershell -File 3ds/tools/azt.ps1 mac01t_fsplit -Row LB1b -Build <lane> -Try`. Shots land in build3ds/azt/shots/mac01t_fsplit; copy them per run (the next run overwrites).
- Compare: `cmp` each top shot (name without bot_ prefix).
- Copies made: build3ds/azt/shots_conhead_A, build3ds/azt/shots_conlb1b_A.
