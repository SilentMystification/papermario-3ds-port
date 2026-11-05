# Agent S3N: map model records draw natively

## STATE
branch: nds1 | commits: 1f1839f2c (S3N-0), c48abe457 (S3N-1 part: nd_gv_params moved, nd_shift_mul, nd_prim_ndc_z), 067a85de9 (S3N-2 nd_native_submit)
done: S3N-0; S3N-2 (gate: spot_mac01 frames 200 to 205 byte equal to HEAD, cmp of the ppm files; build ok).
part: S3N-1. gv move gated (nd_host PASS incl. nd_gv 200000 random bit-equal cases; spot_mac01 byte-equal). NdMode and NdModelTex compile tables NOT built (see blocker B4).
open: S3N-3, S3N-4, S3N-5 not started. Spot pictures for the model path not run.
budget S3N: 4 Azahar starts (build-at HEAD baseline, mac01 x2, mac01 after S3N-2). Limit 8.
next: user decision on B4 (texture descriptor source), then NdMode and NdModelTex (S3N-1 rest), then S3N-3.

## Blockers (plan default taken where noted)
B1. Walker state is global; nd_raw_native reads combine, r_zcmp, ... from walker globals. Plan default: derive from compiled deltas (S3N-1). OPEN.
B2. Geometry: DLC2 static groups (dlc_find) are reused as is. Plan default kept.
B3. Merge (S3N-2 in the plan) moved to S3N-3 with its consumer.
B4 (new, needs user decision). The texture bind list of a model (TextureHandles[i].gfx, built by make_texture_gfx in model.c) is run by the walker through state that is not in the list: ldrec, blk, tlut_pal, tsz_last caches and the V05, V12c, V13, FX3f, PT09, PT37 rules in pm_gbi.c op_settilesize and op_loadblock. nd_words cannot give NdTexDesc (w, h, stride, pal) without copying those rules. Options: (a) build NdTexDesc from TextureHandles[i].header and the raster and palette pointers (model.c), reproducing the common case only and fall back on the rest, verified by ndmcheck; (b) keep the walker texture path for models until S3L. Plan default not chosen because it changes which textures draw.
B5 (new). Render mode compile: nd_words from zeroed state gives om_h, om_l only for the bits the RM lists write. The cycle type and any other bit the lists do not set come from the walker state at draw time. The native NdMode must either carry those bits from the record or the list must set them. ndmcheck (S3N-3) will show it; plan default: fallback when a bit that nd_render_state reads is not written by the list.

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
