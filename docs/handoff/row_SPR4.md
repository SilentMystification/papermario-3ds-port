# Row SPR4: battle sprites draw natively at the vertex word (no record mutation)

MDL-3 (merged cc10a9043): the last sprite fallback is battle sprites, sprnat why 4 = 555 per 300 frames at kmr_03. This row also covers the LS-3 no-mutation rule for G_PM_SPR (plan_s10_path.md D8).

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

## Rules
Own worktree build3ds/wt/spr4, branch spr4 from nds1; copy assets/us and ver/us/build. Code first (CLAUDE.md rule 5). pm_gbi.c is shared with ST/TX/GM/WK: keep your hunks to the G_PM_SPR case, the G_VTX hook and pm_gpu_spr_build; rebase before each commit and check markers. No attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. Delete this file in the last commit.
