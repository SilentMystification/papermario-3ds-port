# Row TRI: CPU triangles, wide edge, zmark go native

## Goal
Remove walker draws from wide-edge ortho quads (entity_model wd 630 at iwa_10), zmark draws, and CPU-clipped triangles, then delete the CPU triangle path. Plan: docs/handoff/plan_walker_end.md (read Decisions, Streams and ownership, Rows TRI-1 to TRI-4).

## Start state
Branch nds1, HEAD 3f6ce7a01. WIDE1 (ecbe5aa54, 1cfb15e2d, 070092ae8) left partial-edge quads as a named fallback (wide_edge_k, counters interp and c14).

## Rows (in order, one commit each)
TRI-2 WIDE first (needs nothing): plan row TRI-2. Use the CPU edge rule exactly (grep `F07-7: a rectangle on the game edge` and wide_edge_k in pm_gpu.c).
TRI-1 ZMARK: only after the CEN-1b commit is in git log (grep the log for "CEN-1b"). If it is not in yet, do TRI-3 first.
TRI-3 CLIP: plan row TRI-3. The why codes tri_cpu_clip and tri_cpu_seam come from CEN-1a (docs/handoff/agent_wcen.md table); if the table is not in yet, count seam_obj and seam_clip yourself with a debug switch.
TRI-4 DELETE: only when the orchestrator tells you that RECT-3, OFF-4 and PM5-4 are in (D6). Hand back after TRI-3 with TRI-4 open if they are not.

## Touch only
3ds/src/pm_gbi.c draw_tri, gv_tri, draw_tri_cpu, off_screen_check; 3ds/src/pm_gpu.c pm_gpu_tri3d, pm_gpu_vert_clip, wide_edge_k and its callers' vertex copy, pm_gpu_set_zmark, the zmark and nozt refusal lines of nd_raw_native; 3ds/src/nd_model.c only the why 15 case (TRI-1); own hunks of azt_scenarios.json, bench_defs.json.

## Done criteria
1. TRI-2: entity_model wd at iwa_10 from 630 to about 0 (interp 0); wide HUD right by eye at battle_wide and pause_spin_wide.
2. TRI-1: zmark why 0 where censused; by eye; spot_mac01_pic byte-equal.
3. TRI-3: seam counters 0; the intro pan (V18 scene) right by eye.

## Rules
CLAUDE.md rules 5, 7, 9. Decisions D1 to D10 of docs/handoff/plan_walker_end.md are fixed. Other agents edit pm_gpu.c, pm_gbi.c, pm_tex.c, nd_*.c at the same time: edit only the functions your stream owns (plan table "Streams and ownership"); commit only your hunks through your own temporary index (GIT_INDEX_FILE copied from HEAD), never the shared index; no git add -A, commit -a, reset, stash, checkout, restore. Build in a scratch tree from HEAD plus your diff (git archive HEAD, apply your diff, copy assets/us and ver/us/build, gen_map_restore.py, copy 3ds/src/map_restore_table.c), lane named after your row; azt.ps1 <scenario> -Row <row> -Build <lane>. After each code commit build the committed tree clean. Status line in 3ds/logs/perf/status_<row>.txt before each Azahar start. Budget: reset (powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <row>) and continue. No toggles (debug switches allowed, removed in the deletion row), no stubs, no push, no attribution lines, never commit 3ds/src/map_restore_table.c. Two tries per named cause, then the next code point. A new scenario goes into azt_scenarios.json and bench_defs.json in the same commit. Delete this file in the commit that completes the last row.
