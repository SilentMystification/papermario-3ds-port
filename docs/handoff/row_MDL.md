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

## Oracle decision MDL-1 R13 (2026-11-06, Opus; replaces the nd_r13_desc sequence decoder)
Decision: run the R13 block through the dispatcher's texture handlers (the words are the record, D1/D2), and take unit 0 (and unit 1 if the combine uses tex1) from the state that results. Do not write a second decoder (nd_r13_desc as a sequence decoder breaks D2, and 3 loads plus 2 TLUTs plus tile 0 in between is exactly the walker's rule set). The counted fallback stays only for blocks that hold a non-texture op.
How:
- nd_model.c nd_model_state, why 2 path: if every op in the R13 block is in {SETTIMG, SETTILE, SETTILESIZE, LOADTILE, LOADBLOCK, LOADTLUT, the syncs, NOOP}, set aux_ok. Any other op keeps why 2, counted.
- pm_gbi.c rec_run, before pm_gpu_model_native: run the block on the walker handlers today (pm_gbi_run on the R13 words, state only); after TX-3, through nd_tex.c.
- Then pm_tex_native_unit0 / pm_tex_native_unit1 give the units from the live tile state.
- The model cache key (nd_mc) adds the bound slot ids (pm_tex_census_id of units 0 and 1), so a hit never reuses another frame's texture.
- Remove nd_r13_desc. Keep the r13all dump line until the gate passes, then delete it.
Gate:
- kmr_03 battle: ndm why 2 drops from 297 per 100 frames to 0 (non-texture ops counted apart).
- battle_start_pic byte-equal against the walker-fallback build (the same branch with aux_ok forced off).
- mac_01 byte-equal; iwa_10 by eye; model fb 0 in mdlw_battle.

## Rules
Own files: 3ds/src/nd_model.c (not the why 4 line), pm_gpu.c pm_gpu_model_native and pm_gpu_model_fb_cached, pm_gbi.c rec_run and load_shaded_sprite, nd_sprite.c. CORE edits pm_gbi.c on branch core (state renames, setters): keep pm_gbi.c hunks small. No toggles (debug switches allowed), no stubs, no push, no attribution lines, never commit 3ds/src/map_restore_table.c. Keep Azahar open. Status line in 3ds/logs/perf/status_MDL.txt. Budget: reset and continue. Two tries per named cause. Delete this file in the MDL-4 commit.

## Oracle review MDL-1 cache (2026-11-06, Opus; on branch mdl bba5bc3d8)
Keep the no-hit rule only for R13 texture-block records. The store change `nd_mc_store(mk, tex != 0, ...)` in bba5bc3d8 is too broad: every textured record without a handle desc becomes no-hit (aux_ok forms, gfx-list records, iwa_10 bound-slot cases), their hits fall back (ndn_texf) and the walker draws them: likely cause of iwa_10 not byte-equal and ndmcheck 208 -> 1619. No slot-id key: the key is built before the record is applied; on a hit rec_apply is skipped and pc jumps past the R13 words, so the block never runs.
Fix (one row): `nd_mc_store(mk, ds != NULL || (r13 && ds == NULL && gfx == NULL && tex), hs, hg)`; count those stores in the mdlw line as `r13nohit <n>`.
Gate: iwa_10 byte-equal against efaca195d, ndmcheck mismatches back to about 208; battle_start f182 byte-equal to r13off; mac_01 byte-equal; ndm hit/miss at mac_01 and iwa_10 equal to efaca195d; kmr_03 why 2 = 0 and r13nohit about 297 per 100 frames.
Revisit only if a console prof of a battle shows the R13 miss path above 0.5 ms/frame (then: run the block, compare the stored unit's pm_tex_census_id and the tex epoch in pm_gpu_model_hit).
