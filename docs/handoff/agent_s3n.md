# Agent S3N: map model records draw natively

## STATE
branch: nds1 | S3N-3a committed (48dc9ed53, nd_model_state + host case 6). S3N-3b committed (ndmcheck switch, pm_gpu.c ndm_*, pm_gbi.c rec_run arm).
done: S3N-0, S3N-1, S3N-2, S3N-3a, S3N-3b code (the tool works: prints ndmcheck lines in prof=4 via pm_log_prof_line).
3b gate NOT met in substance: spot_mac01 run (Azahar start 8 of 8, S3N budget at cap, not reset): `ndmcheck f300 recs 38886 eq 0 diff 0 fb 38886 legacy 0 nodraw 0`.
  diff 0 is vacuous: every record falls back (B8), so no draw was compared. spot_mac00 and spot_sbk56 NOT run.
open: B8 (blocks a real 3b gate, 3c validation and S3N-4). 3c NOT started (its gate cannot be tested under B8). S3N-4, S3N-5 not started.
budget S3N: 8 of 8 used (at cap, no reset). Reset only with the coordinator decision on B8.
next: user decision on B8, then 3b gate on the 3 spots, then 3c.

B8 (new, BLOCKS 3b gate, 3c, S3N-4): the plan rule B5 (om_h bits 20-21 and om_l mask 0xF0FF1C33 must all be written by the render list) fails for every model render list. nd_render_state reads om_l bits 0, 1 (alpha compare), 4, 5, 10-12, 16-23, 28-31 and the om_h cycle type. A list from gDPSetRenderMode writes bits 3-31 of om_l only and no om_h cycle type, so nd_mode_compile returns ok 0 and the record falls back. Measured: 38886 of 38886 records fallback at mac_01 (300 frames).
  Options for the user:
  (a) producer change (src/model.c): the record writes the full om_h and om_l words the model needs (cycle type, alpha compare, all read bits). Pixel risk: the walker state the model currently inherits must equal the defaults written. Needs picture check.
  (b) walker sticky values for unwritten bits (ctx from the walker at record entry, not native). Keeps the walker state dependence the plan wants gone.
  (c) narrow the read mask to the bits every model list writes, and prove the rest is constant for models (needs a walker scan of all ModelRenderModes lists).
  Plan default (B5 as written): fallback. Taken for now: this is the default and it keeps the 3b check honest (no native draw without the state). Not chosen: any of (a) to (c) without the user.

B7 (new, BLOCKS S3N-3): the combine mode and the TEV key of a draw come from sync_combine (pm_gbi.c 1081: sync_combine_work_timed, sync_legacy, decide, pm_comb_colours; memo scm_sig/scm_find). Its inputs include walker texture state (td[0..1].valid, tile formats and sizes, ldrec, tex_on, use_t1), box_pat, box_prod, box_folded, dual_on, comb_set, comb_alpha_shade, leg_mode. A native model draw has none of this. ndmcheck would compare a key the native path cannot build. Options: (a) port sync_legacy and decide into a pure function over an input struct the record and the handle desc fill (large; box and dual state need records too); (b) keep the walker combine for models and have nd_model_draw read the mode the walker last set (a stale-state risk); (c) S3N-3 compares only the parts natively derivable (render state, unit words, matrix, palette, geometry ranges) and logs combine and tev as skipped. Plan default NOT chosen: the user decides.

B7 DECIDED (coordinator, plan_native_system.md 2.3): native model draws always use combine 8. The TEV plan comes from pm_comb_build over the record combine words (R4 entry w0, w1; two = 2CYC class; t1only as nd_raw_native sets it). Colours from the record (prim, env, fog, blend). No port of sync_legacy, decide, box or dual. Fallback when pm_comb_build refuses (rc != 0) or the plan uses tex1 or noise. ndmcheck compares all fields including combine and TEV. A diff where the walker chose a legacy mode is logged as class `legacy` (counted apart, not a failure). The gate for those models is the picture by eye in S3N-4.

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
