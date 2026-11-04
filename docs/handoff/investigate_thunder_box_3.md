# Investigation: THUNDER-BOX yellow rectangle (third pass)

Scope: read only. No source, asset or commit change. Shots: build3ds/azt/shots/thunder_box/0001_0_0.ppm (frame 256), 0002_0_0.ppm (frame 260).

## Result
- The stale-PRIM lead is wrong. PRIM is set every frame in `flashing_box_shockwave_appendGfx`, so it is not stale.
- The writer is not proven. Best candidate: the lightning bolt draw, not the shockwave mesh. Confidence: low.
- Ranked candidates:
  1. Lightning bolt draw (`src/effects/lightning.c:241-279`): yellow PRIM, 2-cycle texture state, bolt triangles. Carries the same yellow PRIM and an alpha that stays 255 for about 26 frames.
  2. Shockwave mesh group 1 and 2 (`assets/us/effects/gfx/D_09000FF8_394098.gfx.inc.c:5-36`). Not yet skipped as a whole.
  3. Fill path `3ds/src/pm_gbi.c:3774-3807`. Not checked in this frame.

## PRIM lead (refuted)
- `src/effects/flashing_box_shockwave.c:63-65` sets `primCol` to (255, 255, 0) for `FX_SHOCK_OVERLAY_LIGHTNING_BATTLE`. This runs at effect creation.
- `src/effects/flashing_box_shockwave.c:159` calls `gDPSetPrimColor(..., primCol.r, primCol.g, primCol.b, unk_18)` on every frame, before the battle branch. So the battle branch at lines 176-178 does not need to set PRIM. Lines 176-178 only set PRIM for `LIGHTNING_WORLD`.
- `unk_18` stays 255 until `left < 5` (`flashing_box_shockwave.c:117-119`).
- Conclusion: PRIM for the battle quad and the mesh is (255, 255, 0, 255) for most of the effect life. It is not stale.

## Shockwave mesh (group 1 and 2) math
- Group 1 (`D_09000FF8...:5-6`, render mode `G_RM_ZB_CLD_SURF`): colour = (PRIM - ENV) * SHADE + ENV. Alpha = TEXEL0 * PRIM. Texture is I4 `D_09000000_3930A0`, set by `D_09000950_3939F0.gfx.inc.c:15`.
- Group 2 (`D_09000FF8...:35-36`, render mode `G_RM_AA_ZB_XLU_SURF`): colour = (PRIM - ENV) * SHADE + ENV. Alpha = PRIM alpha (flat, 255 for most frames). No texel read.
- Vertex colours in `D_09000CF8_393D98` and `D_090009F8_393A98` are only 255 (white) or 0 (black). Colour therefore gives yellow (255, 255, 0) where the vertex is white and white (255, 255, 255) where it is black.
- Vertex positions of both meshes form bolt-shaped fan strips about 320 units from the origin. They do not form a straight-edged rectangle. A hard rectangle needs a rectangle triangle pair or a fill. Neither exists in these meshes.
- Run 8 (group 2 alpha to TEXEL0) did not remove the rectangle. Group 2 alpha is therefore not the only source. Run 9 (skip D_09000950) added yellow blocks at top and bottom and did not remove the top-left rectangle.

## Lightning bolt draw (new candidate)
- `src/effects/lightning.c:60-64`: `unk_14` 30 for `arg0 >= 3`.
- `src/effects/lightning.c:103-109`: `arg0 >= 3` also creates the shockwave. Battle context gives `FX_SHOCK_OVERLAY_LIGHTNING_BATTLE`.
- `src/battle/move/item/thunder_rage.c:34`: `fx_lightning(rand_int(2) + 3, ...)`, so `arg0` is 3 or 4.
- `src/effects/lightning.c:224-225`: `dlist` = `D_E006CB60[unk_00]`, `dlist2` = `D_E006CB78[unk_00]`. For `unk_00` 3 the lists are `D_09003378_3720D8` (bolt) and `D_09003948_3726A8` (state). For `unk_00` 4 the bolt is `D_090032F8_372058`, and the state is the same `D_09003948_3726A8`.
- `src/effects/lightning.c:262` (not used, only `unk_00 < 3`) and `:273`: `gDPSetPrimColor(255, 255, 0, unk_10)` for `unk_00 >= 3`. `unk_10` starts 255 and drops by 13 per frame from frame 7 (`lightning.c:191-196`).
- `src/effects/lightning.c:241`: `gSPDisplayList(dlist2)` (state). Line 279: `gSPDisplayList(dlist)` (bolt triangles).
- `assets/us/effects/gfx/D_09003948_3726A8.gfx.inc.c:5-16`: 2-cycle, texture I8 `D_09000000_36ED60` 128 x 128, mirror and wrap, mask 7 and 5. Combine uses TEXEL0, TEXEL1, ENVIRONMENT (alpha 0x80 at line 13), PRIMITIVE. No triangles in this list.
- `assets/us/effects/gfx/D_09003378_3720D8.gfx.inc.c:7-15`: bolt triangles from `D_09002BC0_371920` (32 vertices, all white). UV values reach about 15033 (469 texels, mirror wrap in a 128 texel tile).
- `src/effects/lightning.c:217`: `RENDER_TASK_FLAG_REFLECT_FLOOR` is set. The bolt may also draw a reflected copy. Not checked.

## Other notes
- `assets/us/effects/gfx/D_09000950_3B5C90.vtx.inc.c` is not referenced by any `gsSPVertex` in a drawn list. The battle quad is not drawn. (Consistent with the prior report.)
- Prior note in `docs/handoff/investigate_thunder_box_2.md` says the mesh outline is not read. This pass read the vertices. The outline is bolt-shaped, not rectangular.
- The yellow blocks at top and bottom (run 9) show that the state from `D_09000950_3939F0` affects some draw. The top-left rectangle did not change with that skip. Two different writers are likely.

## What settles it
- Test A (one quantity): in `src/effects/flashing_box_shockwave.c:185`, skip `D_09000FF8_394098` for the battle type (keep line 184). Shot at frame 260. If the top-left rectangle is gone, the shockwave mesh is the writer. If it stays, go to Test B.
- Test B (one quantity): in `src/effects/lightning.c:279`, skip `gSPDisplayList(dlist)` for `unk_00 >= 3`. Shot at frame 260. If the rectangle is gone, the bolt triangles are the writer. If it stays, the writer is not in either effect. Check the fill path in `3ds/src/pm_gbi.c:3774-3807` and the reflected bolt copy (`RENDER_TASK_FLAG_REFLECT_FLOOR`).
- Each test uses one start. Revert the edit before the next test if the shot does not change.
- Name the value the draw uses in the next diagnosis: the RGBA of the PRIM, the texel alpha, or the vertex colour.

## Open items
- Whether the lightning bolt draw runs twice (reflected copy from `RENDER_TASK_FLAG_REFLECT_FLOOR`).
- Whether the I8 texture `D_09000000_36ED60` has a solid region that the mirror and TEXEL1 tile can expose as a rectangle. Not checked.
- Render order between the lightning effect and the shockwave effect (both `RENDER_MODE_SURFACE_XLU_LAYER3`). Not checked.
