# Agent S3N: map model records draw natively

## STATE
branch: nds1 | commits: 1f1839f2c (S3N-0), c48abe457 (S3N-1a gv move), 067a85de9 (S3N-2 nd_native_submit), 662e7d2ae (S3N-1b handle desc B4, render mode compile B5/B6), c8ce0207f + this doc commit (handoff)
done: S3N-0, S3N-1 (gates: nd_host PASS incl nd_gv and nd_model; spot_mac01 byte-equal after each game-side edit), S3N-2 (spot_mac01 byte-equal).
open: S3N-3 (nd_model_draw + ndmcheck) BLOCKED on B7. S3N-4, S3N-5 not started.
budget S3N: 5 of 8 Azahar starts used (baseline HEAD, mac01 x3, mac01 after S3N-2, mac01 after S3N-1b): 5 used. No reset.
next: user decision on B7, then S3N-3.

B7 (new, BLOCKS S3N-3): the combine mode and the TEV key of a draw come from sync_combine (pm_gbi.c 1081: sync_combine_work_timed, sync_legacy, decide, pm_comb_colours; memo scm_sig/scm_find). Its inputs include walker texture state (td[0..1].valid, tile formats and sizes, ldrec, tex_on, use_t1), box_pat, box_prod, box_folded, dual_on, comb_set, comb_alpha_shade, leg_mode. A native model draw has none of this. ndmcheck would compare a key the native path cannot build. Options: (a) port sync_legacy and decide into a pure function over an input struct the record and the handle desc fill (large; box and dual state need records too); (b) keep the walker combine for models and have nd_model_draw read the mode the walker last set (a stale-state risk); (c) S3N-3 compares only the parts natively derivable (render state, unit words, matrix, palette, geometry ranges) and logs combine and tev as skipped. Plan default NOT chosen: the user decides.

## Decisions (coordinator, 2026-11-05)
B4 DECIDED: option (a). NdModelTex from TextureHandle header (mainW, mainH, mainFmt, mainBitDepth, mainWrapW, mainWrapH, masks by INTEGER_LOG2, extraTiles, aux raster) plus raster and palette. One TARGET_3DS call nd_tex_handle_set(...) at the end of load_texture_impl (model.c). Common case only: extraTiles 0 (one tile), no aux raster, no mipmaps. Everything else fallback. ndmcheck compares NdUnit words with the walker words, so a wrong desc shows as a diff.
B5 DECIDED: plan default. NdMode is ok only when the list writes every bit nd_render_state reads (om_h bits 20 and 21 (cycle type); om_l bits 0,1,4,5,10,11,12,16-23,28-31 = mask 0xF0FF1C33). Otherwise fallback.
B6 (my default, not in plan): a render mode list with any op other than SETOTHERMODE_H/L or ENDDL is fallback (GEOMETRYMODE, SETCOMBINE, TEXTURE). Geometry mode from the walker state is not known natively.

## S3N-0 notes
- Sites removed: MODEL_RM_OP, MODEL_TEX_OP, MODEL_MTX_OP calls (3DS), REC_SKIP and recSkip writes (tint, combine, fog, shroud, panner, billboard), lights (record R3 w1 carries them), pipesync (recOn was always 1), custom gfx pre/post else-branches, geometry else-branch, gSPPopMatrix else-branch.
- Kept on 3DS: record writes, untextured ops (4 words, R13 aux), special tile list (make_texture_gfx, R13 aux), temp Gfx builders (not stream words).
- recOn and recSkip variables removed (both were 1 on 3DS). Macros and call sites are under #ifndef TARGET_3DS, so a 3DS build that reaches an old site fails to compile.
- Pre-existing: on non-3DS the MODEL_MTX_OP macro was not defined (nested #ifdef). Now defined under #ifndef TARGET_3DS.

## Blockers (plan default taken where noted)
1. Walker state is global. nd_raw_native (pm_gpu.c) reads combine, r_zcmp, r_zupd, r_atest, r_blend, plan_k, prim, cconst, pal_g, gidx and gni from walker globals. A record-driven nd_model_draw must set all of them from record data, or the draw key is wrong. Plan default: derive them from the compiled deltas (S3N-1), not from walker state.
2. Geometry is not a draw. Triangles reach the batch through G_VTX and G_TRI in the walker (gni, gidx). nd_model_draw needs the NdObj segments of the model geometry (nd_dlc, compiled at map load). Plan default: use dlc_try on the geometry list (rec_dl already does that); a model whose geometry is not compiled is a fallback class.
3. Merge (S3N-2) has no consumer until the record arm calls nd_model_draw (S3N-3). Plan default: build merge with S3N-2 and gate it at S3N-3.
4. Pictures (spot_*_pic) need Azahar starts. Not run here. They gate S3N-3 and later.

Plan of record for the remaining rows is unchanged. The next session starts at S3N-1.
