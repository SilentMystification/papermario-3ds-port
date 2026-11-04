# TRACE TEX GEOM (row TRTG), 2026-11-03

Branch native. Read-only, except this file. Azahar starts used: 1 of 8 (TRTG).
Status file: 3ds/logs/perf/status_TRTG.txt (1 line per start).
Binary: build3ds/pm_3ds.3dsx (not rebuilt). Log: build3ds/last_log.txt. Shot: build3ds/shots/0000_5_1.ppm (frame 200), 0001_5_1.ppm (frame 204).

## Run 1 (proof source for item 1)
Command: `PM_ROW=TRTG run_azahar.ps1 -Debug "warp=hos_01:0 shots shotwin=200:6 loglevel=debug drawlog=200:1 texlog texbind texstat" -Until "shot \d+ frame 205\b" -Timeout 25 -Force`
Map: hos_01 = Shooting Star Summit (src/world/area_hos/hos_01/hos_01.h:2). Entry 0, story -128. No pos= switch used (default camera). Scene reached at frame 200. The shot shows a blue rock wall on the left and the sky. Wall texture appears as a blotch pattern, not clearly garbled in the frame.

## 1. Shooting Star Summit wall: PARTLY PROVEN, mismatch NOT PROVEN
- Wall chunk (drawlog f200 #27, build3ds/last_log.txt:702): scr -5 118 297 336, tri 5, tex 1, timg 0x0100066c.
- Tile 0 at this draw: fmt 4 (I), siz 0 (4 bit), 64x64. Window sz 0 0 124 124 (0 to 31 texels in quarter texel units). Wrap words cms 0, cmt 0, masks 5, maskt 5 (repeat). Shift 0 0.
- Texture bind (build3ds/last_log.txt:138): slot 8, img 0x100066c, 64x64, fmt 4, siz 0, dirty 0.
- Tile 1 at the same draw (dd1 line, last_log.txt:703): fmt 0 siz 2, 16x16 (src 010002cc), wrap 0 0 4 4, shift 1 1, window 0 0 60 60.
- Finding: the window (31 texels) is shorter than the mask period (32 texels). No clamp and no mirror on this axis. The TEXWIN clamp case (tile 1, cmt clamp, mask 6, window crossing the period) does not apply to this chunk.
- Not proven: no texture word dump compares the slot words with the ROM texture. No reference shows the wrong texel pattern. Only the sampling mode is logged.
- Fix proposal: none yet. Next test: texture word dump of slot 8 (64x64 I4 at 0x0100066c) against the ROM bytes at the same texture address, and the PICA wrap register set for unit 0 for this draw.

## 2. Chuck Quizmo stretched geometry: NOT REACHED
- Code: src/effects/quizmo_stage.c, src/effects/quizmo_assistant.c, src/effects/quizmo_audience.c. Scenes with Quizmo NPC: src/world/area_dro/dro_01/npc.c, dro_02/npc.c.
- Lead (INFERRED, not proven): stage light display list D_09001518_39A0D8 (64x16 CI texture, SetTileSize 252x60, T clamp). Source: docs/handoff/agent_grab3.md.
- No start spent on this item. Test: warp to dro_01, Quizmo show, one shot with drawlog and texlog on the stage light draw.

## 3. Jr Troopa transform purple effect: NOT REACHED
- Code: src/battle/area/kmr_part_3/actor/jr_troopa.c (and mage, para, egg, final variants). Transform effect file not yet located in src/effects.
- No start spent on this item. Test: battle with Jr Troopa transform, one shot with drawlog on the purple draw (clamp words, alpha mode).

## 4. Toad Town sewer coins and item sprites: NOT REACHED
- The sewer map id is not found in src/world/world.c AREA list (no sew area). Coordinator must name the map id or the warp name.
- Coin and item sprite draws not located.
- No start spent on this item. Test: pos= warp to a sewer coin, one shot with drawlog and texlog on the coin draw, then one item sprite (same texture path check).

## Summary
| Item | Status | Proof | Next test |
|---|---|---|---|
| 1 wall | PARTLY PROVEN (chunk and mode) | build3ds/last_log.txt:702, :703, :138 | texture word dump of slot 8, unit 0 wrap words |
| 2 Quizmo | NOT REACHED | none | dro_01 Quizmo show, drawlog on stage light |
| 3 Jr Troopa | NOT REACHED | none | Jr Troopa transform, drawlog on purple draw |
| 4 sewer coins | NOT REACHED | none | sewer coin pos= warp, drawlog on coin and item |

Budget left after this report: 7 of 8 starts.
