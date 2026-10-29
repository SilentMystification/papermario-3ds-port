# Diagnosis: TOADFOUNT (Toad Town fountain, mac_01)

## Status
- Fountain NOT seen in any shot. Cause NOT found. No game source edit.
- Azahar: 3 of 3 starts used in row TOADFOUNT. No start left until the orchestrator resets the budget.
- Scenario toad_fountain added (unverified). Catalog row added.

## Map data [C]
- Decoder: scratchpad scripts shape.js, tex.js, mtx.js (read assets/us/mapfs/geom/mac_01_shape.bin, big-endian N64).
- Pointer base 0x80210000 (file offset = pointer minus base). Checked: vertex table at 0x80214ff0 holds Vtx records. MODEL_hikari = 0x2D.
- Map origin (0, 0, 0) holds no fountain-like leaf. Floor leaves span x and z +/-625. [C]
- Candidate A: group with Mtx at 0x80214a70 holds 5 leaves (leaf 0x29 to 0x2D, names dr_yuka, o279, o283, o284, hikari).
  - Local bounds: x +/-148, y -1.4 to 124, z +/-148. Shape reads as a roof or dome (textures mac_mac_roofdertif, mac_mac_roof1tif).
  - Mtx row 3 (row-vector convention) gives translation (-299, 103.4, -325), rotation 45 deg about Y. World centre about (-299, 160, -325). [C] [E]
  - Render modes: leaves 0x29, 0x2A, 0x2B, 0x2C, 0x2D are all 1 = RENDER_MODE_SURFACE_OPA (include/enums.h:3301). [C]
  - hikari (0x2D) is panned at src/world/area_mac/mac_01/main.c:168, 182-183. Its render mode is opaque, not blended.
- Candidate B: leaves 0x26 (rm 34, SURFACE_XLU_LAYER3, texture mac_komonotif) and 0x27 (rm 17, SURFACE_XLU_LAYER1, no texture name). Group Mtx 0x802149f0 translation (-303.7, 39.1, -328.8). [C]
  - Translucent layers fit the "blended overlay" in the user report better than candidate A. [E]
- Leaf 0x86 (rm 34, no texture name) sits at x 234 to 299, z 288 to 388. It is at the flower gate (entry 5, 264, 27, 350). Not water. [E]
- Column-major reading of the Mtx gives translation 0 for every group. Rejected: all groups would sit at origin. [E]

## Runs (scenario toad_fountain) [M]
- Route: warp=mac_01:0, padscript seg_a, go_-300_0_500, seg_b, wait40, go_-150_0_300, seg_c, wait40, seg_d. Shots every 4 frames from frame 100.
- Scene start f51. seg b (x about -300) at f114. seg c at f158. Mario at x -170, z 0 to 3 from f200.
- Run 1, default build: boot line build 1488, git f230e46c6. 50 shots (f100 to f296). Cap 40 s ended the run.
- Run 2, lane final (29f504d7b, sha256 0df21e8c, same as 3ds/logs/builds copy). 13 shots (f100 to f148). Cap 40 s ended the run. Until used frame 149, which never prints (shots step by 4).
- Run 3, lane toadf6 (final6 3dcfc661d, sha256 ebeb4902). 13 shots (f100 to f148). Until matched at 16 s.
- Read by eye (run 1, rotated to landscape): f100, f140, f180. Toad Town street, post office side, Toads, Mario near x -170. No fountain. Water visible only at the bottom-left edge (harbour canal).
- shot_cmp top shots, frames 100 to 148 (13 shots): 0 of 13 byte-equal for each pair (HEAD vs 29f504d7b, HEAD vs final6, 29f504d7b vs final6).
- Differences: 690 to 2100 px per shot, all at x 70 to 144, y 52 to 347 (portrait shot coordinates). That band holds the moving NPC sprites. The same-binary noise in notes is 300 to 1300 px, so the build pair cannot be told apart from noise. [M]

## Source path [C]
- Render mode to render index: src/model.c:1594 to 1655 (switch on renderMode).
- Layer draw state (depth test, z write, decal bias): src/pm_gpu.c:738 to 749 (from diag_toadvis.md).
- State skips: src/pm_gbi.c:2714, 2752, 3283 (from diag_toadvis.md).

## Ranked candidates [E]
1. The overlay is translucent (candidate B, rm 17 or 34). Its draw order or z write against the blocker decides the look. Check: per-draw render mode and z write of leaves 0x26 and 0x27 on the 3DS build.
2. Candidate A (roof, rm 1) may be the blocker. The panned hikari overlay has no blend mode, so it cannot be the water on its own. Check: draw mode word of leaves 0x29 to 0x2D in the dlc replay.
3. G2 equal-state skip (src/pm_gbi.c:2752) drops a mode word between the blocker and the overlay. Check: mode word with and without the skip. Not measured.

## Next step
- Orchestrator: reset budget of TOADFOUNT. Then one run at a view of candidate B: stop near x -300, z -300, facing the group at (-304, 39, -329).
- Route: padscript from entry 0 with a go_ token that stops north of x -300 (z -250), then wait40, then shot window on that stop.
- Check the walk path first: mac_01_hit.bin collision near (-300, -250 to -330). [E]

## Limits
- Azahar: no fountain in any frame of the walk. Not reproduced.
- Texture names are blank on several leaves. The texture comes from a script or variant. Not decoded.
- Shot writer runs at about 3 frames per second. Keep the shot window short.
