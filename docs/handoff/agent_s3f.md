# Agent S3F: handoff

## STATE
branch: nds1 | commits: eeba51266 (S3F-1a), dfbf5d131 (S3F-1b), 6c4dc9e70 (S3F-2a: 1CYC, 2CYC, FOG, alpha test, all render modes in the record)
done: S3F-1a, S3F-1b; S3F-2a (mdldiff 6500 runs 0 diffs, nd_host PASS, seam_pic frame 200 right by eye)
INCOMPLETE: S3F-2 (shroud, depth, remap, panner, billboard, custom gfx, local vertex copy still outside the record), S3F-3, S3F-4 (not started)
budget: S3F hit 8 starts at seam_pic; reset once (run_azahar.ps1 -ResetBudget S3F), then seam_pic pass
open: S3F-2b shroud, depth, remap (design below), S3F-2c panner and billboard, S3F-3 floor reflection and custom gfx ranges, S3F-4 delete
not checked: matrix stack top is not in TS_FIELDS (mdldiff does not compare it); fog colour, blend colour, fog_fm, fog_fo are now in TS_FIELDS
rule for next: records are 6 words (R0 to R5), build record words at the end of appendGfx_model; old words after the record count in R0 low byte

## S3F-2b design (not built)
- Record grows to 8 words. R6: w0 prim colour word, w1 env colour word. R7: w0 blend colour word, w1 shroud render mode c2 (gDPSetRenderMode(PM_RM_SHROUD, c2)).
  R3 w0 bits: 1 fog, 2 prim, 4 env, 8 blend, 16 shroud rm.
- Old order to keep (mdldiff compares end state): depth [prim, fog colour, fog pos] before header today; remap [prim, env] before header today.
  Move them after gDPPipeSync and before lights (both recOn and non-record paths), gated by REC_SKIP when recOn.
- Shroud (ShroudTintAmt 255 returns early, keep): RM IDX_10 op, then shroud c2 word, then fog colour (fog rgb, ShroudTintAmt), blend (ShroudTint rgb, 255), fog pos (970, 1000). Combine entry is SolidCombineModes[tex][SHROUD].
- Consumer: env colour needs an op in rec_op (G_SETENVCOLOR). Today mg_run has no arm for it (it returns q and the walker runs it), so add the arm the walker uses (GP_SET(2, envc == w1); envc = w1) as a function both paths call. Do not copy the logic.
- Add prim and envc to TS_FIELDS when the record writes them.

## Blockers recorded
- mg_run has no G_SETENVCOLOR arm. Plan default: add one (see above) in S3F-2b.
- Depth and remap pre-header words: plan default: move after pipesync (see above).
