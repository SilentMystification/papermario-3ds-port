# Row BOMBLIT: which path draws Bombette unshaded on the first frame of her blow-up (measurement census)

(Model: Haiku. Bug: user fact (console, mim_11): on the first frame of Bombette's blow-up animation her colour is wrong, as if the
lighting does not apply for that one frame. Code only: no shot reading.)

## Code read (oracle, main 6e171b39a)
mim_11 sets sprite shading (src/world/area_mim/mim_11/main.c:244 SetSpriteShading(SHADING_MIM_11)): a partner sprite is drawn as a
shaded sprite: a CI4 raster in tile 1 (tex_tile1 records it as m1 only when nd_tm.ld_valid, nd_tex_set.c:540-549), then the tile 0
SetTileSize calls tex_shaded0 (nd_tex_set.c:200, :582-600), which binds the shaded bake (load_shaded_sprite :691-697: the two palettes
tlut_pal[tile_pal[1]] and [tile_pal[0]], shade_mean, pm_tex_load_shaded) and sets dual_on; the native sprite path needs dual_on
(pm_gpu.c:3933, why 3). The draw is unshaded (plain palette, no shade colour) when any of these fails:
 a. m1 not recorded: the raster came by LoadBlock or no LoadTile before tile 1 (ld_valid 0, :540);
 b. tex_shaded0 conditions (:583-585): tile_fmt/siz, tile_pal[0] == tile_pal[1], combine not TEXEL0 ... TEXEL1, m1.w or m1.h 0;
 c. load_shaded_sprite: a palette slot still NULL (:694) or pm_tex_load_shaded fails (pm_tex.c:1824);
 d. the native sprite path refuses (why 3) and its fallback draws the walker words without the bake;
 e. a cached sprite state or command block replayed from the previous animation frame (HC8 sprite state cache, pm_gpu.c sprite key).
The first animation frame is the only frame that changes raster and palette at once; which of a-e holds there is not decidable by
reading the decomp sprite list order. This census names it.

## Start
`sh 3ds/tools/wt_new.sh BOMBLIT`. PM_ROW=BOMBLIT PM_LANE=BOMBLIT.

## Touch only
3ds/src/nd_tex_set.c (tex_tile1, tex_shaded0, load_shaded_sprite), 3ds/src/pm_gpu.c (spr_build_rare), a new cold function in
3ds/src/nd_tex_set.c; 3ds/src/pm_input.c:307 (only pm_log to pm_log_evt); 3ds/tools/azt_scenarios.json and bench_defs.json.

## Exact edits (debug switch shadecen, cold counters; no draw change)
1. nd_tex_set.c: `unsigned shc[8];` (0 m1 set, 1 m1 skipped: CI4 tile 1 with ld_valid 0, 2 tex_shaded0 ok, 3 shaded0 conditions
   false, 4 palette NULL, 5 load_shaded fail, 6 native why 3). Increment at each site only when `PMDL("shadecen")`.
2. A cold `shc_frame(unsigned f)` called from pm_gpu.c frame end next to the other census frame hooks: when PMDL("shadecen") and any
   shc[k] != 0: `pm_log_hw("shadecen f%u m1 %u m1skip %u ok %u cond %u nopal %u loadfail %u why3 %u", f, ...)`; then zero shc.
3. Scenario bombette_mim11: debug `warp=mim_11:0 partner=3 padscript=wait90,cdown,wait150 shadecen loglevel=debug det`, until
   `padscript: done at scan` (pm_input.c:307, pm_log: if the until check needs a pm_log_evt line, make that line pm_log_evt), cap 30,
   needs_picture false; bench of the same name.

## Gate (measurement)
Build, memcheck OK. One Azahar start: `bench.py run bombette_mim11 --row BOMBLIT --why "which shading path fails on the first
blow-up frame"`. Hand back every shadecen line from the C-down press to the end, as printed. Stop after the hand-back.

## At the end
Delete this row file in the completing commit.
