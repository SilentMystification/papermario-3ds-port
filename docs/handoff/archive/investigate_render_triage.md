# Investigation: console render triage (7 items), 2026-11-03

Branch native, HEAD 8fca53deb. Read-only. No Azahar start used (RTRI budget 0 of 6). Every cause below is INFERRED from code reading. None is PROVEN. A test in the last column decides each one.

Known state from the handoffs:
- TEXWIN (1d6beb014, wall band fix) is reverted in HEAD by e89900035 (merged by 8c382352c). The wall band is open again on this branch.
- The TEXGARBLE scratch switches texclamp, texclamp1, texrep1 and texnear are still in pm_tex.c (lines 1583, 1671, 1672, 1584/1672).
- V16b (white box around a flashing sprite) and V18/PT23 (depth precision family, near-plane clip V27) are open, console only.

## Groups

### Group A: texture clamp and pad (items 3, 6, 7; maybe 2)
Mechanism (INFERRED):
- pm_tex.c:269-277 `pm_tex_set_wrap` stores the mirror bit (mk_ms, mk_mt) and the mask, but not the clamp bit.
- pm_tex.c:128-132 `wrap_of` maps clamp to GPU_CLAMP_TO_EDGE. The GPU clamp reads the padded texture, because pm_gbi.c:800-808 divides by the padded size from pm_tex.c:1598-1609.
- pm_tex.c:336 and 376 (`expand`) and the tx1 path (pm_tex_tx1.h, tx1_map) pad the region past the window with the mask period (wrap_mod). A clamped coordinate past the window edge then reads repeated texels, not the edge texel.
- Lead for item 6 and 7: a quarter texel offset at pm_gbi.c:2519-2520 and 3110-3111 (PT38, tile_fx and tile_fy from ult and uls & 3). The bilinear filter reads one texel past the window edge.
Joins: TEXGARBLE wall band (TEXWIN, reverted), texture garble (review_tex_cache, DMA race, not re-checked here), PT58.
Fix proposal: store the clamp bit with the mask (new static mk_cs, mk_ct in pm_tex_set_wrap). In the pad and tx1 map, a clamp axis replicates the edge texel (sx = w - 1) instead of the mask period. One change in pm_tex.c covers all loads. Byte-equal check on yard and mac01t before merge (shared draw code rule).
Test: warp to the Koopa Bros fortress spike (item 6) and to nok_14 (27, 0, -44) (item 3). Compare texclamp1=1 against the default, one start each. Pass: the line or the band changes, and the picture still shows the subject.

### Item 1: nok_02 path flicker, end piece at x 250 y 0 z 11 vanishes
Probable cause (INFERRED): screen-space cull sign flip near the eye.
- pm_gbi.c:1693-1694: `cross = (ex1*ey2 - ey1*ex2) * (p1->w * p3->w)`, then `cross = -cross` when an odd number of w are negative.
- The product ex1*ey2 - ey1*ex2 equals w1 w2^2 w3 times D, where D is the screen area of X = x/w. Multiplying by w1 w3 gives w1^2 w2^2 w3^2 D. The sign of cross is therefore the sign of D with no extra flip. The extra flip for odd negative w is then wrong, unless the near clip keeps w < 0 vertices (clip_near, V27).
- Same test is in the early path at pm_gbi.c:1631. Both copies need the same fix.
- A triangle with one vertex near w = 0 flips its cull result from frame to frame when the camera moves. The whole piece then vanishes. The flicker of the whole path matches a sign test that changes at w = 0.
Joins: V18 (road flicker, near plane V27), PT23 (Toad Town geometry), V19 (shadow flicker). All open, console only.
Fix proposal: remove the odd-negative-w flip only after a check that clip_near never leaves w < 0 (read pm_gbi.c clip_near first). Use one helper for both copies (pm_gbi.c:1631 and 1693).
Test: warp to nok_02 at pos 250 0 11. Shotwin series of 30 frames during a camera move. Count the frames where the end piece is missing. One start with the flip on and one with a debug switch that removes it.

### Item 2: nok_14 (-70, 0, -12) sky stretch at top left
Probable cause (INFERRED): sky rows wrap at the top of the sky image, and the wrap is drawn with a clamp window.
- background.c:118-126 `bg_sky_rows` draws the sky with texrects of 1 texel per row (dtdy 1024). Row t/32 is the source row.
- background.c:298 comment: the screen rows come from source rows y0 onward, then from 0 to y0 - 1. A texrect that crosses the wrap point reads t past the end of the 200 row image, and the clamp maps it to the edge row. The sky then stretches from the top.
- Flicker while moving: y0 changes with the camera pitch, so the wrap row moves.
Joins: PT58 (wide sky fill, fixed for sam_03), BGROW (1 px sky row, parked).
Fix proposal: split each sky draw at the wrap row into two texrects (rows y0 to 199, then 0 to y0 - 1). Do not clamp the sky T axis. One change in background.c.
Test: warp to nok_14 at -70 0 -12. Shotwin 30 frames during a pitch change. Check the top rows of the sky.

### Item 3: nok_14 underwater terrain near the lake (27, 0, -44), wrong textures
Probable cause (INFERRED): Group A (wall band family). Tile 1 or unit 0 clamp with a window shorter than its mask.
Joins: TEXGARBLE wall band (TEXWIN, reverted in HEAD). Test in Group A.

### Item 4: blue switch break, transparent square rises above the button
Probable cause (INFERRED, low confidence): the exclamation mark quad of the Blue Switch.
- src/entity/model/BlueSwitch.c:24-38: `Entity_BlueSwitch_LoadExclMark` loads a 16 by 32 IA 4 bit texture (4b), mode MODULATEIA, clamp on both axes.
- BlueSwitch.c (matrix `Entity_BlueSwitch_mtxExclMark`) moves it 6 units up. The square rises above the button.
- The port maps IA 4 bit to LA4 (pm_tex.c:295, 298). The conversion at pm_tex_tx1.h:108-109 (alpha = nibble & 1 ? 15 : 0, intensity = 3 bit expanded) reads as correct. So the square comes from another cause: a texture window that is wider than the 16 by 32 image (Group A pad), or the translucent BlueBubble surface (G_RM_AA_ZB_XLU_SURF, BlueSwitch.c, Entity_BlueSwitch_RenderBlueBubble).
Joins: none known. Group A is the first test.
Test: warp to the blue switch scene. One drawlog on the exclamation draw (tile fmt, size, window). Then the Group A switch.

### Item 5: grey box outline around Mario
Probable cause (INFERRED): decal two pass stencil, the V16b family.
- pm_gpu.c:890-899: the decal path marks the stencil in pass 1 and writes a constant colour in pass 2 (stc_set 2, GPU_EQUAL, GPU_STENCIL_ZERO). A decal that is not alpha tested draws its full quad, so a rectangle appears.
- Mario's shadow is a decal (shadow1 and shadow2 vtx, assets/us/entity/vtx). The stencil area is the quad, not the shadow shape.
- DECAL_EPS (pm_gpu.c:892) and decal_bias 0.001 (pm_gpu.c:485-489, switch decalbias) are the same depth family as V19b.
Joins: V16b (white box), V19b (decal bias not seen in Azahar).
Fix proposal: alpha test the decal pass (r_atest) so transparent texels write no stencil. One change in pm_gpu.c.
Test: any scene with Mario in shadow, one shot with decalbias=0 and one default. The box must change. Not run.

### Item 6: Koopa Bros fortress, horizontal line across the top of the spike texture
Group A (texture clamp and pad). The line sits on the first texel row past the window top, which is the clamp edge. Test as in Group A.

### Item 7: star spirit card after Koopa Bros, line through the card, spirit misaligned
Probable cause (INFERRED): Group A plus the quarter texel offset.
- The misalignment matches the ult and uls & 3 offset (pm_gbi.c:3110-3111, PT38). A card that uses a window with ult & 3 != 0 is shifted by up to 0.75 texel, and the bilinear filter reads one row of the window edge (the line).
- Joins: PT07, PT08 (star spirit talk and text, no repro in Azahar).
Test: Group A test, then the star spirit card with ult & 3 on and off (the code has no switch for this; add one only if the Group A test does not change the card).

## Triage table

| Item | Group | Cause | Probable | Joins | Test |
|---|---|---|---|---|---|
| 1 | B depth and cull | pm_gbi.c:1693-1694 sign flip for odd negative w | INFERRED | V18, PT23, V27 | nok_02 pos 250 0 11, 30 frame move |
| 2 | C sky wrap | background.c:118-126, 298 wrap row clamped | INFERRED | PT58, BGROW | nok_14 -70 0 -12, pitch move |
| 3 | A texture clamp | pm_tex.c:269-277, 336, 376 | INFERRED | TEXGARBLE wall band | nok_14 27 0 -44, texclamp1 |
| 4 | separate | exclamation quad or BlueBubble | INFERRED, low | none | blue switch drawlog |
| 5 | D decal stencil | pm_gpu.c:890-899 no alpha test | INFERRED | V16b, V19b | Mario shadow, decalbias 0 |
| 6 | A texture clamp | pm_tex.c:269-277, 336, 376 | INFERRED | TEXGARBLE | Koopa Bros spike, texclamp1 |
| 7 | A plus quarter texel | pm_gbi.c:3110-3111 | INFERRED | PT07, PT08 | star spirit card, after Group A |

## Next steps (in order)
1. Group A fix (store the clamp bit, replicate the edge in pad and tx1 map). One code agent, one row. Byte-equal check on yard and mac01t.
2. Item 2: split the sky draw at the wrap row (background.c).
3. Item 1: read clip_near (V27) and confirm w >= 0 after the clip. Then decide on the flip (pm_gbi.c:1693 and 1631).
4. Item 5: alpha test the decal pass, if the decal bias test points to the decal.
5. Items 4 and 7: after Group A. Item 4 needs a drawlog first.

No code was changed in this investigation.
