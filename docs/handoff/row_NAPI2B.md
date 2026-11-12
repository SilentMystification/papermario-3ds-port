# Row NAPI2B: the HUD rect record drawn by a direct executor (NATIVEAPI A1, first direct executor)

Model: Sonnet (user decides). Why not Haiku: it creates the shared direct helpers (pso decision cache, texture bind by descriptor,
walker invalidation) that every later row (glyphs, overlays, windows, menus, particles) calls; the split of ensure_tile_slow must keep
every bind byte-equal. Why not Opus: the decision code exists (nd_comb_decide, nd_render_state); this row calls it, it does not design it.

## Base and start
`sh 3ds/tools/wt_new.sh NAPI2B`; PM_ROW=NAPI2B PM_LANE=NAPI2B. Base `git merge-base HEAD main` (main 20d613506 at writing).

## Files and places
- 3ds/src/nd_direct.c (new), 3ds/include/nd_api.h (new declarations at the end), 3ds/src/nd_tex_set.c (ensure_tile_slow :378 only),
  src/hud_element.c (hud_rect_run :477 only), 3ds/tools/azt_scenarios.json + bench_defs.json (one entry each, after `pause_slotpeak`),
  3ds/tools/nd_host_test.sh (one gcc line for the new 3ds/tools/nd_direct_test.c, after the nd_api_test line :43).

## Design (code read, oracle 2)
1. nd_tex_set.c: move the bind part of ensure_tile_slow (the tile_bound and load arm after the FB1 and glyph arms: pm_gpu_flush,
   pm_tex_set_wrap, pm_tex_load, tile_x0/y0, ens_* fields) into `int nd_tex_bind_desc(const NdTexDesc* d, int n)`; ensure_tile_slow
   builds the NdTexDesc from nd_ws.td[n] and nd_ws.tst1 and calls it. NdTexDesc: src, pal, pal_n, fmt, siz, stride, x0, y0, w, h,
   wrap[4] (cms, cmt, masks, maskt), sz[4] (the SetTileSize window). No other change in nd_tex_set.c.
2. nd_direct.c: `const NdDec* nd_pso_decide(NdPso id)`: per pso id, once (cold), build an NdCombIn from nd_pso_get(id) (cw0, cw1,
   omode_h with the othermode bits the description sets, merged over nd_ws.omode_h for the rest), tex_on 1, use_t1 0, no box state,
   call nd_comb_decide and nd_render_state; store NdCombOut and NdRender. `void nd_direct_bind(NdPso id, u32 prim, u32 env)`: if
   out.plan_on: cur plan copy, pm_comb_colours with prim and env, pm_gpu_plan_key + pm_gpu_set_plan; else pm_gpu_set_combine(out.leg);
   the render state as sync_render sets it from NdRender. `void nd_direct_end(void)`: ens_clear(); nd_ws.cur_tile = -1; sc_ok = 0;
   nd_sr_forget() (the next walker draw rebinds and resyncs).
3. hud_element.c hud_rect_run: per record make the pso (nd_pso_n64 with the combine and render mode words hud_rect_emit would write
   for fmt, flags and dropShadow), then per piece nd_tex_bind_desc (image, palette for fmt 1, the piece window uls..lrt, masks, the
   mirror/wrap/clamp flags of the emit arm) and pm_gpu_tex_rect with x/y from ulx..lry and s/t as nd_texrect computes them (texStart
   * 32, dsdx = widthScale, filter offset 16 for FILTER_TEX), then nd_direct_end. hud_rect_emit stays for core 1 (fps60 anchors).
   A record shape the direct path cannot state (a flag it does not know): fall back to the old emit + nd_walk_state for that record.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS, including nd_direct_test (pso cache: same id gives the same NdDec; two descriptions, two entries).
- `bench.py base spot_mac01_det --commit <merge-base>`, `bench.py base pause_tab_det_pic --commit <merge-base>`; one list run
  `bench.py run spot_mac01_det,pause_tab_det_pic --row NAPI2B --why "A1: HUD rects direct"`; `bench.py cmp` each: all shots byte-equal.
- Speed (census): new scene `hud_fncov_pause` = `pause_slotpeak` debug with `fncov=200 det` instead of texstat, until `fncov f\d+ done`,
  cap 45. PM_FNCOV build; run base commit and head. Expected: pm_gpu_tex_rect calls equal; nd_texrect calls lower by the HUD piece
  count; the fallback count 0. Hand back the 15 largest deltas.
- Budget 8 Azahar starts. Console: none here; CONSOLE C1 after NAPI3B merges.

## Stop
Byte difference, a fallback above 0 in the pause scene, or a bind the split changes: stop, commit, hand back the values (2 tries per
named cause). Delete this row file in the completing commit.
