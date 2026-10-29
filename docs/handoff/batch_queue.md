# Batch queue: everything before the one console push

Rule from the user (2026-10-29): all work is done and tested on Azahar before the console push. Anything that needs hardware gets its functionality in and tested on Azahar first. One final build with all changes and the performance counters goes to the console. Quick tests use short tours; the full 411-map run is the completion test only. No culling. src/ edits are allowed for optimization (3DS-guarded, smallest swap, picture identical).

Gates on a console number are replaced by the Azahar direction: a row is kept when the Azahar walk (or its quantity) drops, the picture is identical, and the counter for the row is in the final build for the console to confirm.

## Lane 1: texture and VRAM (pm_tex.c, pm_gpu.c, pm_mem.c)
Order: row 0 failure log, 0b (stale texture fix, committed a56ee72c4), 1 linear memory, 2 per-bank accounting, 3 boot layout and 3 MB cache, 4 full buffers, 5 shrink and regrow. Plan: plan_vram_3mb.md. Completion test: one 411-map run after row 5.

## Lane 2: display list and walk (pm_gbi.c, pm_interp.c, pm_dlc.inc.c, src/model.c and other producers)
Order: W1 op timer, P1 producer census, C1 native texture bind, C2 next producer from P1, volcano fog fix (A, diag_visual_vol_sewer.md), W4 + W2 dispatch table and sampled instruments, W3 matrix hash, W6 I-cache flags, pause menu lag row (diag_pause_closes.md), then the Opus plan for the route to 33 ms from the C1 and P1 numbers. Plans: plan_dl_architecture.md, plan_walk_cost.md.

## Final steps
1. All rows committed and merged in main, no uncommitted files except map_restore_table.c and its .bak.
2. One Azahar short tour plus the effect scripts on the final build, pictures equal.
3. Build lane final, archive to 3ds/logs/builds, line in pushed_builds.txt (picture ok).
4. Push command to the user (rule 6) with the console checklist: `rom mode` boot line; HUD VRAM A/B min and linear; `opsplit f` and `prod f` lines; texmap line (inval/lookup hash ms); `ui:` markers for pause, partner and file select; the lid-close steps in diag_sleep_hang.md; frame time at the yard against 58 ms and the 33.4 ms target.
