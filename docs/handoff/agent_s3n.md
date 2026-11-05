# Agent S3N: map model records draw natively

## STATE
branch: nds1 | commits: 1f1839f2c (S3N-0 dead model emission deleted in src/model.c, 3DS path, N64 kept under #ifndef TARGET_3DS)
done: S3N-0. Gate: build ok (PM_ROW=S3N sh 3ds/build.sh, exit 0), nd_host run 66 PASS. No Azahar start (no pixel change expected: every removed site was gated by a value that is 1 on 3DS).
open: S3N-1, S3N-2, S3N-3, S3N-4 not started (see Blockers).
budget S3N: 0 of 8 Azahar starts.
next: S3N-1 (nd_model.c, compile-once deltas and texture descriptors). Needs a consumer before it can be gated by pictures.

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
