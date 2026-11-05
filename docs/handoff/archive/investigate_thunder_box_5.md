# Investigation: THUNDER-BOX yellow rectangle (fifth pass)

Scope: read only. No source, asset or commit change. Shot: build3ds/azt/shots/thunder_box/0002_0_0.ppm (frame 260). Prior reports read: investigate_thunder_box_3.md, investigate_thunder_box_4.md.

## Result
- Best candidate: bolt draw `src/effects/lightning.c:279` `gSPDisplayList(gMainGfxPos++, dlist)`, with the tile 1 size at `src/effects/lightning.c:278` as the likely cause of a flat region. Confidence: low.
- No candidate draws a flat-colour rectangle in this frame. Each candidate below is checked against the code. None is proven by a shot.

## Candidate table

| # | Draw | file:line | Command | Value used | Fits top-left pale yellow rectangle? |
|---|---|---|---|---|---|
| 1 | Bolt triangles | `src/effects/lightning.c:279` | gSPDisplayList(dlist) | `D_E006CB60[3]` = `D_09003378_3720D8` (8 quads, 32 verts, UV up to about 469 texels) | Yes, best fit. Textured triangles, not a flat fill. Yellow comes from PRIM (255,255,0). |
| 2 | Tile 1 size (seam) | `src/effects/lightning.c:278` | gDPSetTileSize(1, uls2, ult2, uls2+1020, ult2+124) | Tile 1 spans 1020/4 = 255 texels. Texture is 128 texels wide (`D_09000000_36ED60`). Mirror and wrap repeat the bolt inside tile 1. | Partly. It can repeat bolt texels across a wide area, but it sets no colour. It only changes which texels show. |
| 3 | Bolt PRIM | `src/effects/lightning.c:273` | gDPSetPrimColor(0,0,255,255,0,unk_10) | RGB (255,255,0), alpha `unk_10` (255 from start, -13 per frame from frame 7, `lightning.c:191-196`) | Colour fits yellow. Not a rectangle by itself. |
| 4 | Bolt state | `src/effects/lightning.c:241` | gSPDisplayList(dlist2) | `D_E006CB78[3]` = `D_09003948_3726A8`. 2-cycle. No triangles. No fill. | No. State only. |
| 5 | Reflected bolt copy | `src/effects/lightning.c:216-217` | renderTask flag RENDER_TASK_FLAG_REFLECT_FLOOR | Same appendGfx as candidate 1, drawn mirrored under the floor. | Unlikely. A reflection places the copy below the floor, not top left. Not checked in a shot. |
| 6 | Squirt strip | `src/effects/squirt.c:250` and `:258-260` | gSPDisplayList(D_09000800_3B5B40 or D_090008A8_3B5BE8) plus gSP2Triangles strip | PRIM (210,230,255) for type 0, env (unk_44, unk_48, unk_4C) | No. Colour is pale blue, not yellow. Strip, not a rectangle. Squirt is not in the battle path (not checked). |
| 7 | Squirt quad list | `src/effects/squirt.c:8` | `#include "effects/gfx/D_09000990_3B5CD0.gfx.inc.c"` | Not used by `squirt_appendGfx`. Only `D_09000800_3B5B40` and `D_090008A8_3B5BE8` are drawn at `squirt.c:250`. | No. Earlier claim (investigate pass 4) that this quad is drawn by squirt is wrong. The include only adds the data. |
| 8 | Fill path | `3ds/src/pm_gbi.c:3774-3807` | G_FILLRECT | Fill colour = `fill` (cycle 3) or `prim` (cycle 1/2). Full rectangle from lrx, lry, ulx, uly. | Only if a FILLRECT command runs in this frame. The bolt state list (candidate 4) has no FILLRECT. Lightning cannot be the source. The fill rects in battle are full viewport (`src/battle/16C8E0.c:849`, `:856`). `src/draw_box.c:775-777` is a side buffer. |

## Texture check
- `assets/us/effects/gfx/D_09000000_36ED60.png` (I8, 128 texels wide) was read. It shows a bolt on black. No solid bright block exists. The bright area is only the bolt streaks.
- So the texture cannot supply a flat rectangle by itself. A flat block needs a wrapped or mirrored coordinate range that repeats the bolt edge across a wide area. Candidate 2 (tile 1 span of 255 texels) gives that range.

## Best fit
- Candidate 1 (`lightning.c:279`) is the only draw that paints textured geometry from the top-left region in the battle path. Its colour fits yellow. Its UV range (about 469 texels) is wider than the 128 texel texture, so mirror and wrap repeat the bolt. Repeated texels can read as a hard-edged block.
- Candidate 2 (`lightning.c:278`) is the likely reason for the hard edge. It is not a separate draw. It changes candidate 1.

## Shot that would confirm
- Test B2 (already defined in investigate_thunder_box_4.md). Skip `gSPDisplayList(dlist)` at `src/effects/lightning.c:279` for `unk_00 >= 3`. Shot at frame 260. If the panel goes, candidate 1 paints it.
- If B2 keeps the panel, test the tile 1 span: at `src/effects/lightning.c:278`, change `uls2 + 1020` to `uls2 + 508` (127 texels). One quantity. Shot at frame 260. If the panel width or edge changes, candidate 2 is the cause.
- Each test uses one start. Revert after a shot that does not change.

## Open items
- Whether the reflected copy (candidate 5) runs for this battle. Not checked.
- Whether squirt (candidate 6) runs in this attract battle. Not checked.
- Confirm the target figure is the top Shy Guy with the user before Test B2.
