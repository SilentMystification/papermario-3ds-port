# PT10: Peach's room (kkj_14), flickering pixels on floor and bed (NOT reproduced in Azahar)

Map: kkj_14 "Peach's Castle - Peach's Room (2F)" (src/world/area_kkj/kkj_14/kkj_14.h line 2). The prologue scene is entry 3 (EVS_Scene_Chapter0). Evidence level: no cause found. Level of every guess below is GUESS.

## What Azahar shows
- Run B (PM_ROW=INVSCENE, start 2): `fast fixedseed warp=kkj_14:2 story=ch0_toad_town shots shotwin=180:8 smokeframes=200 smoke loglevel=debug`. 8 consecutive frames, 3ds/logs/perf/invscene/b (shots, b_union.png).
- Changed pixels between frames: 60 to 150 per frame pair, 246 in the union of 7 pairs. They are the 6 candle flames (animated), one fireplace spark and Mario. No pixel of the floor, the wall or the bed changes. Camera and Mario stand still.
- Draw list of frame 70 (run C, drawlog=70:1): 67 draw chunks. 55 are opaque (ZMODE_OPA), 10 are ZMODE_XLU, 2 are ZMODE_DEC. The 2 decals are the shadow of Mario (img 004446d8, prim 00000080 and 0000007f). The room mesh has no decal. So decalbias (pm_gpu.c:252-256), the V16b two pass decal (pm_gpu.c:258-263) and DECAL_EPS do not touch the floor or the bed in this view.
- Depth test of opaque draws: GPU_GREATER (pm_gpu.c:587-592). Strict test: for two opaque surfaces at the same depth the first drawn wins.
- Switch tests nozclip, nozt, depthview=3: not run (budget 3 starts, the 3rd was used for PT06 and a second map in the same start). Reason to expect no change: a still camera gave no flicker at all in Azahar.

## Why Azahar can hide it
- A flicker that is not in 8 still frames needs a changing input: a camera move (the prologue scene moves the camera), the shadow decal, or hardware rasterization that Azahar does not copy. Azahar uses a host GL rasterizer. The O3DS PICA200 rounds vertex x and y to its own sub-pixel grid and outputs z as float24 (V18/V19 notes: step 2^-17 near z = 1, docs/plans/V18_V19_azahar.md).

## Candidate causes (GUESS, in order)
1. Coplanar opaque surfaces (bed base or rug on the floor mesh) with strict GREATER and float24 z: the winner per pixel changes when the camera moves a little (breathing camera of the prologue scene). Check: draw list of the room at the frame with the flicker, find two chunks with the same plane (drawlog box overlap, `ddonly=<lo>:<hi>`).
2. A T-junction or shared edge between the bed mesh and the floor mesh (different matrices): the PICA fill rule with exact float vertices leaves a pinhole of 1 pixel where the N64 snapped vertices to 1/4 pixel (S10.2). The port does not snap (no rounding of screen x and y in pm_gbi.c tri emission). Pinholes show the background colour through the floor.
3. The Mario shadow decal under the new two pass stencil path (pm_gpu.c:258-263, switch nodecal2): the shadow of Mario or Peach lies on the floor near the bed.

## What the user must provide
- Two consecutive hardware shots (or one shot plus a second run with the same frame) of the flicker, with the frame number from the FPS HUD, and the log of the same run with `loglevel=debug`.
- One run each with `nodecal2`, `decalbias=0.00003` and `nozclip` in -GameDebug. If the flicker vanishes with one of them, that switch names the cause.
- Hardware shots at entry 3 (prologue scene) exactly: `warp=kkj_14:3`.
