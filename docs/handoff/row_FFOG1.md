# Row FFOG1: a floor vertex at or behind the eye gets fog 0 (Forever Forest, mim_11)

(Model: Haiku. Starts after ESCAPE merges: both add scenarios. `sh 3ds/tools/wt_new.sh FFOG1`, PM_ROW=PM_LANE=FFOG1. Output to build3ds/out/.)

How the floor draws:
- Map models with fog go through pm5 (nd_model.c:60-70: lflag.z = 1, fogu from fog_fm/fo, alpha = fog factor, cm[3]=1 ca[3]=0).
- The pm5 fog per vertex is `alpha = clamp(fogu.x*z'/w' + fogu.y, 0, 255)` (pm5.v.pica:135-140). Fog distances: mim_01 990-1000 with colour 0,0,0 on a black background; mim_11 960-1000 (mim_11/main.c:9-10).

Paths that can give a wrong floor:

P1 (cause, applies to every mim room): a floor vertex behind the eye gets full fog, and the PICA spreads it over the visible floor.
- Behind the eye (w < 0), N64 z/w = (A z_e + B)/(-z_e) = 1.05 + 2n/z_e > 1, so fog = 255.
- The PICA clips the triangle at near and interpolates that vertex's alpha into the visible part. Example: w_A=400, w_B=-50, the near-clip point is at t≈0.85, so alpha ≈ 217. The near floor goes almost fully fog colour, which is black on a black background: "the floor does not render, just the background", "random".
- The N64 clipper makes new vertices and computes fresh fog at the near plane (z/w ≈ -1, fog 0), so it never shows this.
- It depends on a 508-unit ground triangle reaching behind the camera. That is why Azahar did not show it on the FFOG route: the dlcwz straddle line never printed there, so no box crossed w=0.
- The FFOREST far clamp (pm5:70-72) does not change a vertex with w<0 (z' < 0 < -w/65025), so it neither causes nor fixes this.

Excluded:
- Stale combined (cmis 0) and the on-screen box cull (ons 0): FFOG census.
- Far cut: FFOREST clamp.
- Stereo cull: clip_bits4 (nd_geom.c:40) only widens the x planes.
- Near cut at 16 (NoN): only matters within 16 units of the camera, not the floor.

Row FFOG1 (Haiku, 4 Azahar starts):
- Edit 3ds/shaders/pm5.v.pica after line 140 (`min r12.x, myc3.xxxx, r12.x`): `slt r13.x, myc3.yyyy, r5.wwww` then `mul r12.x, r12.x, r13.x`. Comment: FFOG1, a vertex at or behind the eye has fog 0, as the N64 near-clip vertex. myc3.y = 1e-10; r13 is free after the light loop.
- Keep the uniform block order (pm_gpu.c:1528 check).
- New scenario mim11_floor (same commit as its bench, azahar_pic):
  - Debug string: `warp=mim_11:0 det wzcull shots`, plus a padscript walk toward the camera for 3 s.
  - Until-line `dlcwz f\d+ grp \d+ str [1-9]`, cap 20 s.
  - If str stays 0 in 2 starts, hand back with the setup (the orchestrator asks the user for the walk path).
- Gate:
  - Shot at the dlcwz frame shows the floor textured to the bottom of the screen, with no black or fog-colour wedge.
  - The same on mim01_ring_wide_ffog with wzcull.
  - Byte check against the merge-base shots: spot_mac01_det and battle_start_f182_cold (fog off: byte-equal).
  - nd_host PASS, memcheck OK.
  - Console: the user walks mim_11 and the forest on the XL.
- Delete row_FFOG.md and row_FFOREST.md in the completing commit if the console check passes.
