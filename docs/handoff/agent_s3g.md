# Agent S3G: handoff

## STATE
branch: nds1 | commits: 4323c3e69 (S3G-2 local copy, special tile list, untextured and fog shroud in record), f90d823b2 (S3G-1 custom gfx pre, post, geometry, pop in record)
done: S3G-1, S3G-2. nd_host PASS (runs 55, 57). spot_mac01_pic right by eye (S3G-1 and S3G-2, S3G-2 identical to S3G-1 pixelwise).
open: S3G-3 floor reflection nd_range_*, S3G-4 sort and merge.
budget S3G: 2 Azahar starts used (spot_mac01_pic x2).

## Design (plan defaults)
- Record R11 w0 pre list, w1 post list. R12 w0 geometry list (0 for local copy: its list is the script's own G_DL), w1 pop flag. R13 w0 count of words after the record (special tile list, or 4 untextured ops), run in place of the bind (rec_aux, pm_gbi_run with nbytes).
- rec_dl: a model list runs as G_DL does (dlc_try first), then pm_gbi_run nested (returns at G_ENDDL).
- Pipesync: g1_skip in the walker, so no pipesync word on the record path.
- Shroud tint 255: early return before the record (nothing emitted; old path also drew nothing). Plan default.
- Non-3DS path of appendGfx_model unchanged (N64 build). 3DS path: recOn = 1 for every model. Old emission code under recSkip/REC_SKIP is dead on 3DS now (see the cleanup note below).

## Notes
- Session note: git add -N and git reset -q were run once by this agent (S3G-1 start, before coordinator rule). No staged work of others was lost as far as status showed. nd_core.c was converted to LF and back to CRLF (no content change).
