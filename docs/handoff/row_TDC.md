# Row TDC: tridrop census after DEL (dropped native triangles, whole runs)

## Goal
TRI-4 deleted the CPU triangle path. A triangle the ring path refuses is now dropped and counted on the `tridrop f<N> none .. scr .. tss .. lit .. fog .. zmark .. tex1 .. refuse .. t1fold ..` line (nd_geom.c nd_tridrop_log, printed by rect_log). The wcen census covered frames 0-300 only. This row reads the tridrop line over whole runs of 8 benches and names each non-zero field. Diagnostic row: no code change.

## Start state
Worktree build3ds/wt/tdc, branch tdc from nds1 (head after the fb4 merge c3e7afb19 plus this row file). Copy assets/us and ver/us/build into it.

## Premise and its check
Premise: the tridrop line prints in every rect_log window of a run. Check first (0 starts): grep -n "nd_tridrop_log" 3ds/src/*.c shows the call in rect_log. After the first run, grep -c "^.*tridrop f" the log is above 0; 0 lines: hand back at once.

## Benches (one run each, cold, lane tdc)
pause_spin_grab_det_w300, sbk56_orbdbg_det, battle_start_f182_cold, mdl5_battle_det, pond_mac00_pic, spot_sbk56_pic, tour10_det, golden_multi_det.

## Done criteria
1. Build OK on lane tdc.
2. For each bench: the sum of every tridrop field over the run (python over build3ds/lane-tdc/slot*/last_log.txt, or the log the bench names), and for each non-zero field the frame windows and the map.
3. Result table in this file under "## Result" (bench, map, field, sum, first frame). No fix in this row: each non-zero field becomes a fix row the orchestrator writes with the oracle (likely sources: GVR_LIT from lit_freeze nd_geom.c, texgen in an ep_t1f epoch, refuse).
Affected scenarios: the 8 benches above. No shots are judged (the counter is the quantity).

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. PM_ROW=TDC PM_LANE=tdc on every build.sh and bench call. Budget: 8 starts, reset once at the start (powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget TDC). No git stash. Wait for runs in the foreground. Do not edit runs.jsonl by hand.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=TDC PM_LANE=tdc sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=TDC PM_LANE=tdc python 3ds/tools/bench.py run <bench> --row TDC --lane tdc --why "<text>"

## At the end
Commit this file with the Result section (do not delete it: the orchestrator turns it into fix rows). Report in 5 lines.
