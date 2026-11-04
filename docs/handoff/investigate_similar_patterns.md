# Investigation: other draws with the Thunder Rage box pattern

Status: static search only. No shot run. No source file changed.
Known case: `docs/handoff/investigate_thunder_box.md`.

## Pattern under test
A Z-compared, non-decal XLU quad (ZMODE_XLU, no Z_UPD) drawn over or at the depth of a sprite or actor. The V16b stencil path does not run for it (`3ds/src/pm_gbi.c:678` sets `decal` only for ZMODE_DEC, `0xc00`; `3ds/src/pm_gpu.c:1107` needs `r_decal`).

## Gate of the V16b stencil path (for reference)
`3ds/src/pm_gpu.c:1107`: `decal2 = z && r_zcmp && r_decal && r_blend && !r_zmark && !r_nocol && draw_3d`.
The path is skipped when any of these is false: depth off, decal off, blend off, case 6 depth pass (`r_zmark`), no-colour write (`r_nocol`), or 2D draw (`draw_3d`).
Blend is computed at `3ds/src/pm_gbi.c:659-660`. Only the XLU decal modes set `r_blend`.
Draw call sites: `3ds/src/pm_gpu.c:1076` (C3D_DrawElements, raw path), `:1113`/`:1120` (C3D_DrawArrays, V16b path), `:1134` (B3b pass 2). Only `:1113`/`:1120` can run the stencil. The raw path at `:1076` is taken for decals with blend off (`pm_gpu.c:2098-2099` return 0 only for decal and blend both set).

## Hits

### HIGH
1. `src/effects/flashing_box_shockwave.c:173-186` (LIGHTNING_WORLD branch). Draws the same quad `D_09000950_3939F0` as the battle branch, with PRIM alpha `unk_14` (30 or 100, `src/effects/lightning.c:60-64`). Sized by `unk_24`, not by the target. Same render state (`G_RM_ZB_CLD_SURF`, no decal). Same box class in the world. Can show the same box: high.
2. `src/effects/flashing_box_shockwave.c:173-186` (MEGA_SHOCK branch), called from `src/battle/partner/watt.c:1317` and `:1320`. Same quad, same `unk_24` sizing. No target bounds. Can show the same box: high.
3. Opaque decals skip the V16b path. `G_RM_AA_ZB_OPA_DECAL` and `G_RM_ZB_OPA_DECAL` have cycle 1 `GBL_c1(CLR_IN, A_IN, CLR_MEM, A_MEM)` (`reference/paperboat/external/libultraship/include/libultraship/libultra/gbi.h:711-714, 820-821`). Bit 18 is set, so `pm_gbi.c:659-660` gives `blend = 0`. Then `decal2` is false (`pm_gpu.c:1107`) and the draw uses GEQUAL with bias (`pm_gpu.c:747, 1013`). That is the bleed the V16b comment describes (`pm_gpu.c:445-447`). Opaque decal users: `src/model.c:661-664` (IDX_02), `:735-737` (IDX_03), `:894` (IDX_12), `:961` (IDX_13), `:1084` (IDX_21), `:1156` (IDX_22). Shows a box: medium for the code path, high for the skip. The visible box needs an opaque decal on a sprite. The user model data that selects these modes is not traced.

### MEDIUM
4. `src/sprite.c:292` and `:307` (translucent sprite shading, alpha != 255). A 4-vertex quad sized to the sprite (`src/sprite.c:150-175`, `spr_make_quad_for_size`) drawn with `G_RM_ZB_CLD_SURF`. Same render state as the thunder quad. The quad is at the sprite bounds, so the box follows the sprite size. Shows the same box if the combine (`PM_CC_0A`, `PM_CC_3D`) ignores texel alpha. Confidence medium.
5. `src/imgfx.c:1653` and `:1683`. A 4-vertex quad per part (`imgfx_vtxBuf[i]`) with `G_RM_ZB_XLU_SURF`, no decal. Can show a rectangle around a sprite if the caller places it there. Caller not traced. Confidence medium.

### LOW
6. `src/sprite.c:72` and `:90`, `:264`. Same ZB_CLD state for translucent sprite DLs. Shows a box only if the colour ignores texel alpha. Low.
7. `src/effects/star_outline.c:224`, `:244` (ZB_CLD) and `:252` (AA_ZB_XLU). Outline is a 20-vertex star mesh (`assets/us/effects/gfx/D_09000B20_414AC0.gfx.inc.c`), scaled by `unk_38`/`unk_54`, not by the target. Shape is a star, not a box. Low.
8. `src/effects/stars_shimmer.c:318`. ZB_CLD for state 3 only. Particle, not a box around a target. Low.
9. `src/effects/radiating_energy_orb.c:176-217`. Per-part quads (one triangle pair each, `D_09001490_3A21F0` onward) with ZB_CLD, scaled by `unk_2C` and `scale`. Radiates from the orb centre. Low.
10. `src/effects/gather_magic.c:184`, DL `D_090001A8_38F8B8` and `D_090001C8_38F8D8`. Small per-particle quads scaled by `scale`. Low.
11. `src/battle/16C8E0.c:932`, `:941` (alpha 0, invisible), `:947`, `:948` (opaque HP bar fills at enemy screen position). Screen-space fill rects via `btl_draw_prim_quad` (`src/battle/190B20.c:3016-3040`). Not depth-tested. Positioned around the enemy but cannot flicker by depth. Low.
12. `src/battle/btl_states_menus.c:969`, `:982`. Alpha 0 fills around wheel options. No visible colour. Low.
13. `src/effects/energy_shockwave.c:164` (scale by `unk_1C`). Ring mesh in DL `D_09000DC0_3762D0`, render state in the DL. Not a box. Low.
14. `src/model.c:795-799` (IDX_09, ZMODE_XLU with Z_CMP and Z_UPD) and `:837-856` (IDX_0B). Model meshes, not quads. Low.

## Checked and not a hit
- `src/effects/flashing_box_shockwave.c:162-171` (SHOCK_HIT and LIGHTNING_WORLD frame mesh, `D_09000600_3936A0`): render mode `G_RM_ZB_OVL_SURF`, not XLU. Sized by `scaleX`/`scaleY` (target bounds). Not the XLU quad.
- `src/effects/flashing_box_shockwave.c:184-185` (`D_09000FF8_394098`): the bolt mesh after the quad. ZB_CLD and AA_ZB_XLU, but a bolt-shaped mesh, not a box. Same effect, low for the box.
- `src/imgfx.c:1862` (`G_RM_ZB_XLU_DECAL`): a decal, so it runs the V16b path.
- `src/draw_box.c` and `src/battle/16C8E0.c:1056`: 2D windows with no Z. Not depth-tested.
- `src/effects/damage_indicator.c:222`, `src/effects/disable_x.c:250`: `G_RM_CLD_SURF`, no Z_CMP. Not depth-tested.
- `src/battle/190B20.c:3024` and `src/battle/btl_states_menus.c` screen fills: no Z_CMP.

## Scope limits
- `assets/us/effects/gfx` has 102 lines with ZB_CLD, ZB_XLU or AA_ZB_XLU render modes. Only the DLs named above were traced to a draw. The rest were not traced.
- Render modes of the entity model data (`RENDER_MODE_DECAL_OPA` and similar) are set in data files. Not traced in this pass.

## Suggested next step (not a source change)
- Hit 3 is the same class as the V16b fix, and the fix covers XLU decals only. A test of an opaque decal on a sprite would confirm it. The code agent should not edit until a shot shows the bleed.
- Hits 1 and 2 need the same decision as the Thunder Rage case: is the quad the box.
