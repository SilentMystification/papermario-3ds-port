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

## Result
Runs on lane tdc, build 2909, commit d8864ae48, one start per bench (8 starts; the golden run first refused with SPEED CONFLICT, nothing ran; retry with --speed turbo). Sums are over every tridrop line of the run log (build3ds/azt/logs/<scenario>.txt copy). Windows are rect_log calls (every 100 frames; f300 printed twice in golden, two sums added).

| bench | map | field | sum | first frame |
|---|---|---|---|---|
| pause_spin_grab_det_w300 | pause spin (grab) | all 0 | 0 | - |
| sbk56_orbdbg_det | sbk_56 | all 0 | 0 | - |
| battle_start_f182_cold | kmr_03 | all 0 | 0 | - |
| mdl5_battle_det | kmr_03 | all 0 | 0 | - |
| pond_mac00_pic | mac_00 | no tridrop line (0 windows) | - | - |
| spot_sbk56_pic | sbk_56 | no tridrop line (0 windows) | - | - |
| tour10_det | 10-map tour | all 0 (14 windows, f0..f1000) | 0 | - |
| golden_multi_det | isk_04 (f300), kzn_19 (f400) | t1fold | 573 (61 + 5 at f300, 507 at f400) | f300 |

Notes:
- pond_mac00_pic and spot_sbk56_pic use prof=4, which sets prof=1 log=off (scenario debug text). The rect_log lines (pm_log_d) are not in the log, so no count. The premise holds only for benches with the plain log. A bench for these spots with the plain log is needed to count them: the orchestrator decides.
- golden_multi_det: only non-zero field is t1fold (texture 1 fold, nd_comb.c t1_fold_tri). Map at f300 is isk_04 (map change #211, tour in #228), at f400 kzn_19 (map change #266). Other fields 0 in every window.
- No code change in this row. Fix row candidates: t1fold source in isk_04 and kzn_19 (not a drop count of refuse or lit).
