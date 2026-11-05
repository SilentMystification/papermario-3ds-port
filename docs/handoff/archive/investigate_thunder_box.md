# Investigation: Thunder Rage box around Shy Guy (attract loop, battle)

Status: cause partly named. The draw that makes the box is not proven. No shot run.

## Symptom
Thunder Rage lightning on a Shy Guy draws a flickering box around the enemy. Expected: the flicker follows the Shy Guy texture outline.

## Call chain (code)
1. `src/battle/move/item/thunder_rage.c:27-34`: `scaleX = (size.x + (size.x >> 2)) * scalingFactor`, `scaleY = (size.y - 2) * scalingFactor`, then `fx_lightning(rand_int(2) + 3, ...)`.
2. `src/effects/lightning.c:60-64`: arg0 3 or 4, so `unk_14 = 30`.
3. `src/effects/lightning.c:103-109`: arg0 >= 3 calls `flashing_box_shockwave_main` with `FX_SHOCK_OVERLAY_LIGHTNING_BATTLE` (= 3, `include/effects.h:1277`) in battle context (`CONTEXT_BATTLE` = 1).
4. `src/effects/flashing_box_shockwave.c:56-57`: `scaleX = sizeX * 0.5`, `scaleY = sizeY * 0.5`.
5. `src/effects/flashing_box_shockwave.c:173-186`: the battle type draws only the quad `D_09000950_3939F0` (lines 183-185). Its matrix is `scale(unk_24*0.25, unk_24*0.25, 1) * translate(0, scaleY, 0)`. The battle path does not use `scaleX`.
6. `flashing_box_shockwave.c:112`: `unk_24 += (8.0 - unk_24) * 0.05`. So `unk_24` goes to 8.0 and the scale goes to 8.0 * 0.25 = 2.0 on each axis. The quad then has half-size 24 x 16 units (`assets/us/effects/gfx/D_09000950_3B5C90.vtx.inc.c`, vertices +-12, +-8).
7. `flashing_box_shockwave.c:136`: `RENDER_MODE_SURFACE_XLU_LAYER3`.

## Render state of the quad (cause candidate)
- `assets/us/effects/gfx/D_09000950_3939F0.gfx.inc.c`: `gsDPSetRenderMode(G_RM_ZB_CLD_SURF, G_RM_ZB_CLD_SURF2)`, `G_CYC_1CYCLE`.
- `G_RM_ZB_CLD_SURF` = `Z_CMP | IM_RD | CVG_DST_SAVE | FORCE_BL | ZMODE_XLU | GBL_c1(CLR_IN, A_IN, CLR_MEM, 1MA)` (`reference/paperboat/external/libultraship/include/libultraship/libultra/gbi.h:826-827`). ZMODE_XLU = 0x800 (`gbi.h:677`). No Z_UPD.
- `3ds/src/pm_gbi.c:678`: `decal = (m & 0xc00u) == 0xc00u`. For this quad `m & 0xc00 = 0x800`, so `decal = 0`.
- `3ds/src/pm_gpu.c:1107`: `decal2 = z && r_zcmp && r_decal && r_blend && !r_zmark && !r_nocol && draw_3d`. `r_decal = 0`, so the V16b two-pass stencil path (`pm_gpu.c:1108-1121`) never runs for this quad. The quad uses one plain XLU draw.
- `3ds/src/pm_gpu.c:446-447` (V16b comment) says the N64 overlay is `Z_CMP, ZMODE_DEC`. That is wrong for this quad. The V16b fix targets decals only, so it does not cover this draw.

## Evidence
- Texture `assets/us/effects/gfx/D_09000000_3930A0.png` (I4, 32 x 32): a diagonal bolt. Corners and most edges are 0. Row 0 has non-zero texels at x = 2 to 10 (values 1 to 10 of 15). I4 decodes to L = A = nibble (`3ds/src/pm_tex_tx1.h:119-122`). So texels alone give alpha 0 outside the bolt. A full rectangle from texel alpha is not expected. Only a thin top edge is possible.
- Combine (`D_09000950` gfx): colour = (PRIM - ENV) * TEXEL0 + ENV, alpha = TEXEL0 * PRIM alpha. PRIM alpha = `unk_18` = 255 until `left < 5` (`flashing_box_shockwave.c:117-118`). Outside the bolt the alpha is 0.
- `flashing_box_shockwave.c:162`: the `time % 2` toggle applies only to `SHOCK_HIT` and `LIGHTNING_WORLD`, not to the battle type. So the battle quad does not toggle with the frame parity. The flicker source is not in this toggle.
- The quad is sized by `unk_24`, not by the enemy. Its size is 48 x 32 units for any enemy, so a box larger than a small Shy Guy is expected by design.

## Ruled out
- Stencil decal path (`pm_gpu.c:1107-1121`): not entered for this quad (`decal = 0`).
- I4 decode (`pm_tex.c:289-290`, `pm_tex_tx1.h:119-122`): gives L = A = nibble. Correct.
- Combine alpha: alpha is TEXEL0 * PRIM. Zero texel gives zero alpha.
- Frame-parity toggle (`flashing_box_shockwave.c:162`): battle type not affected.
- Vertex colour: `D_09000950_3B5C90` colours are (0,0,0,255). Cycle 1 alpha does not use SHADE.

## Open question
Which draw forms the visible box. The quad is the only XLU draw of this effect in battle. Its visible area looks like the bolt, not a box, per the texture. So the box may come from another draw (shadow, darkness fill from `FadeBackgroundDarken`, or the quad's top edge with bilerp). This is not proven by code.

## Minimal fix proposal (code agent, after the question is settled)
- If the quad is the box: size the battle quad from the actor, `data->scaleX` / `data->scaleY` (`flashing_box_shockwave.c:178-179`), instead of `unk_24` alone. Set the quad to write only where the bolt texel is set (alpha test on the texel, `atest` threshold), so no rim alpha is left.
- If the quad is not the box: no source change yet. Name the draw first.
- Do not treat `pm_gpu.c:446-447` as the cause. Its comment describes ZMODE_DEC, which this draw does not use.

## What would settle it
1. A diagnostic row that tags the draws of `FX_SHOCK_OVERLAY_LIGHTNING_BATTLE` (one colour per draw, or a log line with the draw index and the `rect`/`first`/`count` values) and one shot in a Shy Guy battle with Thunder Rage. The catalog (`docs/handoff/test_catalog.md`) has no Thunder Rage scenario. `battle_start` only reaches the battle HUD. A new scenario needs the user's approval.
2. Console check: the user confirms whether the box is white (env colour) or dark, and whether it has a rim at the texture top edge.
