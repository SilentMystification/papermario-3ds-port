# trace_coin_item: coin and item texture garble (branch native)

Investigation. Report only. No source edit. Test row CIT. Azahar starts: 4 of 6 (coin_kmr03, scenario added to 3ds/tools/azt_scenarios.json, not committed).

## 1. Result

- Coin found: kmr_03 field coin (ITEM_COIN, src/world/area_kmr/kmr_03/entity.c:61-64, x 345, z -250, y 205 to 280). Sewer coin not found in a new shot. Sewer blocks (tik_03 entity.c:7, 9, ITEM_COIN in Entity_YellowBlock) show only the block face.
- Garble on the coin: NOT PROVEN. At the frame used (pm frame 250, run 2) the coin is clipped at the screen edge (about 12 px visible). The shot does not show the texture.
- Draw found (PROVEN by drawlog, run 2): chunk #20 at frame 250. Item quad 24 x 24, CI4 (fmt 2, siz 0), timg 0x00d0f7e0, screen box 149 3 173 28. Sparkle chunk #21 at 8 x 8, box 161 12 170 20.
- Logged tile state of chunk #20: tile window 1024 1024 1052 1052 (quarter texels, 256 to 263 texels). Code sets 0x400 to 0x45C (256 to 279 texels) at src/item_entity.c:1008-1010 (4-bit path, 0x45C). The window differs in the lower right corner. Cause INFERRED (the log may read a state set after the item draw). Not checked.
- Cause of garble: NOT PROVEN.

## 2. Runs (scenario coin_kmr03, lane native, PM_ROW=CIT)

| Start | Warp and pos | Shots | Result |
|---|---|---|---|
| 1 | kmr_03 entry 0, no pos, shotwin 20:6 | pm frames 20 to 25 | Boot test triangle only. Map loads at pm frame about 203 (log: `warp: scene frame0 49` after shots). |
| 2 | pos 345:75:-240, shotwin 250:6, drawlog 250:1 | 0000 (frame 250), 0001 (254) | Coin chunk #20 at top right edge, clipped. Shot: build3ds/azt/shots/coin_kmr03/0000_0_2.ppm |
| 3 | pos 345:230:-240, shotwin 250:4 | 0000 (frame 250) | Mario settles at y 191. No 24 x 24 coin chunk in frame 250. Coin above the screen top. |
| 4 | pos 345:75:-200, shotwin 250:2 | 0000 (frame 250) | Mario settles at y 50. No 24 x 24 coin chunk. Picture equal to run 3. |

Sewer: not tried in a new start. Old shot (build3ds/shots/0000_2_2.ppm, build of 2026-11-03, tik_03 entry 0) shows three yellow ITEM_COIN blocks with a curved white glyph (the "?" face, rotated by the portrait shot). No coin item in that shot.

## 3. Screen mapping (INFERRED)

The PPM is 240 x 400 (top screen turned). Fit from runs 2 and 4: shot x = 239 - scr y, shot y = scr x + 40. The coin in run 2 maps to shot x 211 to 236, y 189 to 213, which matches the clipped object at the right edge. The visible area ends at shot x 220 (black beyond).

## 4. Texture and draw data (run 2, frame 250, log build3ds/lane-native/last_log.txt lines 342 to 344)

- dd line #20: `src 005a9b08 geom 00000005 tex 1 timg 00d0f7e0 fmt 2 siz 0 24x24 tri 2 scr 149 3 173 28 tw 00000002 00000002 00000008 00000008 ts 0 0 sz 1024 1024 1052 1052`.
- The tw words are the tile wrap state (pm_gbi.c:310). Value 2 is clamp in s and t, mask 8. Mask 8 is not the mask of a 24-texel tile (INFERRED from the mask of other chunks).
- The dd1 line of chunk #20 reports tile 1 as the map wall (src 010036ac 32x64). The coin uses tile 0 only.

## 5. Next steps (not run)

1. One start with `texword=00d0f7e0@250` and the same kmr_03 window (texword dumps the first 4 binds from frame 250). Check that the address still matches in this build. The address moves between builds (see test_catalog.md notes).
2. Decode the dump with 3ds/tools/texdump_png.py (CI4 needs a new decoder, the tool reads I4 only).
3. Compare the decoded coin with the shot. Pixels in the shot at the clipped object are too few for a garble judgement. Use a position with the coin at screen centre (not found in starts 3 and 4).
4. Sewer: one start at tik_03 with Mario under a yellow block (block position 140 50 -75, floor not known). Read the block face with a 3x crop.

## 6. Notes

- Scenario coin_kmr03 is in 3ds/tools/azt_scenarios.json (uncommitted). It needs a catalog row in the same commit (orchestrator).
- Shots and logs: build3ds/azt/shots/coin_kmr03/, build3ds/lane-native/last_log.txt (run 4).
- Test status line: 3ds/logs/perf/status_CIT.txt (4 start lines).

## 7. Update after the coordinator note (start 5, CIT 5 of 6)

- Retraction: point 3 (tile window 256 to 263 vs code 256 to 279) is withdrawn. dd_line prints the tile state at the next chunk, so the window belongs to the 8 x 8 sparkle (chunk #21), not the coin. No mismatch is claimed.
- Run 5: `warp=kmr_03:0 pos=345:75:-280 shotwin=280:2 drawlog=280:1 texword=00d0f7e0@250`. Placement settled at y -289.7 (Mario at z -280). texword binds of 00d0f7e0 at frames 250, 269, 285, 305: slot 61, 24 x 24, CI4 (fmt 2 siz 0), stride 24, slen 288, GPU 32 x 32, gn 2048 (RGBA16 after the palette), wrap 0 0, filter 1, hash c7c2b4f6.
- Source (texsrc_00d0f7e0_1.bin, CI4 indices, decoded in the scratchpad): a clean coin outline, index 0 outside the disc, indices 1 to 7 inside. The disc spans x 5 to 19 and y 4 to 20.
- Converted texture (texgpu_00d0f7e0_1.bin, decoded as 8 x 8 Morton tiles, row 0 at the top): the disc has the same shape. Colours: tan, black, blue-violet centre line, cyan rim. The area of index 0 is opaque blue-violet, not transparent. PROVEN for the texel layout and the shape. INFERRED for the palette: the colours do not look like a gold coin, so the palette base or entry 0 alpha is likely wrong. Palette is not dumped, so this is not proven.
- Picture: no top shot file reached build3ds/azt/shots/coin_kmr03 in run 5 (the azt folder keeps the 01:23 frame). The shot is missing, so the picture check failed. The coin as seen on screen is NOT shown.
- Coin position on screen: runs 2 (z -240) show the coin clipped at the top edge. Runs 3 and 4 show no coin draw at frame 250. The camera at frame 250 may still be the start camera (runs 3 and 4 shots are equal).
- Starts used: 5 of 6 (CIT). One start remains.

Decode scripts (scratchpad, not committed): gpu_morton.py (8 x 8 Morton tiles), ci4_idx.py (CI4 index print), rgba16_png.py (linear RGBA16, wrong for PICA tiles).
