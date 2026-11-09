# Row FFOREST: Forever Forest objects sliced at the far plane, ground flicker (PICA far clip vs N64 far clamp)

## User facts
User (2026-11-09, O3DS XL, build 92e013100): "Forever Forest has rendering issues... elements in the center of the field, like
trees, bushes and rocks, seem to clip out of visibility weirdly. Additionally the ground flickers here." "The culling seems to be
rotating the wrong way relative to the camera." The user has seen this elsewhere too.

## Start state
Worktree build3ds/wt/FFOREST, branch FFOREST from main. PM_ROW=FFOREST, PM_LANE=FFOREST. One Haiku agent.
REPRO: mim01_ring_det on base already shows it (run 151823, shot 0000: near-black pixels in rotated box (200,60,400,140) = 6996).

## Cause (shared oracle; about 60% for the objects, about 45% for the ground flicker)
- mim_01..mim_10 use far 650 (src/world/area_mim/mim_01/main.c:24 SetCamPerspective(..., 25, 16, 650)); other maps 4096.
- N64 F3DZEX2 NoN rejects a triangle only when all 3 vertices are past far (port copy: clip_bits4 bit 32, nd_geom.c:47; gv_tri
  reject mask 0x2f, pm_gpu.c:156) and clamps vertex z for the rest. The port's pm4/pm5 shaders (3ds/shaders/pm4.v.pica:54-57,
  `outpos.z = zmix.x*z + zmix.y*w`, no clamp) let the PICA clip each triangle exactly at depth 650.
- The zone camera looks along the ring tangent about 435 units from Mario; the field centre is 600-1000 deep, so trunks, rocks and
  the 508-unit ground triangles (o401) are cut on a plane that sweeps opposite the camera motion ("rotating the wrong way"), and
  the cut edge on the ground moves every frame (flicker).
- Ruled out: area_mim hide/fade code (none), math helpers (equal to reference), vanilla bounds cull (models < 100 units only),
  port box culls (same matrix, homogeneous tests), coplanar ground layers, fog (past far).

## Fix steps (only the two shaders)
1. pm4.v.pica after line 56: compute z into a temp, then `z = min(z, -eps*w)` with eps = 1/65536 in a new constant; `mov outpos.z, z`.
   A vertex past far lands on the far plane; a vertex behind the eye (w < 0) keeps -eps*w > 0 and is clipped as before. Comment
   FFOREST (N64 rejects only whole triangles past far and clamps vertex z; the PICA would cut).
2. Same in pm5.v.pica at its outpos.z lines. Keep the uvso/cmul/cadd/zmix register block (pm_gpu.c:1528 checks it).
3. Do not change clip_bits4, dlc_box_out or pm_gv_rejm. Do not add a near clamp.
4. Check the depth clear: clamped depth eps must pass GREATER against the cleared buffer.
5. Only if the shot does not change: log `farpart f<n> n <c>` once per 300 frames (gv_tri triangles with mixed far bits) and check
   whether the static native ranges (nd_model_set.c:30) carry the far groups.

## Gates (CLAUDE.md GATES: light)
- nd_host PASS; log shows `pm4 DVLB on`.
- mim01_ring_det (until `shot \d+ frame 280\b`): shot 0000 shows the ground and whole trees/rocks right of the stump with no straight
  cut; the near-black count in box (200,60,400,140) drops clearly below 6996; no jumping edge between consecutive shots.
- Byte check on 2 scenes against the stored base shots: spot_mac01_det, battle_start_f182_cold (far 4096: no change expected).
- memcheck OK (shader only). Console (user): walk the mim_01 ring on the XL.
