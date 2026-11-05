# Agent S3N: map model records draw natively

## STATE
branch: nds1 | commits: 48dc9ed53 (3a), 7e80630bb (3b), 0cc27f058 (B8), 6f7901630 (handoff), be480ae1d (B9 list ops), e859fd38b (begin classification).
done: S3N-0, S3N-1, S3N-2, S3N-3a, S3N-3b code: B8 om merge + nost, B9 list ops (SETCOMBINE, TEXTURE, GEOMETRYMODE) into NdMode, geom/z/cull from geom, uvso/cmul/cadd/zmix compare.
3b gate NOT met: eq 0 on all 3 spots. Benches spot_*_ndmcheck (committed 5d151eb12), budget reset at start, 6 of 8 used:
  mac_01 (bench, e859fd38b): recs 38886 eq 0 diff 0 fb 3154 legacy 19052 nodraw 16680 nost 0 badop none
  mac_00: recs 44608 eq 0 diff 0 fb 3243 legacy 16972 nodraw 24393 nost 0
  sbk_56: recs 23221 eq 0 diff 0 fb 0 legacy 13122 nodraw 10099 nost 0
  diff 0 is not proof: eq 0 means no draw was compared. The scene line event failed on mac_01 (prof line before the ndmcheck line; coordinator says read the line yourself, do not edit the scenario): reported.
3c NOT started.
next: decide B10, then 3b gate and 3c.

B10 (new, BLOCKS 3b gate): the walker draws in these records are almost all legacy class (walker combine != 8, B7: counted, not compared). Native compare is only on combine 8 (plan). The walker never picks combine 8 for these models, so eq stays 0. Also nodraw is large: records whose draws go to flush_draw_cpu or other non-raw paths, not compared.
  Options for the user:
  (a) compare the non-TEV fields for legacy class draws (depth, blend, tex, cull, u0, uvso, cmul, cadd, zmix; not tmpl id, TEV words, combine). Gives eq for legacy class with partial compare. Walker combine stays the oracle for those fields only.
  (b) force combine 8 on the walker for these draws (picture change, not allowed without a check).
  (c) count legacy as its own gate (picture by eye in S3N-4), and set the 3b gate to eq above 0 on combine 8 draws only (none exist here: gate impossible).
  Plan default (B7 as written): legacy counted apart, not compared. Taken for now.

B8 DECIDED (coordinator): om_l = (nd_st.om_l & ~written) | list bits; om_h likewise. Read bits with no nd_st value: nost (-1). Done in nd_model_state (merge) and nd_state.c (om_kh, om_kl known masks).
  Deviation: cycle type from the merged om_h (not "2CYC flag of the record class"). R3 has no 2CYC flag; the merged cycle type is the only source. ndmcheck would show a wrong cycle type as diff.
  uvso, cmul, cadd, zmix compared: native gv = gv_params_calc (pm_gbi.c, now non-static) with tss from nd_st tex_s, tex_t; the other gv inputs are walker state at compare time (not independent).

B9 (new, BLOCKS 3b gate, 3c): nd_mode_compile (B6, my default) rejects any render list op other than SETOTHERMODE_H/L. The walker's rm_apply_impl (pm_gbi.c) also accepts G_SETCOMBINE, G_TEXTURE and G_GEOMETRYMODE in a render list. Most model render lists hold those ops, so the record falls back at begin (fb 100% at 3 spots).
  Consequence for a native draw: a SETCOMBINE in the list replaces the record combine entry (rec_apply runs the entry first, then rm_apply), so B7 (combine from the record) is wrong for such lists. A TEXTURE op changes the texture state. A GEOMETRYMODE op changes z and cull and lighting, which are the geometry state the record does not carry.
  Options for the user:
  (a) keep B6: fallback (current; 3b gate stays vacuous, no native model draws).
  (b) native list ops: apply SETCOMBINE from the list over the record entry (B7 rule changes), TEXTURE through nd_words, GEOMETRYMODE into the z and cull inputs. Needs nd_model_state to take the list's combine and geometry words, plus compare fields for them.
  (c) strip those ops from the producer render lists (src/model.c): a picture-changing producer change, needs a picture check.
  Plan default (B6 as written): fallback. Taken for now.
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
