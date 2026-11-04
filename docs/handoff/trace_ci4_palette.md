# trace_ci4_palette: coin CI4 palette and TLUT path (branch native)

Investigation. Row CPAL. Azahar starts: 2 of 4 (scenario coin_kmr03, cap 45 s from the catalog; azt.ps1 has no cap option). Status: 3ds/logs/perf/status_CPAL.txt.

## 1. Result

- PROVEN: the coin 24x24 CI4 bind uses its own palette (tile 0 palette 0, TMEM slot 0). Bytes at frames 245 to 247 equal the palette of assets/us/icon/anim/coin_0.png (16 entries, RGBA16). Code: item_entity.c:979 and 1014 (`gDPLoadTLUT_pal16(..., 0, ...)`), pm_gbi.c:3080-3087 (slot = (tile TMEM - 256) / 16), pm_gbi.c:3195 (`pal0 = tlut_pal[tile_pal[0]]`).
- PROVEN: palette entry 0 is 0x294a, alpha bit 0. Index 0 is transparent. The converted texture (texgpu_00d0f7e0_1.bin, frame 250) holds only the 10 palette values. Its little-endian 16-bit values match the palette: 10 of 16 found in LE, 0 in BE.
- PROVEN: the TLUT type is RGBA16. The item display list ends with `gsDPSetTextureLUT(G_TT_RGBA16)` (item_entity.c, D_8014C620). IA16 does not occur for coins.
- INFERRED: the trace colours (tan, black, blue-violet, opaque index 0) come from a big-endian read of little-endian texel bytes. BE reading of 0x294a gives 0x4a29, blue-violet with alpha 1. BE reading of 0xeed5 gives 0xd5ee, tan.
- NOT PROVEN: the cause of the visible garble. Shot build3ds/azt/shots/coin_kmr03/0000_0_2.ppm (frame 280) shows a gold ring with a blue centre near Mario. The coin palette has no blue entry, so the blue comes from the draw stage. The chunk at frame 280 (dd #20, 24x24, screen box 146 -2 173 26) is at the top edge and may not be the ring in the shot.

## 2. Hypotheses

| # | Hypothesis | Result |
|---|---|---|
| H1 | Palette select ignored or wrong | Refuted for the coin binds (tile palette 0, slot 0 = coin palette). Slot 1 holds the shading palette (values ffff07f1..., written by sprite_shading.c:413). Tile palette 1 is used only by the shaded branch (item_entity.c:985 region), not seen at coin binds. |
| H2 | TLUT type IA16 read as RGBA16 | Refuted for the item. RGBA16 is set, the port decodes RGBA5551. |
| H3 | Alpha of index 0 | Refuted. Entry 0 alpha is 0. |

## 3. Palette decode (frame 245 bind, memory order, 2 bytes per entry, big-endian)

| idx | bytes | RGBA5551 (correct) | IA16 (I, A) | LE read of the same bytes (trace) |
|---|---|---|---|---|
| 0 | 29 4a | (5,5,5), alpha 0, transparent | I 41, A 74 | 0x4a29 = (9,8,20), alpha 1, blue-violet opaque |
| 2 | ee d5 | (29,27,10), alpha 1, gold | I 238, A 213 | 0xd5ee = (26,23,23), alpha 0, tan |
| 12 | 00 01 | (0,0,0), alpha 1, black | I 0, A 1 | 0x0100 = (0,4,0), alpha 0, black |

The correct reading is RGBA5551 big-endian (the ROM and memory order). It gives a gold disc with transparent index 0. IA16 gives grey or white discs.

## 4. Shot problem (cause found in code, observed in runs)

- pm_gpu.c:1596 logs `shot N frame F` before `fopen` (pm_gpu.c:1603). `fclose` is at pm_gpu.c:1620.
- run_azahar.ps1:288 breaks on the until match. run_azahar.ps1:315 kills Azahar. The loop sleeps 0.5 s per pass (run_azahar.ps1:264).
- A matched shot line kills Azahar before the PPM write finishes. The file is then missing or partial. Trace run 5 matched `shot 1 frame 281` and wrote no file.
- Start 2 of this row did not reach frame 281 in 45 s. It wrote 0000_0_2.ppm (frame 280), so the file exists when no match kills the run.

## 5. Fix proposal

1. No palette or TLUT change is needed for the coin texel data.
2. Trace decoders: read the converted texture as little-endian 16-bit.
3. Shot: move the log line after `fclose` (pm_gpu.c, after line 1620), or add a `shot saved` line and use it as the until.
4. Garble: next probe at the coin draw (24x24 chunk). Print the combine state (comb_a, comb_c, dual) and the tile 1 state. Confirm which object the blue ring is.

## 6. State

- Debug print (PALX, pm_gbi.c) was reverted. `git diff` on pm_gbi.c is empty. The native lane was rebuilt from HEAD (BUILD_RC 0).
- map_restore_table.c was regenerated for each build and restored (byte-equal to the backup).
- Raw dumps: AppData/Roaming/Azahar/sdmc/3ds/PaperMario/texgpu_00d0f7e0_1..4.bin, texsrc_00d0f7e0_1.bin.
