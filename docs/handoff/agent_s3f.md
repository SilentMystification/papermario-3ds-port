# Agent S3F: handoff

## STATE
branch: nds1 | commits: eeba51266 (S3F-1a), dfbf5d131 (S3F-1b), 6c4dc9e70 (S3F-2a), 771e42d55 (handoff), d0574e0a0 (S3F-2b)
done: S3F-1a, S3F-1b, S3F-2a (1CYC, 2CYC, FOG, all modes, combine), S3F-2b (shroud, depth, remap; record 8 words)
gates S3F-2b: mdldiff 9000 runs 0 diffs; seam_pic frame 200 right by eye. Not verified: the mdldiff scene holds each class.
open: S3F-2c panner, billboard (2 matrix ops: finalMtx then billboard MUL), custom gfx (G_DL words, S3F-3), local vertex copy, IGNORE_MATRIX, specialTex. These keep old words; the record does not carry them yet.
open: S3F-3 floor reflection (nd_range, execute_render_tasks), custom gfx ranges. open: S3F-4 delete (done grep: 21 hits before S3F-2b).
open: 3 spot pictures (spot_mac01_pic, spot_mac00_pic, spot_sbk56_pic) not run. Budget: S3F reset once after 8 starts.
rule: record is 8 words (R0 to R7), values at the end of appendGfx_model; old words after the record count in R0 low byte; R3 w0 flags (see model.c comment).
next: S3F-2c billboard and panner first (smallest), then decide custom gfx and local copy with S3F-3 item 2.

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
