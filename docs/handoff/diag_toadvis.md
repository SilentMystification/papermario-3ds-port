# Diagnosis: TOADVIS (Toad Town ground flicker, water with the gate)

## Status
- Cause NOT found. Water not identified in a shot. Flicker not reproduced.
- Azahar: budget 3 starts after the reset, all used. Starts: toad_gate x3 (earlier budget), mac01t final6 and 29f504d7b, mac01s working tree.
- No game source edit. Scenarios toad_gate and mac01s added (unverified).

## Measured [M]
- Live log build3ds/hw_live_log_170.txt (console build 1467, git 06e62c0aa), mac_01 window (lines 261 to 392):
  - texture failures 0 (line 264). No texfail lines. No texstat or texmap lines (switch off).
  - vram acct A used 3031 KB, free 41 KB, min 41 KB (line 265). Map vram free 82 KB of 6144 KB.
  - texcache halves at boot only (lines 32 and 33). No shrink, drain or regrow line in the mac_01 window.
  - tex-cache 1971 KB peak 1971 KB (line 267). grabs 0 on every b7 line.
  - dlc2 f899: static grp 9944, tri 109886, fail 0, hashbad 0, missing 0.
  - V05 "slots are used again" (line 243) is map area 5 map 5, frame 676. Not mac_01.
- Azahar toad_gate (preset toad_west, mac_00 entry 0, shots 200 and 204): west gate path, grass, wall. No water, no fountain.
  - 29f504d7b vs final6: 17 px differ per shot, column x=120, y 220 to 254. Not the water.
- Azahar mac01s (tour to mac_01 entry 0, shots 500 and 504), the Toad Town street. The strip at the right edge (light blue, behind the wall) is the only candidate water. It is not confirmed as water.
  - final6 (3dcfc661d), 29f504d7b, working tree: 2 of 2 top shots byte-equal across all three.
- Working tree is not HEAD: uncommitted LB1 in 3ds/src/pm_gpu.c and 3ds/src/pm_nusys.c (deferred GPU begin when pm_interp_on > 0).
- Fault: mac01t until `mdlsum f899 r12` never printed on 29f504d7b. The run idled 121 s to the timeout (3ds/logs/test_budget.csv, 17:08:15 to 17:10:27). Scenario mac01s uses a shot line instead.

## Water and fountain in source [C]
- MODEL_water (0xD6, include/mapfs/mac_00_shape.h:9) is in mac_00: texture-panned (src/world/area_mac/mac_00/main.c:97), custom gfx gfx_build_water (water_gfx.c:6-31, mdl_draw_hidden_panel_surface), collider COLLIDER_water with SURFACE_TYPE_WATER (main.c:95). This is the harbour water, not the mac_01 gate.
- MODEL_hikari (0x2D, include/mapfs/mac_01_shape.h:161) is in mac_01: texture-panned with two panners (src/world/area_mac/mac_01/main.c:168, 182-183). Candidate for the mac_01 water surface. Its basin or blocker model is not named.
- mac_01 gate models: MODEL_east_gate (0xA3), MODEL_west_gate (0xA0), MODEL_castle_gate (0xA4) (mac_01_shape.h:42-46), rotated by main.c:82-91.
- fx_water_fountain (include/effects.h:1590) is called only from src/battle/partner/sushie.c and src/common/SpitInk.inc.c. No mac_01 script calls it.
- Draw path for a layer: src/pm_gpu.c:738-749 (depth test GREATER or GEQUAL, write mask by r_zupd, decal bias). State skips: pm_gbi.c:2714 (G1), :2752 (G2), :3283.
- Kick split: pm_gpu.c:551-563, second split at 85 percent. The depth buffer is not cleared between parts (only the first part clears the colour target, split_frame at :280).

## Ranked candidates [E]
1. Blocker drawn with a transparent or no-depth-write mode, so the overlay shows the background (MODEL_hikari path). Check: log r_zupd, r_blend, r_zcmp per draw of the hikari segment.
2. G2 equal-state skip (pm_gbi.c:2752) drops a mode word between blocker and overlay. Check: per-draw mode word with and without the skip.
3. Ground flicker: VRAM at 41 KB free. A slot reuse or eviction of a texture in use is possible, but no texture failure and no shrink line were logged on mac_01. Eviction counts not measured (texstat off).

## Smallest next step (no edit yet)
- One run with `texstat` and the mac_01 window on the console build, and log the per-draw modes of MODEL_hikari.
- Find a view with MODEL_hikari in frame: a camera on the plaza, not the street. Check the shot before any build compare.

## Limits
- Azahar: no fountain or gate water in any shot. Not reproduced. Budget used up.
- Console items not checked: depth precision (24 or 16 bit), second split timing, linear memory timing.
