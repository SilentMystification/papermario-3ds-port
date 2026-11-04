# Investigation: THUNDER-BOX yellow rectangle (fourth pass)

Scope: read only. No source, asset or commit change. Shots: build3ds/azt/shots/thunder_box/0001_0_0.ppm (frame 256), 0002_0_0.ppm (frame 260). PPM 240 x 400.

## Result
- Cause not proven. Best candidate: the bolt triangle draw `gSPDisplayList(dlist)` at `src/effects/lightning.c:279`, painted with the PRIM set at `src/effects/lightning.c:273`. Confidence: low.
- Two earlier claims are wrong:
  1. Investigate pass 2 called `D_09000950_3939F0` the battle quad that is drawn. It is not. That list (`assets/us/effects/gfx/D_09000950_3939F0.gfx.inc.c`) holds state only: no `gsSPVertex`, no triangles, no texrect. It cannot paint a rectangle.
  2. The quad geometry `D_09000990_3B5CD0` (vertices `D_09000950_3B5C90`, 24 x 16 units) is included only by `src/effects/gfx/squirt.c:8`. The flashing box path (`flashing_box_shockwave.c:184-185`) never draws it.
- So the battle flashing box path (`src/effects/flashing_box_shockwave.c:173-187`) draws only the mesh `D_09000FF8_394098` at line 185. Run 10 ruled out that mesh.

## Full-frame draw audit (battle frame, yellow or rectangle-capable draws)
Each row lists a draw that can paint a rectangle, and what it uses. Status is from code reading or shot evidence.

| Draw | file:line | Value used | Status |
|---|---|---|---|
| Bolt triangles | `src/effects/lightning.c:279` gSPDisplayList(dlist) | list `D_09003378_3720D8` (`assets/us/effects/gfx/D_09003378_3720D8.gfx.inc.c:7-15`), 8 quads of 2 tris from 32 verts | Not tested. Lead. |
| Bolt PRIM | `src/effects/lightning.c:273` gDPSetPrimColor(0,0,255,255,0,unk_10) | RGB (255,255,0), alpha `unk_10` (255 start, -13 per frame from frame 7, `lightning.c:191-196`) | Not tested. Lead. |
| Bolt state | `src/effects/lightning.c:241` gSPDisplayList(dlist2) | `D_09003948_3726A8` (`...gfx.inc.c:5-16`): 2-cycle, I8 texture `D_09000000_36ED60` 128 x 128, mirror+wrap, masks 7 and 5, ENV alpha 0x80 | Part of lead. |
| Bolt tile 1 size | `src/effects/lightning.c:278` gDPSetTileSize(1, uls2, ult2, uls2+1020, ult2+124) | tile 1 spans 255 texels wide on a 128 texel texture. Wrap seams fall inside the bolt. | Open lead (texel seam). |
| Shockwave mesh | `src/effects/flashing_box_shockwave.c:185` gSPDisplayList(D_09000FF8_394098) | PRIM (255,255,0,unk_18) | Ruled out (run 10). |
| Battle state list | `src/effects/flashing_box_shockwave.c:184` gSPDisplayList(D_09000950_3939F0) | state only | Cannot paint. Ruled out. |
| Full viewport fill | `src/battle/16C8E0.c:849` and `:856` gDPFillRectangle | depth fill, then `bgColor` fill when `BACKGROUND_FLAG_TEXTURE` is off | Full viewport, not a rectangle. Shot shows the scene. Ruled out. |
| Box fill to colour image | `src/draw_box.c:775-777` gDPFillRectangle to `mdl_address` | `primR/G/B` (box colour), target is a side buffer, not the screen | Only if this box is in the frame and composited. Unverified. Low. |
| Full screen flash | `src/effects/star_spirits_energy.c:396` gSPTextureRectangle full screen | PRIM (208,208,208) | Not yellow. Ruled out. |
| Item outline | `src/effects/got_item_outline.c:128` gDPSetEnvColor(255,255,0,255) | ENV yellow, type != 0 | Only with an item outline active. Unverified. Low. |

## Why the bolt triangles are the lead
- The bolt is the only yellow draw in the battle path that has straight-edged geometry of rectangle size. Each quad is 2 triangles of a strip, with UV reaching about 469 texels (investigate pass 3).
- The shot (frame 260, crop from the 2x scaled copy) shows a pale yellow panel at about x 89-100, y 190-218. It has vertical hard edges and two stripes. It sits left of the figure at about x 116, y 200 (the top figure in the lower-left group). It lies on the bolt path.
- Pixel count: strict yellow (R>200, G>200, B<80) is 250 px at frame 260. The region is not a solid block of that colour. Pale yellow is the mix of PRIM and texel. That fits a texel-modulated bolt quad, not a flat fill.

## Shot evidence (measured with PIL on the PPM)
- Frame 260 strict-yellow bounding box: x 89-232, y 89-310. Bands: y 100-200 has 134 px at x 202-232 (the bolt); y 200-300 has 89 px spanning x 89-232 (the panel plus the bolt).
- Frame 256: 283 px, same layout.
- The panel is the only hard-edged rectangle in the crop. Its source is the open question.

## What settles it
- Test B2 (one quantity: bolt triangles). Edit `src/effects/lightning.c:279`: skip `gSPDisplayList(dlist)` for `unk_00 >= 3`. Shot at frame 260 (shotwin around 252:12, until frame 260). If the panel is gone, the bolt triangles paint it. If it stays, the bolt triangles are ruled out. Revert the edit after the shot if the shot does not change.
- Test B1 (one quantity: bolt PRIM colour, run only if B2 removes the panel). Edit `src/effects/lightning.c:273`: PRIM (255,255,0) to (255,255,255) for `unk_00 >= 3`. If the panel turns white, the PRIM at that draw is the paint source. If it stays yellow, the colour comes from the texture (TEXEL0/TEXEL1), so the next test is the tile 1 seam (`lightning.c:278`).
- Each test uses one start. The shot must show the panel at the same window. Do not change two lines in one shot.

## Open items
- Whether `lightning_render` (`lightning.c:213-218`, `RENDER_TASK_FLAG_REFLECT_FLOOR`) draws the bolt a second time. Not checked.
- Whether `src/effects/gfx/squirt.c` runs in this attract battle. Not checked. If it does, the quad `D_09000990_3B5CD0` is a second flat-colour source.
- Whether the I8 texture `D_09000000_36ED60` (`assets/us/effects/gfx/D_09000000_36ED60.png`) has a solid region at the seam of the 128 texel tile. Not checked. The PNG was not read in this pass.
- Render order between the lightning effect and the shockwave effect (both `RENDER_MODE_SURFACE_XLU_LAYER3`). Not checked.
- The shot description says "top Shy Guy". The panel in frame 260 is at y 190-218, near the figure at about x 116, y 200. Confirm the target figure with the user before Test B2.
