# agent_rc1: RC1 (record capture driver), state at the stop

Plan: docs/handoff/plan_record_capture.md row RC1. Base: HEAD 14456994d (source equal to 821b01baf).
Status: code built, not committed. Picture gate not run (blocked on Azahar).

## Changed files (uncommitted, uncommitted until the gate passes)
- src/model.c: `plain` hoisted out of the PM_ROW census block. Header w1 bit 9 (0x200) = plain && RENDER_MODE_SURFACE_OPA.
  New accessor `pm_rec_ver(slot)` at the end of the file (3DS only). Reason: pm_gbi.c does not include model.h.
- 3ds/include/pm_gpu.h: prototypes pm_gpu_rec_begin, pm_gpu_rec_end. The plan named src/pm_gpu.h; the real header is 3ds/include/pm_gpu.h.
  The signature uses unsigned long (u32 is unsigned long here; u32 is not declared in that header).
- 3ds/src/pm_gpu.c: statics rec_cap, rec_ref, rb_*; noise refuse (`if (rec_cap) rec_ref = 1;`); pm_gpu_rec_begin, pm_gpu_rec_end.
- 3ds/src/pm_gbi.c: RecKey, arena (256 KB malloc once), rec_try, rec_done (walker), rec_report (every 300 frames in
  pm_gbi_end_frame; the plan named a dr1 log site, which does not exist in the tree). G_PM_MODEL case: rec_try before mg_run,
  rec_done after.

## Deviations from the plan (all small)
- Walker extra-length mask is 0x7ff (bits 20 to 30, the PICA field), not 0xff.
- The rec line is printed by rec_report every 300 frames, the same cadence as dlc_report.
- pm_gbi.c and pm_gpu.c each keep their own `rec_ref` (static, file scope).

## Build
- Build OK, exit 0. No warning in changed lines (log: 3ds/logs is not kept; scratch copy rc1_build3.log).
- Command: `python 3ds/tools/gen_map_restore.py`, `sh 3ds/build.sh`, then `cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c`.
  Never commit 3ds/src/map_restore_table.c.

## Gate (not run)
- HEAD baseline: source at HEAD equals the archive 3ds/logs/builds/pm_3ds_1646_821b01baf.3dsx. Staged as build3ds/lane-rc1h/pm_3ds.3dsx.
  Script: scratchpad run_rc1_head.ps1 (4 starts: mac01t_cap1 x2, yard x2, PM_ROW=RC1H, `-Build rc1h`).
- Start 1 was refused: Azahar PID 43012 already running (started 11:38, not from azt, no pid file). Not killed.
- Pending: 4 HEAD starts (PM_ROW=RC1H), then 2 RC1 starts (PM_ROW=RC1, default build3ds/pm_3ds.3dsx).
  Justification line before each start in 3ds/logs/perf/status_RC1H.txt and status_RC1.txt.

## Next steps
1. Stop the other Azahar process, or wait until it exits.
2. Run the HEAD pair per scene; top shots must match in each pair. Not equal: stop, report the scene.
3. Run RC1 on mac01t_cap1 and yard. Top shots must equal the HEAD shots. The rec line: build > 0, wbad 0, cap < 5 in window 2 (f600 line).
4. Pass: commit the four source files and this doc (no map_restore_table.c). Fail: revert the four files with Edit.

## Commands
- Build: see above. Start: `PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 <scene> -Row <row> -Build rc1h -Try` (HEAD) or without -Build (RC1).
- Rec line: grep `rec f` in build3ds/lane-rc1h/last_log.txt (HEAD has none) or build3ds/last_log.txt (RC1).

## Result of the gate run (2026-11-01)
- HEAD pairs (PM_ROW=RC1H, 4 starts): top shots byte-equal in both scenes (mac01t_cap1 0000_1_2.ppm, yard 0000_0_11.ppm).
- RC1 (PM_ROW=RC1, 2 starts): top shots byte-equal to HEAD in both scenes.
- rec line, yard: `rec f349 cap 48 build 9 ref 0 part 0 emp 39 wbad 0 kb 6 n 9`. Build 9 > 0, wbad 0.
- Window 2 (f600) was not reached: the yard until is f300, mac01t_cap1 until is shot frame 504. Cap in window 2 is not measured. The mac01t_cap1 rec line is not in the copied logs.
- Gate not passed (window 2 open). Code is NOT committed. Next: a run that reaches f600 on yard or mac01t_cap1 (needs a scenario until change, which goes through azt_scenarios.json and the catalog), or the orchestrator's decision.
- Note: cap 48 in window 1 with emp 39 means most captures are empty (models out of view).
