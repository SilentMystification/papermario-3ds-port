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

## Overnight order (user asleep, 2026-10-29): do everything, do not stop on a blocker that has unrelated work
- Lane 3 (starts when lane 1 is done, shares pm_gpu.c): intro cutscene box (diag_cutscene_box.md, merged boxlog); verify the earlier text fixes with catalog scenarios: periods/spacing and repeated letters in menus, the 4x tiled textures (Mario House sign, intro SUPER text), the file select rotation glitch. Anything unreproducible is marked unverified with the shots, not skipped silently.
- Test catalog (azt.ps1, test_catalog.md) must exist and work by the end; CLAUDE.md rule 5 then points agents to it.
- When lanes 1 to 3 are done: final agent runs the catalog on the merged HEAD (short scenarios, one completion run), merges to main, builds lane final, archives the 3dsx (pushed_builds.txt), and writes docs/handoff/FINAL_WRITEUP.md: everything done (commits by area), what was dropped or reverted and why, open items, the console checklist (what to see on the hardware, with the expected numbers), the push command. The user pushes in the morning.
