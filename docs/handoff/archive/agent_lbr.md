# Handoff: LBR epic (LB1, R5B, R2B)

## State
- LB1 (lazy FrameBegin): dropped. Patch: 3ds/logs/perf/lb1_dropped.diff. Tree reverted with Edit.
- R5B: committed b1d5d425e (3ds/src/pm_gbi.c).
- R2B: committed f230e46c6 (3ds/src/pm_gbi.c, 3ds/src/pm_gpu.c, 3ds/include/pm_gpu.h).
- Final build: 3ds/logs/builds/pm_3ds_final8_f230e46c6.3dsx, sha256 3c50c58f1761c7c3b33dc38614008072137d2262b19febd22719d9e9382d939b. Pushed: no.
- Status: 3ds/logs/perf/status_LBR.txt.

## LB1 (dropped) numbers
- Lane lb1 (built from the LB1 edits, pre-R5B tree). Gate: top shots byte-equal.
- mac01 2 of 2, yard 1 of 1, pause_menu 1 of 1: equal.
- mac01t_fsplit run 1: 0 of 2 equal. fs1 skipped 18 of 900 steps (HEAD-era runs: 16). Scene state at frame 500 differs (inputDisabled 1 vs 0).
- mac01t_fsplit run 2 (same binary): 2 of 2 equal. So the run 1 mismatch is run timing, not picture.
- battle_start: shots 0000 to 0003 equal, 0004 to 0006 differ (32129, 30255 and 1587 px). Baseline folder had 9 shots, this run 7. Not resolved; no start left.
- mac01t (non-fsplit) not run: no start left.
- fsplit gwait on Azahar: 0.0 ms in every run. Azahar does not show the C3D_FrameBegin wait.
- Gate missed, row undone.
- Open: a lazy begin needs a picture check that is not timing-bound (fixed step count or a fixed-frame capture). The FS1 skip count changes the scene at frame 500 in mac01t.

## R5B numbers
- mac01 lt 160 and lt_same 160 per frame at the baseline. After the change lt 0 (all equal lights skipped before the handler). Each skipped MOVEMEM needed lit_pending == 0 (lit_freeze side effect).
- mac01 2 of 2 top shots equal. mac01t 2 of 2 equal.
- mdlsum f899 tot ops 1524 (HT1 baseline 1844). The skipped ops leave the count. The scenario range (1623 to 2065) needs an update. Not changed here (azt_scenarios.json belongs to BATEND).

## R2B numbers
- Skips: SETCOMBINE (comb_gen, sc_sig_now), TEXTURE (sc_sig_now), GEOMETRYMODE (pm_gpu_mode_is).
- Pictures: mac01t 2 of 2, yard 1 of 1, pause_menu 1 of 1, mac01 2 of 2 equal.
- Timing, mac01 f299, one window per build: pre-R2B lane cc 597 us + geo 508 us = 1105 us; R2B cc 518 + geo 488 = 1006 us. Other classes moved by 1 to 2 percent between the runs (0a 2017 vs 1984 us).
- mdlsum f899 tot ops 1335. Range update needed (see R5B).

## Azahar starts
- LB1: 6 of 6 (mac01t_fsplit x2, mac01, yard, pause_menu, battle_start).
- R5B: 2 of 6 (mac01, mac01t).
- R2B: 5 of 6 (mac01t, yard, pause_menu, mac01, mac01 on lane r5b).

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=<row> PM_LANE=<lane> PM_BUILD_CAP=900 sh 3ds/build.sh; cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c.
- Test: PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build <lane>.
- Compare: python 3ds/tools/shot_cmp.py <base dir> build3ds/azt/shots/<scenario>. Baselines used: copies in the session scratchpad (lb1_base), made from the shot folders of 14:42 to 16:46 runs of the same scenario.

## Next steps
1. Orchestrator: set the mac01t mdlsum range for R5B and R2B tree (about 1335 to 1524 per frame now).
2. LB1 retry needs a timing-fixed capture (fixed step count) or a battle_start baseline from HEAD on the same run.
3. Console proves the cc/geo and light changes; push with the line in docs/handoff/FINAL_WRITEUP.md after the orchestrator check.
