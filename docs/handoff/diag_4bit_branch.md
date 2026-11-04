# diag_4bit_branch: pm_gbi.c 4-bit source branch (branch native)

Read-only investigation. Only this file is committed. No Azahar start (see Section 5).

## 1. Result

- The 4-bit branch (pm_gbi.c:3150-3154) is correct for every 4-bit LoadTile call site in src/ and include/. Status: PROVEN for the nibble model and the call sites listed; INFERRED for the RDP pitch rule (see 2).
- The branch is wrong for one pattern: a 4-bit SETTIMG (G_IM_SIZ_4b) followed by LoadTile. No call site in src/ uses it (grep: no gDPLoadTextureTile with G_IM_SIZ_4b).
- The 4-bit branch does not explain the sewer garble on the evidence in this file. Item coins and CI4 items use the 8-bit-wide image trick, which matches (Section 3).
- The coin address 00571b90 is not a 4-bit texture and not a sewer coin in build pm_3ds_1927-native (Section 4).

## 2. Semantics used

- SETTIMG width is in texels of the SETTIMG size. Row pitch in DRAM is width * size / 8 bytes. For 4 bit this is width / 2 bytes (include/PR/gbi.h:409 G_IM_SIZ_4b_BYTES 0; TXL2WORDS_4b = width / 16 words, include/PR/gbi.h:3276). INFERRED from the libultra macro convention.
- LoadTile sl and sh are texel coordinates in the SETTIMG size units. The 8-bit-wide trick passes uls << 1 (quarter texels), so the load origin is uls / 2 bytes (include/PR/gbi.h:4018-4030).
- LoadBlock is linear in DRAM and TMEM. The render tile samples row t at t * line * 8 bytes of TMEM, where line is the SETTILE line field in 64-bit words. INFERRED for 4 bit.
- Converter convention (3ds/src/pm_tex_tx1.h:28, tex_conv_test.py:79): stride is in texels, texel (x, y) is nibble y * stride + x, byte = nibble >> 1.
- Branch (pm_gbi.c:3150-3154): r_str = timg_stride (pm_gbi.c:3020, SETTIMG width). Source = timg + r_y * r_str + r_x (bytes). Stride = 2 * r_str texels. This is RDP-correct only when r_str is a byte count, i.e. SETTIMG size 8 bit. The same form appears at pm_gbi.c:2927 (m1 shaded sprite, m1.stride = timg_stride * 2) and pm_gbi.c:2970 (tile 1, V21: stride = l1stride * 2, offset t1x >> 1).

## 3. Call sites and the branch (table)

Branch tuple = (stride in texels, byte offset). RDP tuple = (stride in texels, nibble origin).

| Case | Decomp source | SETTIMG | LoadTile origin | RDP tuple | Branch tuple | Result |
|---|---|---|---|---|---|---|
| 8-bit-wide trick, full window | include/PR/gbi.h:4018 gDPLoadTextureTile_4b (users: src/model.c:3489, src/hud_element.c:537-561, src/draw_box.c:508); include/gbi_custom.h:6, 39 (ScrollMultiTile2_4b, ScrollTextureTile_4b); include/PR/gbi.h:4049 gDPLoadMultiTile_4b (src/model.c:3415) | 8b, width W/2 | 0,0 | stride W, origin 0 | stride W, offset 0 | MATCH |
| 8-bit-wide trick, even origin (8,8) | same | 8b, W/2 | 8,8 | stride W, nibble 8*W+8 | stride W, offset 8*(W/2)+4 | MATCH (host test) |
| 8-bit-wide trick, odd origin uls=3 | same, uls odd | 8b, W/2 | 3,0 | stride W, nibble 3 | stride W, offset 1 (nibble 2) | MISMATCH: branch starts 1 texel early. A byte pointer cannot start on an odd nibble. Call sites with odd uls: not found (INFERRED none). |
| Item CI4 (HUD cache) | src/item_entity.c:981-1010 (also 1002 for the other branch): gDPSetTextureImage CI 8b width 12, LoadTile 0,0,0x2E,0x5C, render tile 4b line 2, SetTileSize 0x400..0x45C | 8b, 12 | 0,0 | stride 24, origin 0 | stride 24, offset 0 | MATCH. SetTileSize origin 256 texels is the texcoord origin (V05), not the load origin. |
| Direct 4-bit SETTIMG + LoadTile | none in src/ or include/ | 4b, W=24 | 0,0 | stride 24, origin 0 | stride 48, offset 0 | MISMATCH: stride doubled (2x), data from row 2y. Not used. |
| Direct 4-bit SETTIMG + LoadTile, origin (8,8) | none | 4b, W=24 | 8,8 | stride 24, nibble 200 | stride 48, offset 200 | MISMATCH: offset 2x too far. Not used. |
| Block 4-bit (16-bit SETTIMG width 1, LoadBlock) | include/PR/gbi.h:3682 gDPLoadTextureBlock_4b (users: src/entity/sbk_omo/StarBoxLauncher.c:54 W128, Tweester.c:71 W64, src/filemenu/filemenu_msg.c:733 and 779 W16); include/gbi_custom.h:67 gDPScrollTextureBlock_4b | 16b width 1 | LoadBlock | linear, stride = W | stride = window w (= W) | MATCH. ld_valid = 0 after SETTIMG, so the no-offset path runs. |
| Block 4-bit, W=24 (not a decomp size) | none | block | - | TMEM line 2 words, row stride 32 texels (INFERRED) | stride 24 | MISMATCH if used. All decomp block widths are multiples of 16. |

Host test: 3ds/tools/tex_conv_test.py and tex_conv_drive.c, run with the request generator in the scratchpad (Section 6). Each row above was built as two requests, one with the RDP tuple and one with the branch tuple, on the same random 4-bit image, and both outputs compared byte for byte. Results: MATCH for the 8-bit-wide full window, even origin, item CI4, block W=32; MISMATCH for odd origin, direct 4-bit (two rows), block W=24. The odd-origin row cannot express the RDP origin in a byte pointer, so its mismatch is the analytic result (one texel early), not a converter output.

## 4. Coin texture 00571b90

- build3ds map: 3ds/logs/builds/pm_3ds_1927-native.map line 75178: 0x00571b90 is D_80242B70 in src/world/area_sam/sam_05/npc.c:47 (INCLUDE_IMG monstar_blank2.png). The next symbol is at 0x00571d90, so the object is 0x200 = 512 bytes.
- assets/us/world/area_sam/sam_05/monstar_blank2.png: 32 x 32, bit depth 8, colour type 4 (grey plus alpha). This is an 8-bit texel texture, not RGBA16 (2048 bytes) and not 4-bit.
- Conclusion: in build 1927 the address is not a sewer coin and not 4-bit. The key 00571b90 in azt_scenarios.json (texdump_coin, texreg_coin) is stale (TextureHeap moves; see test_catalog notes). Use a shape key (for example RGBA16 32 x 32) to find the coin.

## 5. Azahar start: not run

- Existing lines do not show the (stride, offset) that the branch passes per 4-bit bind. texword (pm_tex.c:783) logs stride, but only for the shape key (64 x 64 I4) and the first 4 binds. suntile (pm_gbi.c:731) logs the branch inputs without the tile size. texbind (pm_tex.c:789) logs fmt, siz, size, not the stride.
- A probe for the tuple needs a code change, and a scenario in 3ds/tools/azt_scenarios.json with a catalog row. Both are outside this read-only task.
- Next step (needs approval): one code row adds the texword probe for 4-bit binds (key: fmt 2 or 4, siz 0), then one scenario run on tik_03 (entry 0) with PM_ROW=B4, justification line first, cap 25 s.

## 6. Fix proposal (not applied)

- Branch at pm_gbi.c:3150-3154 and the m1 and V21 forms: keep the current form for SETTIMG size 8 bit (every decomp user above). For SETTIMG size 4 bit, use stride = r_str texels and nibble offset (r_y * r_str + r_x); the byte pointer is (nibble >> 1). For an odd nibble origin, keep the current floor and note the one-texel shift.
- Test script (scratchpad, not committed): C:\Users\Andrew\AppData\Local\Temp\claude\c--Users-Andrew-Documents-GitHub-papermario-3ds-port\bf7242ad-7188-4cf3-8ec3-af3ba8c281b2\scratchpad\b4\b4_cases.py. Run: `/c/Python314/python -I b4_cases.py <driver> <outdir>`.

## Open

- The sewer item and coin garble is not explained by the 4-bit stride. Next check: the item path's SetTileSize origin (src/item_entity.c, 0x400 texels) against the texcoords of the item quads. This needs a trace with the texcoords, not a guess.
