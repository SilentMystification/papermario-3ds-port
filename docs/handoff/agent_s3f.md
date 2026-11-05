# Agent S3F: handoff

## STATE
branch: nds1 | commits: 560c5759b (S3F-2c billboard, panner, IGNORE_MATRIX in record), c856da583 (S3F-4 walker model path deleted)
done: S3F-1a, S3F-1b, S3F-2a, S3F-2b, S3F-2c, S3F-4. Done grep (G_PM_MODEL, mg_run, pm_dlc.inc.c in 3ds/src include src) empty.
gates S3F-2c: mdldiff 13000 runs 0 diffs (before the delete); seam_pic frame 200 right by eye.
gates S3F-4: spot_mac01_pic, spot_mac00_pic, spot_sbk56_pic (frames 200 to 205) right by eye. Console perf not run (not a gate).
INCOMPLETE S3F-3: custom gfx not in record (pre and post G_DL stay plain words). Record carry needs a nested DL run from rec_apply, not built, no scene tested.
INCOMPLETE S3F-3: floor reflection nd_range_begin, nd_range_end, nd_range_repeat do not exist in the tree (plan anchor gone). Not built.
open: local vertex copy, specialTex, untextured models: plain words through the walker, not in the record (plan default, no test of these classes).
benches: mdldiff cannot run (its switch went with mg_run). texdiff loses tb_bind. bench_defs.json not edited (user rule).
budget S3F: reset at 8 starts before spot_sbk56 (status line in 3ds/logs/perf/status_S3F.txt). Since reset: 1 start.

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
