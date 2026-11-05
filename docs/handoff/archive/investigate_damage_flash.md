# Investigation: damage flash and lightning box (read only)

Scope: read only. No source edit, stage, commit or revert. Question: which code makes the damage flash for enemies and Mario, which code makes the Thunder Rage box, and what they share.

## Result

There are two different damage paths.

1. The normal hit flash is a palette swap on the sprite. It sets no GBI primitive colour, no blend, no draw list and no render mode.
2. The shock hit is a box. It calls the same overlay as Thunder Rage (`flashing_box_shockwave.c`). This box is pale yellow (PRIM 255,255,0). Its on/off gate is `time % 2`, which causes a per-frame flicker.

The user report of a pale yellow box flickering at the top Shy Guy matches path 2, not path 1. The palette flash is white, not yellow.

## (1) Normal damage flash (enemies and Mario)

- Trigger: `set_actor_flash_mode(target, 1)` at `src/battle/dmg_player.c:947` and `:1115`, `src/battle/dmg_partner.c:805` and `:964`, `src/battle/dmg_item.c:449`, `src/battle/1A5830.c:727` and `:883`.
- Setter: `set_part_flash_mode` at `src/battle/190B20.c:2780-2789` sets `flashMode = FLASH_MODE_LIGHT` and `flashStateChanged = true`.
- Frame timer: `part_flash_on` at `src/battle/actor_rendering.c:3495-3515`. Frames left: LIGHT 1, MEDIUM 8, HEAVY 14 (lines 3501, 3505, 3510).
- On/off toggle: `flashEnabled` set to `FLASH_PAL_ON` or `FLASH_PAL_OFF` on alternate frames, lines 3523-3585. This is the flicker source for the normal hit.
- Effect on pixels: `make_flash_palettes` at `src/battle/actor_rendering.c:2041-2063`. Each palette entry becomes `PAL_MEM(PAL_MEM(*src) | 0xFFFE)` (line 2056), which is white. The alpha bit is kept.
- Draw: `spr_draw_npc_sprite` with `DRAW_SPRITE_OVERRIDE_PALETTES` and `decorations->flashPalettes`, at `src/battle/actor_rendering.c:2127-2131`. Player sprite: `:2194-2197`.
- Sprite palette path: `src/sprite.c:991` and `:1203` swap `palettes = paletteList` when the flag is set.
- Not used: no `gDPSetPrimColor`, no `gDPSetEnvColor`, no render mode change, no new draw list in this path.

## (2) Lightning path (Thunder Rage)

- Entry: `src/effects/lightning.c:105-109`. If `arg0 >= 3`, the code calls `flashing_box_shockwave_main` with `FX_SHOCK_OVERLAY_LIGHTNING_BATTLE` (type 3) in battle, or `FX_SHOCK_OVERLAY_LIGHTNING_WORLD` (type 1) in the world.
- Bolt draw: `lightning.c:262` sets PRIM 255,47,198 (type 0-2 branch). `lightning.c:273` sets PRIM 255,255,0 (default branch). `lightning.c:279` calls `gSPDisplayList(dlist)`.
- Box draw: `src/effects/flashing_box_shockwave.c`.
  - Init colours: lines 60-66. Types 0, 1, 3 set primCol 255,255,0 and envCol 255,255,255.
  - Render task: lines 134-138, `RENDER_MODE_SURFACE_XLU_LAYER3` (line 136), dist 10.
  - Appendgfx: `flashing_box_shockwave_appendGfx` at line 141.
    - Line 159: `gDPSetPrimColor(..., primCol.r, primCol.g, primCol.b, unk_18)`.
    - Line 160: `gDPSetEnvColor`.
    - Lines 162-171: type 0 or type 1, gated by `time % 2`, draws `D_09000600_3936A0` and `D_090008E8_393988`.
    - Lines 173-186: type 1, type 2 or type 3. Line 175 sets PRIM 255,255,0 with `unk_14` for type 1. Lines 184-185 draw `D_09000950_3939F0` and `D_09000FF8_394098`. These are not gated by `time % 2`.
- Update: `flashing_box_shockwave_update` at lines 90-124. `unk_18 *= 0.8` at line 118 when `timeLeft < 5`.
- Battle hit (type 3) takes the not-gated branch. The world hit (type 1) takes the gated branch.

## (3) Shared and only-lightning state

Shared by the shock hit box (type 0) and the lightning box:
- The same function set: `flashing_box_shockwave_main`, `_render`, `_appendGfx` (`flashing_box_shockwave.c:21-70`, `:134-186`).
- The same render mode: `RENDER_MODE_SURFACE_XLU_LAYER3` (`flashing_box_shockwave.c:136`).
- The same PRIM colour 255,255,0 and ENV colour 255,255,255 (`flashing_box_shockwave.c:60-66`).
- The same PRIM write at line 159 (alpha `unk_18`).
- The same `gSPDisplayList` pair `D_09000600` and `D_090008E8` for types 0 and 1 (lines 162-171).

Only the lightning path (type 1, 2, 3):
- Types 1, 2, 3 draw `D_09000950_3939F0` and `D_09000FF8_394098` (lines 173-186).
- Type 3 is excluded from the `time % 2` gate. It draws every frame.
- Bolt triangles and bolt PRIM: `src/effects/lightning.c:262-279`.

Only the damage path:
- Palette override with white (`actor_rendering.c:2041-2063`).
- `flashEnabled` toggle (`actor_rendering.c:3523-3585`).

Shock hit callers (box type 0). These are the damage path that makes the yellow box:
- `apply_shock_effect` at `src/battle/popup_messages.c:2144-2164`. Calls `fx_flashing_box_shockwave(0, ...)` at line 2162 for each visible part with an idle animation.
- Callers: `src/battle/190B20.c:2428`, `src/battle/1A5830.c:325` and `:814`, `src/battle/dmg_item.c:103`, `src/battle/dmg_partner.c:142`, `:344`, `:358`, `:501`, `:903`, `src/battle/dmg_player.c:388`, `:652`, `:1044`, `src/battle/actor_api.c:3176`.
- Script path: `EVS_Enemy_ShockHit_Impl` at `src/battle/standard_events.c:1603` (`FX_SHOCK_OVERLAY_SHOCK_HIT`). Enemy scripts call it with `ExecWait(EVS_Enemy_ShockHit)`, for example `src/battle/area/arn/actor/tubba_blubba.c:215`.
- Port entry: `3ds/src/effect_loads.c:421-423` (`fx_flashing_box_shockwave`), and `3ds/src/pm_fxtest.c:100`.

## (4) Shared value

- The normal damage flash uses no PRIM or ENV value. It uses the palette mask `0xFFFE` only.
- The shared value between the damage flash and the lightning path is none in the box state.
- The only shared value between the damage path and the box is the box itself. The damage path that draws the box is the shock hit (type 0). It shares PRIM 255,255,0, `RENDER_MODE_SURFACE_XLU_LAYER3` and the `appendGfx` code with lightning.

## Most likely drawer of the pale yellow box

Most likely: `flashing_box_shockwave_appendGfx` (`src/effects/flashing_box_shockwave.c:141-186`). It sets PRIM 255,255,0 at line 159 and 175, and it draws the shared mesh. Earlier runs (handoff run 9 and 10) showed that the mesh `D_09000FF8_394098` and `D_09000950_3939F0` skip does not remove the rectangle. So the mesh is not the sole writer. The most likely cause is the PRIM state left set by line 159 or 175 and used by a later draw in the same frame. This is a hypothesis, not a result.

Confidence: low to medium for the PRIM leak. High that the box comes from `flashing_box_shockwave.c` and not from the palette flash.

## Confirm with one shot or log

1. Log one line per `flashing_box_shockwave_appendGfx` call with `type`, `time`, `primCol`, `unk_18`, and the frame number. Compare with the shot at the same frame. This shows whether the type 0 box and the type 3 box both run at the hit.
2. Use a shot pair at an odd and an even `time` for type 0 (shock hit, no lightning). If the box shows on odd `time` only, the `time % 2` gate at `flashing_box_shockwave.c:162` is the flicker source.
3. Log the draw calls after line 185 for the next draw with a PRIM set to 255,255,0 (one line per draw, frame 260). The first draw after the box that reads the stale PRIM is the drawer.

## Open items

- `dmg_player.c` calls `apply_shock_effect` at lines 388, 652, 1044. This report did not read those call sites in detail.
- The shot set `build3ds/azt/shots/thunder_box/` was not opened in this pass.
- Earlier handoffs (`docs/handoff/agent_THUNDER-BOX.md`, runs 9-14) tested several candidates. None removed the rectangle. Run 12 (sparkles skip) does not show a before shot, so it is not a clean result.
