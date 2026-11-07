# Row SBKW: sbk_56 water, native model draw vs the walker

## Start state (nds1 bbc19df44)
sbk_56 cold det, native model draws vs nomodelnat (walker reference) on the same build: 0 of 12 shots equal, the difference in the water, framebuffer x 120..153, y 87..314 (240x400). Present on nds1 850eabf20 too (before MDL-5a). ndm fb is 0 at sbk_56. MDL-5e keeps nomodelnat for this row; delete nomodelnat (and any rec_dl path kept only for it) in this row's last commit.

## Oracle plan SBKW (2026-11-07)
The water is a map model on the record path (rec_run -> pm_gpu_model_native -> nd_model_state -> nd_model_draw), a panned surface: the record carries tile-size panner ops for tile 0 and tile 1 (R9, R10; rec_apply pm_gbi.c ~870 rec_op(q[9]), rec_op(q[10])), usually translucent, often two-tile. Native uvso[2..3] = -(tile_x0 + tile_fx)/tw, -(tile_y0 + tile_fy)/th from nd_ws after rec_apply (nd_model.c gv_params ~30-35). The walker is the reference (it applies the panner and tile state per frame, as the RSP).
Candidates: a. uvso offset on a cache hit: pm_gpu_model_hit (pm_gpu.c ~3527) reuses the cached NdDraw (uvso included) without rec_apply; if the key (k->w = rec words, NDMC_RW) does not cover R9/R10, or the panner reaches nd_ws another way, a hit draws last frame's scroll. b. tile 1 panner (uvx) of a two-tile water. c. per-axis wrap/mirror (nd_tex_wrap_final). d. blend, alpha or depth of the translucent surface (zupd/zcmp key).
Measurement: sbk_56 cold det, frame 200, switch ndmcheck (per record native vs walker key and NdDraw field compare, field names printed); log the record address with its bbox once; read the lines for the records over the water bbox.
Fix by the first differing field: uvso2/3 -> the panner words in the cache key (NDMC_RW covers R9/R10, or add tile_x0, tile_y0, fx, fy after rec_apply to the key; or the hit path recomputes uvso from the live tile state). uvx/u1 -> the tile 1 aux desc and nd_t1_uvmap with the R10 panner. u0 param -> nd_tex_wrap_final. k.zupd/zcmp/blend -> the key's render fields.
Gate: sbk_56 native vs nomodelnat byte-equal (cold det, 12 shots); ndmcheck diff 0 for the water record; mac_01 and iwa_10 native vs nomodelnat byte-equal.

## Rules
Own worktree build3ds/wt/sbkw, branch sbkw from nds1; copy assets/us and ver/us/build. Code first. Cold det runs. No attribution lines, never commit 3ds/src/map_restore_table.c, no git stash, keep Azahar open. Budget: -ResetBudget SBKW at the cap. Delete this file (and nomodelnat) in the last commit.

## State (2026-11-07, first agent)
Measurement (spot_sbk56_ndmcheck, run 7025, nds1 53f7a7773, f300): eq 8880 diff 333; all 333 records: k.combine native 2 vs walker 8, k.tex1 0 vs 1, k.two, k.w0, k.w1, tmpl, tev0, tev1, has_buf, u0.dim, u0.copy, uvso1 (native 39ffff00 vs walker 3a7fff00, 2x), uvso3. Cache-hit diff 0 (14771 hits). Code: nd_model.c:280 t1n = combine==8 && plan.uses_tex1, with t1_valid = c->t1_valid (pm_gpu.c:3376 = r13t1b_ok), set only in rec_run (pm_gbi.c:909) for R13 texture-only records.
Try 1 (333bb6881, pm_gbi_bind_tile1 for every model record): picture unchanged (12 of 12 shots still differ, 4275-4303 px in x120..152 y87..313). Reverted (51a083113).
Next code point (try 2, different point): tile 0 window height: native mainH 64 vs the walker ensure_tile window 32 (nd_tex.c:431, src/model.c:2348). uvso1 2x and u0.dim fit a doubled tile height. Log the native desc h and the walker tile_sz height for the water record, then fix the desc build to the walker's tile window.

## State (2026-11-07, SBKW step 1 and 2)
Branch sbkw rebased on nds1 7377c2848 (7a3ad4dd9 tile 1 step, 4f0f7bfb3 window, 4e5b7fd8a and 42c5f8ffe diagnostics). Lane sbkw build 2851.
Step 1 PASS: b42796a75 (now 7a3ad4dd9) runs only in rec_run's miss branch after rec_apply (pm_gbi.c ~909); the hit path never calls it. Sub b (mdlsub line) 0 at sbk_56 f100 to f400, iwa_10, kmr_03 battle (bench runs 20562, 20898 and the iwa_10 run).
Step 2 (oracle cause NOT supported): switch dlchash (nd_shape.c sbkw_dlchash, pm_dlc_native_ranges) at f200 to f211: 90 native lists per frame, all in gMapShapeData, live Vtx hash equal to the static copy hash (stale 0 in all 12 frames). The water record's list is not among the 90 and is not identified yet. The vertex-pool staleness is not the cause for these lists. The walker reference (nomodelnat) reads the same pool copies (dlc_static_group), so pool staleness cannot explain native vs nomodelnat either.
Still open: ndmcheck at f300 is diff 0 (spot_sbk56_ndmcheck) and cache hits are diff 0, yet sbk_56 shots still differ in x120..153 y87..314 (mean delta under 1, max 101). Next: identify the water list (log the list address of the records whose draw bbox covers the water), compare its native and walker draw order and blend (translucent order), and the state the native draws leave for the next walker op.
Steps 3 and 4 not started. Row INCOMPLETE. Budget reset at 8 starts (-ResetBudget SBKW), 5 starts used after the reset.

## Parked (2026-11-07, orchestrator)
Merged in nds1 ad7e0c994: tile 0 window size and tile 1 on the miss path (ndmcheck sbk_56 diff 0; mac_01, iwa_10 native vs nomodelnat 12/12; sub b 0). The oracle static-pool premise is refuted (dlchash: 90 native lists per frame, live Vtx hash equals the pool copy f200-f211; the walker reference reads the same pool). The residual (sbk_56 water, 10 of 12 shots, mean delta under 1, max 101, x120..153 y87..314) is parked: it does not block S10. nomodelnat stays as a debug tool until this row closes. Next when reopened: identify the water list (records whose bbox covers the water), compare native vs walker draw order and blend of the translucent surface, and the state a native draw leaves for the next walker op.
