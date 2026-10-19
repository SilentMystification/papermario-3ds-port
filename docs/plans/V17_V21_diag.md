# V17 to V21 intro diagnosis (row V17d)

Date 2026-10-19. Read-only diagnosis. No source file changed. Azahar only. Nothing ran on hardware.

## Method and limits

- Run 1: `fast shots` (one shot per 30 frames, 124 shots, f30 to f3720). The title starts at f3626. Run 2: `fast shots shotwin=3140:481` (one shot per frame, f3140 to f3620). Both runs use the same 3dsx (13092440 bytes, 01:14).
- Azahar starts: 3, not 2. A first window run (`shotwin=3140:480`) lost its shots. The V16d agent ran at the same time and deleted the shared folder `sdmc/3ds/PaperMario/shots`. The `run_azahar.ps1` lock does not protect that folder. Run 3 was the repeat. A future agent must not run two Azahar tests that write shots at the same time.
- Contact sheets: 2 (12 frames each: a scouting sheet from run 1 and a sheet from run 2). Also 5 single zoom crops. Shots are in `%TEMP%` only and are not committed.
- Intro scene order in run 1 (hos_05, theater with curtain, fades only inside the viewport): story pages f30 to f1000, Star Rod dome f1000 to f2050, Kammy and lightning f2050 to f2200, Bowser inhales and breathes f2250 to f2320, light burst and spirit capture f2330 to f3130, fade to black f3150 to f3172, road to the sanctuary f3172 to f3580, white fade f3584 to f3626, title.
- The frame numbers in this file are `pm_frame_count` values of the same boot. They repeat in each `fast` run (Turbo).

## Result per fault

| Row | Seen in the shots | Frame | State |
|---|---|---|---|
| V17 stars glow with a box | yes, translucent grey rectangles around the pink glow and the sparkles | f2070 (run 1 shot 69) | cause is a hypothesis |
| V17 partial fade | one case: the white fade into the title is cut at about 85 percent | f3584 to f3626 | cause is a hypothesis |
| V18 road flicker | not reproduced | f3364 to f3530 | no proof |
| V20 brick block | not found | none | not found |
| V21 Bowser fire invisible | yes, no fire at the inhale and the exhale | f2250 to f2310 (run 1 shots 75 to 77) | cause is a hypothesis |

### V17 stars glow with a bounding box (f2070)

- Evidence: in run 1 shot 69 (f2070) two translucent grey rectangles lie over the Kammy scene. They sit beside the pink sphere and the white four point sparkles. The sparkles themselves have the right shape. The rectangles are not in the sky stars at f3540 and f3610 (zoom crops: no box, the stars are fine).
- Evidence from the log (`run1.log`, line `comb new f24` and `f2648`): the planned combiner `(T1-0)*T0+0 / (T1A-0)*T0A` with IA8 textures (fmt 3/1 and 3/2) draws more than 100 times from f24 to f2216. The legacy combiner error for it is 0.992 (`legerr 992`), so only the plan path can draw it right. The same shape of combiner uses both texture tiles.
- Cause (best hypothesis): TEXEL1 (tile 1) is sampled wrong. In `3ds/src/pm_gbi.c`, `G_SETTILESIZE` for tile 1 (about line 2035) builds `td[1]` as a window of `((lrs-uls)>>2)+1` texels that starts at the load origin. The comment says SetTileSize only places the tile. For an effect that sets a scrolled or offset window for tile 1 the real RDP reads the same TMEM image with the tile shift, the mirror and the wrap. The port reads a small window at the image start. If the window is flat in alpha, `T1A * T0A` gives a constant over the whole quad. That shows as a box. This is the same family as the water_splash and motion_blur_flame faults of R13b and R13c.
- Not proven. The effect that draws the box is not named. Do not assume it is bulb_glow: bulb_glow starts at f2094 (`comb new f2094`, `(T0-0)*PRIM+T1`) and the box is at f2070.
- Fix plan:
  1. Name the effect. Command: `PM_ROW=<row> powershell -File 3ds/run_azahar.ps1 -Turbo -FaultStop -Debug "fast shots shotwin=2066:6 xtrace=2068:2 drawlog=2068:2" -Timeout 150 -Until "game mode 2 "`. Read the `xt` lines (fillrect, texrect, render state) and the `dd` lines (draw box, tris, tile size) of f2068 to f2069 with 40 lines of output. Match the box size to one `dd` entry.
  2. In `pm_gbi.c`, `G_SETTILESIZE` (tile 1 branch) and `ensure_aux`/`t1_fold`: for a tile 1 that shares the image of tile 0 (same `ldrec` source), keep the full image as the tile 1 texture and apply `uls`/`ult` as a coordinate offset (`t1_p[2]`, `t1_p[3]`), like tile 0 does through `tile_x0`. Do not cut the window.
  3. Test: the same shot window before and after (one contact sheet, 4 frames), then `python 3ds/tools/golden.py toad yard` and `python 3ds/tools/fxtest_report.py run all` for the effect table (the row FX1 file `docs/plans/FX1_table.md` shows `star`, `stars_*`, `fire_breath`).

### V17 partial fade

- Evidence, black fades are complete: the viewport mean falls to 1 at f3170 and stays at 0 to 2 until f3176 (run 2). The fade in runs at 5 to 6 levels per 8 frames and ends at the scene brightness at f3244. The curtain and the floor stay lit by design (the overlay is `OVERLAY_VIEWPORT_COLOR`, a fill rect over the camera viewport only, `src/screen_overlays.c`).
- Evidence, the white fade at the end is incomplete: the viewport mean is 19 until f3578. It then rises 2.7 to 3.4 levels per frame to 192 at f3620. The title starts at f3626. At that rate the overlay reaches about 85 percent of white when the map changes. The hard cut to the title shows the partial fade.
- Cause (hypothesis): either the real script waits for the overlay to reach 255 before the title starts (then the port starts the title early or stretches the ramp), or the decomp cuts at the same point (then this is not a fault). The ramp is a game-side ramp: `set_screen_overlay_params_back` is called from `src/state_intro.c` (`IntroOverlayType = OVERLAY_VIEWPORT_COLOR`, line 62) and `hos_05/intro.c` (line 1974).
- Not seen: partial fades inside the story pages (f30 to f1000). Shots there are one per 30 frames. The scouting sheet shows shots 2, 5 and 8 as pure white pages and shot 5 as a veil over the ship. These look like normal page fades, but they were not checked against the decomp.
- Fix plan:
  1. Read `src/state_intro.c` lines 30 to 260 (the title hand-off) and compare the frame of the last overlay call with the frame of `game mode 2` (f3626). Command: `grep -n "game mode\|overlay" build3ds/last_log.txt` after a run with a temporary log in `set_screen_overlay_params_back` (progress value, frame). One run, `-Debug "fast"`, `-Until "game mode 2 "`.
  2. If the real ramp reaches 255 before the cut: find which wait is short. A likely place is a frame timer that is counted in 30 Hz steps while the port draws at a different step (`DT` and `gGameStatusPtr->...` in `src/main_loop.c` or the F06 interpolation).
  3. Test: run 2 again, check that the mean at f3626 is 250 or the decomp value.

### V18 road geometry flicker (f3364 to f3530)

- Not reproduced. Facts from run 2:
  - The static part (f3244 to f3360) is stable: the mean frame difference is 0.0 to 0.7.
  - The pan back and up starts at f3364 and ends at f3530. The region means of the sanctuary, the road and the ice change smoothly (for example sanctuary 117, 117, 117, 116, 117, 116, 115 ... 104 over f3376 to f3399). There is no brightness jump of a polygon.
  - An A, B, A pixel test (a pixel that differs from both neighbours while the neighbours match) hits about 200 pixels per frame in f3366 to f3420. The hits lie at the ice horizon (rows 144 to 190) and not on the road or the building. The difference against frame 1, 2, 3 ... 8 later grows steadily (13.9, 17.0, 19.2, 20.8 ...). That is sub-pixel shimmer of the high contrast ice texture during motion, not a strict flicker.
- Cause (hypotheses, none proven), in this order:
  1. The user's flicker is a one-frame polygon pop that the shots do not show, or it appears on hardware only. The world fog is `SetWorldFogParams 0 0 0 0 0 0 0 995 1000` and the camera pulls back (`CamPullBack` boom and `AdjustCamVfov`). A large far distance makes the PICA 24 bit depth coarser. The decomp has the N64 near and far values in `LoadSettings` (`IntroCamSettings`).
  2. The near clip: `clip_near` in `3ds/src/pm_gbi.c` (line 637) drops a triangle when `w <= 0.05` for all corners (`n_drop`). A road triangle that is long and close to the lens may be dropped for single frames.
  3. Culling: `G_CULL_BACK` uses a screen cross product (line 1075). A triangle at a grazing angle may flip sign for one frame. The switch `nocull` turns it off for an A/B check.
- Fix plan:
  1. Count per frame. A temporary log (not committed) of `n_in n_cut n_drop n_out` per frame in `pm_gbi_end_frame` (the `stats` switch only sums 30 frames). Run: `-Debug "fast stats"` with the per-frame log, window f3340 to f3540. A frame with `n_drop` or `n_out` above its neighbours names the cause.
  2. A/B: the same window with `nocull`, and with a 16 bit tinted depth range check in `pm_gpu.c` (log the depth range per frame).
  3. Test: 30 frame series of f3364 to f3394, the diff metric of `docs/plans/V17_V21_diag.md` (flicker pixels under 30 per frame outside the horizon band).

### V20 brick block after the bridge falls

- Not found in hos_05. The only bridge in the intro is the purple road that lowers at f3202 to f3225 (road to the sanctuary). Run 2 frames f3176 to f3260 have no brown or red pixel except the curtain (a colour test `r > g+25 and r > b+25` finds only the curtain rows y 30 to 34). The ice pillars on the left and the right of the road look the same (zoom crop of f3205 and f3225). Spirit cards (f2600 to f3100) show correct card art, so they are not the block.
- Note: `src/world/area_hos/hos_05` has no bridge model code. The word "bridge" appears in `kmr_11` (`bridge_animation.c`) and `kkj_25`. The intro chain is hos_05, kmr_20, osr_00, kmr_20, mac_00. The user may mean a scene after the title in the new game chain, or a frame the 30 frame shots miss (f1000 to f2050 and f2330 to f3130 were sampled once per 30 frames only).
- Next step: ask the user for the scene (a one line question in `3ds/QUESTIONS.md`: the map name or the dialogue text before the bridge). If the scene is the new game chain, take one `fast autoa freshsave shots` series. No fix plan is possible before the scene is known.

### V21 Bowser fire invisible (f2250 to f2310)

- Evidence: run 1 shots 71, 75, 76, 77 (f2130 to f2310). Bowser inhales (shot 75, mouth open, dark room) and breathes out (shot 76, red room, mouth wide open, nothing in front of the mouth). Shot 77 shows a bright burst and sparkles but no flame. The script spawns the effect twice: `fx_fire_breath` type 1 every 5 frames during the inhale (`intro.c` line 704) and `PlayEffect(EFFECT_FIRE_BREATH, 0, -30, 210, 142, ...)` at `intro.c` line 2244.
- Evidence, effect table: `docs/plans/FX1_table.md` row 49 fire_breath: state ok, drew weak, 147 changed pixels, 7 dense cells, no black, no solid colour. Other effects draw 400 to 3000 pixels. The effect is nearly invisible even in the isolated test.
- Evidence, log: the fire combiner (`gsDPSetCombineLERP(PRIMITIVE, ENVIRONMENT, SHADE, ENVIRONMENT, TEXEL1, TEXEL0, ENVIRONMENT, TEXEL0, 0, 0, 0, COMBINED, COMBINED, 0, PRIMITIVE, 0)`, render mode `G_RM_PASS`, `G_RM_ZB_CLD_SURF2`, I4 texture, `assets/us/effects/gfx/D_09000A00_373DE0.gfx.inc.c`) has no `comb new` line in run 1. This is not proof: the log writes a line only for a planned or inexact combiner, so an exact legacy combiner stays silent.
- Cause (best hypothesis): the same tile 1 fault as V17. The fire display list loads the I4 image with `gDPLoadTextureTile_4b` (tile 0) and `gDPLoadMultiTile_4b` (tile 1) and then `fire_breath_appendGfx` sets both tile sizes with a scroll: tile 0 at `imgFrame*32`, tile 1 at `imgFrame*32 + 32` (`src/effects/fire_breath.c`, lines 210 and 211). The fire alpha is `lerp(TEXEL0 alpha, TEXEL1 alpha, ENV alpha) * PRIM alpha`. The port builds tile 1 as a 33 texel window from the image start (`pm_gbi.c` tile 1 branch of `G_SETTILESIZE`, about lines 2035 to 2075) and does not follow the scroll. TEXEL1 is then a fixed or wrong strip, and the cross fade between two frames of the flame texture is lost or dark. A second candidate is the fan geometry: it is five vertices with z scale 0 (`guScaleF(tempMtx, scale, scale, 0.0f)`), `G_CULL_BACK` and a mirror wrap in S. If the tile 0 window width (`pm_tex_size`, power of two of the window) differs from the 128 texel image, the mirror wrap differs from the RDP.
- Fix plan:
  1. Confirm the draw. Command: `-Debug "fast shots shotwin=2276:4 xtrace=2276:2 drawlog=2276:2"`, 150 s, `-Until "game mode 2 "`. In the `dd` lines look for `fmt 4/0` (I4) with `tris 3` or `tris 4`. If none exists, the effect is not drawn: log `fire_breath_render` (queue) and the render task flags (`RENDER_MODE_CLOUD_NO_ZCMP`) before the fix.
  2. If the draw exists: print the `td[0]`, `td[1]` windows and `t1_p` at that draw (the `V12c` block, `pm_gbi.c` about line 977). Compare with the expected window: image 128 x 32, tile 0 start `imgFrame*32`, tile 1 start `imgFrame*32+32`.
  3. Fix: same change as V17 step 2 (tile 1 shares the image of tile 0: use the full image and a coordinate offset). Then `python 3ds/tools/fxtest_report.py run all` (row 49 must reach 15 or more dense cells) and one shot of f2280.

## Shared causes

| Group | Faults | Cause | File and function |
|---|---|---|---|
| A | V17 star box, V21 fire | TEXEL1 (tile 1) is a small window at the image start and does not follow the SetTileSize scroll, mirror and wrap of the RDP | `3ds/src/pm_gbi.c`, `G_SETTILESIZE` tile 1 branch, `ensure_aux` and `t1_fold` |
| B | V17 partial fade | end of the intro: the title starts while the white overlay is at about 85 percent | `src/state_intro.c`, hos_05 end script, game side timing |
| C | V18 | not reproduced, depth or clip or cull | `pm_gbi.c` `clip_near`, `pm_gpu.c` depth range |
| D | V20 | scene unknown | none |

Group A is the one fix that may clear two faults. Test group A first (V21 has the cleaner test: `fxtest_report.py` row 49 and one shot at f2280).

## What was not checked

- Story page fades (f30 to f1000) frame by frame.
- Frames f1000 to f2050 and f2330 to f3130 (one shot per 30 frames only).
- Whether the stars glow box is the same on hardware.
- Any scene after the title (kmr_20, osr_00, mac_00, the demo).
- V19 (Sushie shadows) is not part of this file.
