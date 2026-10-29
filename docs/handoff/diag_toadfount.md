# Diagnosis: TOADFOUNT (Toad Town fountain, mac_01)

## Status
- Fountain NOT seen in any shot yet. Cause NOT found. No game source edit.
- Budget reset by the orchestrator. Starts used since the reset: 1 of 3 (toad_water, HEAD build 1488).
- Position run stopped by rule (1): the first shot shows no fountain.

## Map data [C]
- Decoder: scratchpad scripts shape.js, tex.js, mtx.js (assets/us/mapfs/geom/mac_01_shape.bin, big-endian N64).
- Pointer base 0x80210000. Vertex table at 0x80214ff0 checked. MODEL_hikari = 0x2D.
- Map origin (0, 0, 0): no fountain-like leaf found. [C]
- Roof group, Mtx 0x80214a70: leaves 0x29 to 0x2D (dr_yuka, o279, o283, o284, hikari). Local x +/-148, y -1.4 to 124. Textures roofder and roof1. Render mode 1 (SURFACE_OPA, include/enums.h:3301). [C]
  - Row-vector translation (-299, 103.4, -325), 45 deg about Y. World centre about (-299, 160, -325). [C] [E]
- Translucent group, Mtx 0x802149f0: translation (-303.7, 39.1, -328.8). Leaf 0x26 (rm 34, SURFACE_XLU_LAYER3, komono) and leaf 0x27 (rm 17, SURFACE_XLU_LAYER1, no texture name). [C]
- Leaf 0x86 (rm 34, no texture) at x 234 to 299, z 288 to 388: flower gate area. [E]
- Column-major reading gives translation 0 for every group. Rejected. [E]

## Position run toad_water (HEAD build 1488, git f230e46c6) [M]
- Route: warp mac_01 entry 0, padscript to (-300, -150), then to (-300, -240), wait 20.
- Log: seg b at f121, seg c at f176, seg d at f186. Shots every 4 frames, frames 140 to 176 (10 shots).
- Run hit the 40 s cap. The until-line did not match (regex was broken in the file; now fixed in the working tree).
- Shot f140 and f176, read by eye (rotated to landscape): Mario in front of a purple walled box with a star pattern, a bulletin board on the front, and a round blue stone base at the bottom edge. Blue water or blue base at the bottom left and right edges. No fountain basin, no water column, no spray. Layer order cannot be read.
- Candidate roof group and translucent group are at the top or outside the frame. The purple box is not a candidate. [E]

## Earlier runs (toad_fountain, frames 100 to 148) [M]
- Three runs: HEAD, 29f504d7b (lane final, sha256 0df21e8c), final6 (lane toadf6, sha256 ebeb4902). 13 shots each. No fountain in view.
- Top shots: 0 of 13 byte-equal per pair. Differences 690 to 2100 px at x 70 to 144 (moving NPC sprites). Same-binary noise in notes is 300 to 1300 px, so build pairs cannot be separated from noise.

## Source path [C]
- Render mode to render index: src/model.c:1594 to 1655.
- Layer draw state (depth test, z write, decal bias): src/pm_gpu.c:738 to 749 (diag_toadvis.md).
- State skips: src/pm_gbi.c:2714, 2752, 3283 (diag_toadvis.md).
- Hikari panner: src/world/area_mac/mac_01/main.c:168, 182 to 183.

## Ranked candidates [E]
1. Translucent group at (-304, 39, -329) (rm 17 and 34) is the water overlay. Not yet seen. Its z write or draw order against the roof blocker decides the look.
2. Roof group (rm 1) is the blocker. hikari overlay has no blend mode, so it cannot be the water alone.
3. G2 equal-state skip (src/pm_gbi.c:2752) drops a mode word between blocker and overlay. Not measured.

## Next step
- Find a camera view of the translucent group. Use a stop further north or east. The purple box blocks the view from (-300, -240). Walk to (-240, -300) first and check the frame. [E]
- Compare builds only on a region that holds no NPC. Run a HEAD repeat first.

## Limits
- Azahar: no fountain frame yet.
- Texture names are blank on some leaves (set by script or variant). Not decoded.
- Shot writer runs at about 3 frames per second. Keep shot windows short.
