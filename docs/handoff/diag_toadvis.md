# Diagnosis: TOADVIS (Toad Town ground flicker, fountain water)

## Status
- Azahar budget used: 3 of 3 starts (row TOADVIS). No starts left.
- Fountain not in view in any shot. Water cause NOT found. Flicker cause NOT found.
- No game source edit. Scenario toad_gate added (unverified).

## Measured [M]
- Live log build3ds/hw_live_log_170.txt (build 1467, git 06e62c0aa), mac_01 window (lines 261 to 392):
  - texture failures 0 (line 264). No texfail lines. No evict lines in the map.
  - vram acct: A used 3031 KB, free 41 KB, min 41 KB (line 265). B the same. Map vram free 82 KB of 6144 KB, min 82 KB (line 266).
  - tex-cache 1971 KB peak 1971 KB (line 267). Linear used 8008 KB.
  - grabs 0 on every b7 line. splits: dr1 f900 splits 957 (kick, line 305).
  - dlc2 f899: static grp 9944, tri 109886, fail 0, full 0, hashbad 0, missing 0.
  - V05 "slots are used again" (line 243) is on map area 5 map 5, frame 676. Not mac_01.
  - Frame spikes 51 to 130 ms on mac_01 (frames 741 to 908). Not the picture cause by itself.
- Azahar toad_gate (preset=toad_west, mac_00 entry 0, shots at frames 200 and 204):
  - Run 1 working tree (build3ds/pm_3ds.3dsx). Shots kept, log check failed (regex escape, fixed). Log: build3ds/azt/toad_gate_run0_working_log.txt.
  - Run 2 build 29f504d7b (lane toadA). Log checks 2 of 2.
  - Run 3 build 3dcfc661d = final6 code (lane toadB). Log checks 2 of 2.
  - Picture (read by agent): west gate path, grass, stone wall, red edge at right. No water, no fountain.
  - shot_cmp toadA vs toadB: TOP 0 of 2 byte-equal. Difference 17 px per shot, one column x=120, y 220 to 254. Not the water.
  - Working tree vs toadA: differs (shot_cmp exit 1). Pixel count not measured.
- Working tree is not HEAD. It holds uncommitted LB1 edits in 3ds/src/pm_gpu.c (pm_gpu_begin_wait, deferred GPU begin when pm_interp_on > 0) and 3ds/src/pm_nusys.c. Those edits were not in the console build.

## Code [C]
- The fountain is not a named model in mac_01. include/mapfs/mac_01_shape.h names are MODEL_o### or group names (no mizu, water or fountain). The gate models are MODEL_west_gate, MODEL_east_gate, MODEL_castle_gate (lines 42 to 46).
- Water fountain FX (fx_water_fountain, include/effects.h:1590) is called only from src/battle/partner/sushie.c and src/common/SpitInk.inc.c. No mac_01 script calls it. So the map fountain is a shape model, or a script call not found yet.
- Depth and blend switches for a layer: src/pm_gpu.c:738 to 749 (depth test GREATER or GEQUAL for decal, write mask from r_zupd, decal bias r_decal ? decal_bias() : 0). Rect path 997 to 1007.
- State skips that can carry a render mode across layers: 3ds/src/pm_gbi.c:2714 (G1 g1_skip, sync and no-op), :2752 (G2, equal-state skip), :3283 (G1 skip in the DL loop).
- Kick split: 3ds/src/pm_gpu.c:551 to 563 (kick_check, second split at 85 percent, kick_done). split_frame at :280 with a clear only on the first part. The depth buffer is not cleared between parts.

## Ranked candidates (static, not measured) [E]
1. Blocker drawn with the XLU or zupd=0 render mode, so it writes no depth or no colour. The overlay then shows the background. Check: log r_zupd, r_blend and r_zcmp per draw for the fountain draws (model ID from the model tree).
2. G2 equal-state skip (pm_gbi.c:2752) drops a SETOTHERMODE between blocker and overlay. Check: count G2 skips per draw in the fountain segment against the non-skipped sequence.
3. Model order in the display list. The 3DS path replays the DL in list order (no sort found in pm_gbi.c). Check: list the MODEL draw order of the fountain against the game DL order.
4. Ground flicker: VRAM at limit (free 41 KB min on mac_01). A texture slot reuse (V05 path, pm_gpu.c:1556 to 1563) or a cache halve may reuse a slot used this frame. Check: log slot key on each SETTIMG with a hit, on mac_01, for one frame with the flicker.

## Smallest next step (no edit yet)
- Find the fountain view: warp mac_01 entry 0 and face the plaza (camboom or preset), or decode mac_01_shape.bin model names to a fountain ID. Then one shot per build (29f504d7b, final6, HEAD clean) with the subject in frame.
- Picture check: the water overlay shows the blocker colour behind it at the basin edge.

## Limits
- Azahar does not reproduce or clear the issue: no shot holds the fountain. Result: not tested.
- Console differences not checked: depth precision (24 or 16 bit), the second split timing, linear memory timing.
