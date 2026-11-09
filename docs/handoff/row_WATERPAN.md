# Row WATERPAN: scrolling (panned) models miss the model cache every frame (user, 2026-11-09)

## User facts
User (console): "if you stand at -75 -10 500 on mac_05, all the water being on screen causes the game to lag."

## Cause (shared oracle, code and asset read, main 00a4e1901)
Water = model `kaimen` (mac_05_shape.bin), SURFACE_XLU_LAYER1, CI4 main + CI4 aux wrap/wrap, 2-texture combine, 140 triangles, scrolls
every frame (main.c:166 SetTexPanner, EVS_UpdateTexturePan). 1. The model cache key copies all 28 record words (pm_gpu.c:2975-2977),
including the panner tile-size ops R9/R10 (key words 18-21, nd_recs.c:21, :105), which change every frame: a miss and a full rebuild
every frame, a new cache entry per frame, and at 3/4 full mc_clear_all (nd_model.c:507) rebuilds the whole scene. About 2-4 ms on
core 0. General: every panned model (water, lava, falls, belts). 2. tile_bound (nd_tex_set.c:306-307) compares the tile size, which
moves with the scroll: pm_tex_load + dual_bind each frame, about 1-2 ms.

## Start state
`sh 3ds/tools/wt_new.sh WATERPAN`, branch WATERPAN. PM_ROW=WATERPAN, PM_LANE=WATERPAN. One Haiku agent.

## Steps
1. Check: census lines `ndm f<n> ... hit miss clears` and `ndmrw f<n> w 18:x 19:x 20:x 21:x` (pm_gpu.c:3140-3150) in one Azahar run at
   mac_05 -75 -10 500 (new scenario + bench, cap 15 s). Expect misses on words 18-21 every frame. If not -> hand back.
2. Fix: pm_gpu_model_key clears the panner origin (uls/ult of key words 18-21), keeps the tile index and window size; on a hit,
   rebuild uvso[2..3] (unit 0) and the unit-1 uvx offset from the current panner words by the nd_gv_params rule (nd_model.c:38-41,
   -(tile_x0 + fx) / tw). tile_bound: for a wrap/mirror tile whose mask equals the texture size, compare the window size, not the
   origin; clamp tiles and windows smaller than the mask keep the full compare. Host test: model key of a panned record equal across
   two pan values.
3. Gates: nd_host PASS; the census at mac_05: misses on words 18-21 and clears about 0; byte-equal vs stored base on spot_mac00_det
   and the new mac_05 bench (output must be identical); memcheck OK. Console (user): prof at mac_05 -75 -10 500, nd ms.
4. Commit; delete this row file in the completing commit. Budget: 6 Azahar starts.

## Touch only
3ds/src/pm_gpu.c (pm_gpu_model_key and the hit path), nd_model.c (uv offset on hit), nd_tex_set.c (tile_bound), host test,
azt_scenarios.json, bench_defs.json. Row HC4 also edits these files (HC6 model hit later): keep edits small and say so.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops. No-repeat: a try with no change in the census ends it.
