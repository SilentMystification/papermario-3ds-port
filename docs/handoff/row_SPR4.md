# Row SPR4: battle sprites draw natively at the vertex word (no record mutation)

MDL-3 (merged cc10a9043): the last sprite fallback is battle sprites, sprnat why 4 = 555 per 300 frames at kmr_03. This row also covers the LS-3 no-mutation rule for G_PM_SPR (plan_s10_path.md D8).

## State (2026-11-07, after SPR4 agent 1)
Uncommitted work in build3ds/wt/spr4, saved as scratchpad spr4_wip.diff (this session's scratchpad). Steps 1-3 implemented (G_PM_SPR and G_VTX arms are in nd_walk.c after WK-1). Real bug fixed in the WIP: the flat sprite record ORed the vertex offset into w0 bits 0-15, over alpha bits 8-15; offset now bits 0-7 (src/sprite.c, include/gbi_custom.h, nd_walk.c). sprcheck kmr_03 f300: eq 732, diff 256: the walker key is the open batch state (g_cullm from batch-open geometry, pm_gpu.c ~2816), not the sprite's own state. Waiting for the oracle decision SPR4b.

## Oracle plan SPR4
What the native path cannot express. pm_gpu_spr_build (pm_gpu.c ~3610) builds the key from the walker's sticky state as it was before the record (pm_gbi_sticky: combine, geometry), merged with the record's rmList and final list (nd_sprite_mode). The walker draws the quad later, after the words that follow the record in spr_appendGfx_component_flat (src/sprite.c) have run: the opacity prim, SPR_RM_LIST, the NP1 texture words, and, with SET_VIEWPORT, the vpAlt pass with its own render mode, env, gDPSetCombineMode(PM_CC_0A / 0B, 0C) and its own triangles. In battle (the CONTEXT_BATTLE bit 4 written at sprite.c:431), those words set the state at the draw. So the record-time key is wrong (combine 8 vs 2, cull, alpha test), and spr_battle refuses (pm_gpu.c:3621).
Native form: build the sprite draw at the point where the walker reaches the record's vertex word. The state then comes from nd_ws live, as nd_raw_native does; the record supplies only the geometry, raster, palette, matrix and quad. This also satisfies the LS-3 no-mutation rule.
1. pm_gbi.c, G_PM_SPR case: stop writing G_NOOP into srec[k] and srec[k+1]. Store `spr_at = srec + k` and `spr_rec = srec`, then continue. Keep this case together with LS-3.
2. pm_gbi.c, G_VTX handler: if `pc - 1 == spr_at`, call pm_gpu_spr_native(spr_rec, live = 1). On success skip the vertex and triangle words (pc += 1) and clear spr_at; otherwise run them, which is the walker draw. The vpAlt pass's own vertex and triangle words run normally before that.
3. pm_gpu.c, pm_gpu_spr_build with live = 1: take the combine, geometry and other modes from nd_ws (comb_w0/w1, geom, omode_h/l, om_kh/kl, prim, env) with mode = the identity NdMode (the record's rmList and final list have already run on the walker). Drop nd_sprite_colors in this path (env and prim are live). Delete the flags & 4 refusal. The why 3 shaded case stays separate (its own row).
4. Remove the old record-time path once sprcheck shows 0 diffs on every scene. One path, no switch.
Gate:
- kmr_03 battle: sprnat why 4 = 0.
- sprcheck (spr_check key compare): diff 0 for battle sprites, with the walker key as reference.
- battle_start f182 byte-equal to the `nosprnat` (walker reference) build.
- mac_01, iwa_10 and pause_spin byte-equal (the vp and pause sprite path changes too).
- mdl/spr fallback counters: why 5 stays 0.

## Oracle decision SPR4b
The per-sprite live state is the N64-correct picture. The RDP has no batches: each triangle is drawn with the mode registers current when it runs. At the sprite's G_VTX those registers are nd_ws (geom 00200005, no cull; oml 00551238). A walker draw that uses the state of an earlier open batch (g_cullm set only when gni == 0, pm_gpu.c ~2816; render and combine taken when the batch opened) means a state change did not close the batch. That is a walker bug, not a reference to copy. So the native sprite keeps the per-sprite state, and sprcheck in its present form is not a valid gate (it compares against the buggy key).
Next, in the same row:
1. Name the missing batch break. At the sprite's G_VTX, log gni, the open batch's key (g_ct, g_cullm, gv_wind at open, the render key r_*, the combine and plan key) against nd_ws. Also log the ops run since the batch opened, as op codes. The first state op that changed a key field without a flush is the bug. The likely place is the batch-open test, gtrip_batch_open / grange_batch_open: it compares ct but not the winding that g_cullm also depends on, and it does not compare the render or combine key when those are synced lazily (sync_render at gv_epoch_enter).
2. Fix the walker: that test or that setter closes the open batch when the cull class, winding, render mode or combine key differs. One fix, in the owning module (TRI or ST, whichever owns the function by then).
3. sprcheck then compares like with like and must show diff 0.
Gate:
- battle_start f182: the native sprite build byte-equal to `nosprnat` built with the walker fix; the walker fix alone is checked by eye against an N64 reference shot of the same battle frame (ask the user for an N64 emulator capture, as for sb_effect4.png).
- mac_01, iwa_10 and pause_spin byte-equal to the pre-row build. If the walker fix changes any of them, each changed region is inspected: it should be the same mis-batched-state class, and is accepted only after that check.
- sprcheck diff 0 at kmr_03 battle f300; sprnat why 4 = 0.

## Rules
Own worktree build3ds/wt/spr4, branch spr4 from nds1; copy assets/us and ver/us/build. Code first (CLAUDE.md rule 5). pm_gbi.c is shared with ST/TX/GM/WK: keep your hunks to the G_PM_SPR case, the G_VTX hook and pm_gpu_spr_build; rebase before each commit and check markers. No attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. Delete this file in the last commit.

Gate change (user, 2026-11-07): no N64 reference shot. The walker fix is accepted by code reasoning (name the missing batch break and why the RDP would use the per-triangle state) plus the byte checks of unaffected scenes; any changed region in mac_01, iwa_10 or pause_spin is named by code as the same mis-batched-state class.
