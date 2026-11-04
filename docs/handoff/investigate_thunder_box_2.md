# Investigation: THUNDER-BOX yellow rectangle (second pass)

Scope: read only. No source, asset or commit change. Shots: build3ds/azt/shots/thunder_box/0001_0_0.ppm (frame 256), 0002_0_0.ppm (frame 260). PPM 240 x 400.

## Result
- The draw that writes the solid yellow fill is not identified with proof.
- Best candidate: the battle quad `D_09000950_3939F0` drawn by `src/effects/flashing_box_shockwave.c:183-184` (type `FX_SHOCK_OVERLAY_LIGHTNING_BATTLE`, `flashing_box_shockwave_appendGfx`). Confidence: low.
- Cause is not proven. No line is named as the writer.

## Shot evidence (measured from the PPM)
- Strict yellow (R>200, G>200, B<80): 312 px at frame 256, 232 px at frame 260. No solid yellow block exists. A filled rectangle would give far more pixels.
- Crop of frame 260, x 90-108, y 188-220 (scaled 5x, scratch PNG `crop260b.png`): a pale yellow panel with hard vertical edges sits left of the lower Shy Guy. Two stripes of pale yellow. The yellow star-shaped glow sits on the Shy Guy.
- Crop of frame 260, x 75-145, y 70-135: the top Shy Guy area shows a yellow starburst and a brown stage block with a green edge. No hard yellow rectangle is clearly seen behind the top Shy Guy in this crop.
- Conclusion: the pale yellow panel at x 90-108, y 188-220 is the hard-edged item in this frame. Its source is not identified.

## Draw code facts
- `src/effects/flashing_box_shockwave.c:63-65`: LIGHTNING_BATTLE gets PRIM (255, 255, 0), ENV (255, 255, 255).
- `src/effects/flashing_box_shockwave.c:115-118`: PRIM alpha `unk_18` stays 255 until `left < 5`.
- `src/effects/flashing_box_shockwave.c:112`: `unk_24` goes to 8.0. Quad scale = `unk_24 * 0.25` = 2.0. Quad half size 24 x 16 units.
- `src/effects/flashing_box_shockwave.c:178-185`: battle quad then `D_09000FF8_394098`.
- `assets/us/effects/gfx/D_09000950_3939F0.gfx.inc.c:15`: texture load `D_09000000_3930A0`, fmt I, 32 x 32, `G_TX_MIRROR|G_TX_WRAP`, masks 5, 5.
- `assets/us/effects/gfx/D_09000950_3939F0.gfx.inc.c:7`: combine `(PRIM - ENV) * TEXEL0 + ENV` for colour, alpha `TEXEL0 * PRIM`.
- Quad UVs `D_09000950_3B5C90` run 0 to 1024 (0 to 32 texels). This is one texture width, so the mask wrap does not repeat inside the quad.
- `assets/us/effects/gfx/D_09000FF8_394098.gfx.inc.c:5-6` (group 1, `G_RM_ZB_CLD_SURF`): alpha = TEXEL0 * PRIM.
- `assets/us/effects/gfx/D_09000FF8_394098.gfx.inc.c:35-36` (group 2, `G_RM_AA_ZB_XLU_SURF`): alpha = PRIMITIVE (constant, unk_18). Colour uses SHADE. No texel read in group 2.

## Leads
1. `pm_tex.c:1473` (shr wrap, mirror state): ruled out as the rectangle source. Wrap only affects texels past the image edge. The quad reads 0 to 32 texels in a 32 texel image, so the wrap does not run. Group 1 and 2 UVs reach about 60 texels, so wrap would repeat the bolt texture, not draw a solid block.
2. `pm_tex.c:261-270` (`pm_tex_set_wrap`): sets mask and wrap state only. Not a draw. Not the writer.
3. `pm_tex.c:318-371` (`expand`, mask pad): pads texels past the window with the mask rule. For the 32 texel texture no pad is needed. I4 (FMT_I, SIZ_4) returns at line 322 without expanding, so this path does not build the I4 texture. Not the writer for this shot.
4. Group 2 alpha (run 8, `D_09000FF8` line 36): group 2 alpha is the prim alpha, so the run 8 edit did not touch the quad. The rectangle stayed. Group 2 is ruled out as the rectangle source by run 8 (still visible after change).
5. Non-mesh draw in the same frame: `pm_gbi.c:3774-3807` (G_FILLRECT, 1-cycle fills use prim colour). The screen fade and choice-box fills use this path. Not verified in this frame. Open lead.

## Open items
- Tag the quad draw (`D_09000950_3939F0` at `flashing_box_shockwave.c:184`) and the fill path (`pm_gbi.c:3774-3807`) in a single diagnostic row, one quantity per shot. Name the value the draw uses (texel alpha, combine colour, rect size).
- Check the I4 load path (not `expand`) for the texture `D_09000000_3930A0`: whether texel alpha is zero outside the bolt in the GPU copy. The handoff texture check used the source PNG only.
