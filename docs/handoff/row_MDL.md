# Row MDL: model and sprite record fallbacks go native

## Goal
nd_model_draw still falls back to rec_apply (walker) for some model records: why 2 (R13 aux words), why 3 (R11 custom gfx pre/post), why 6 (tex1/noise), why 15 (zmark+decal), why 11/12. Make them native, then delete the walker geometry fallback in rec_run. Plan: docs/handoff/plan_walker_end.md rows MDL-1 to MDL-4, decisions D2, D8 to D10.

## Start state
nds1 plus branch pm5 (8a00b82c4: PM5-3 done, why 4 = 0 at iwa_10, ndmcheck works: spot_iwa10_ndmcheck f600 diff 455 of 33026 keys). Work in a new worktree from branch pm5: git worktree add build3ds/wt/mdl -b mdl pm5; copy assets/us and ver/us/build; build there (PM_LANE=mdl); azt -Build <worktree 3dsx>.

## Rows (one commit each)
MDL-0 COUNT: one ndmcheck run each at iwa_10, mac_01, sbk_56, kmr_03 battle: print the ndm why counts and, for why 2, the R13 aux word forms (distinct w0 patterns and counts); for why 3, the R11 pre/post list addresses and first opcodes. Write the table into docs/handoff/agent_s6.md STATE (replace the stale MDL line). No code change except the debug print.
MDL-1 R13: decode the R13 aux forms (one form per commit) as aux_ok forms in nd_model.c. Gate: ndm why 2 = 0 at iwa_10, spot_mac01_pic byte-equal.
MDL-2 R11: custom gfx pre/post (why 3) run on the state side (setters / nd_words into nd_ws), then draw natively. Gate: why 3 = 0 where MDL-0 saw it, by eye.
MDL-3 SPRITE: only if MDL-0 or a census shows sprite_component or spr_fb walker draws: shaded sprite as the two-unit native form, imgfx meshes through the ring.
MDL-4 DELETE: pm_gpu_model_fb_cached and the walker geometry fallback in rec_run (`if (!nat) rec_dl`) only if fb = 0 in the MDL-0 scenes; else the remaining cases become named fallbacks with a counter in agent_s6.md.
Hand back with the branch head after MDL-4 or a failed gate.

## Rules
Own files: 3ds/src/nd_model.c (not the why 4 line), pm_gpu.c pm_gpu_model_native and pm_gpu_model_fb_cached, pm_gbi.c rec_run and load_shaded_sprite, nd_sprite.c. CORE edits pm_gbi.c on branch core (state renames, setters): keep pm_gbi.c hunks small. No toggles (debug switches allowed), no stubs, no push, no attribution lines, never commit 3ds/src/map_restore_table.c. Keep Azahar open. Status line in 3ds/logs/perf/status_MDL.txt. Budget: reset and continue. Two tries per named cause. Delete this file in the MDL-4 commit.
