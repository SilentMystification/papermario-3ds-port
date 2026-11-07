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
