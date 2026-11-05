# Plan: THUNDER-BOX (shock hit follows the sprite outline)

## Root cause
The shock hit (type 0, `FX_SHOCK_OVERLAY_SHOCK_HIT`) draws a quad at the part position (`src/effects/flashing_box_shockwave.c:162-171`). A quad cannot follow the sprite outline. The normal hit flash already recolours the sprite through its palette (`make_flash_palettes`, `src/battle/actor_rendering.c:2041-2063`). That path follows the outline, and Mario and enemies both use it (NPC draw at `:2127-2129`, player draw at `:2194-2197`, both through `update_part_flash` at `:3472`).

Fix: the shock hit sets a per-part counter. The palette flash path shows a yellow palette on odd counter values for 30 frames. This is the same length and the same odd-frame gate as the old box (`timeLeft = 30`, `time % 2`). Then the type 0 quad and all of its spawners are deleted.

## Changes (no toggle, no stub)
1. `include/common_structs.h:1746` changes `char unk_769[3];` to `s8 shockFramesLeft; char unk_76A[2];`. The size and offsets do not change.
2. `src/battle/popup_messages.c:2144-2164` `apply_shock_effect`: replace the body. For each part that passes the current guards (not INVISIBLE, `idleAnimations != nullptr`, not SKIP_SHOCK_EFFECT), and also `decorationTable != nullptr` and not `ACTOR_PART_FLAG_NO_DECORATIONS`, set `part->decorationTable->shockFramesLeft = 30`. Delete the x/y/z/f1/f2 code and the `fx_flashing_box_shockwave(0, ...)` call (`:2152-2162`).
3. `src/battle/actor_rendering.c:3472-3484` `update_part_flash`: after the switch, if `shockFramesLeft > 0`, then decrease it when `!isReflection`, and set `flashEnabled = FLASH_PAL_ON` when the value is odd. Add one debug line when the value is odd and `!isReflection`: `pm_log_d("debug shock tint on: frame %u left %d", pm_frame_count(), left)` (debug tool, it may stay).
   Note: the counter decreases per rendered frame, not per game frame. The first rendered frame after the hit has the value 29 (odd), so the tint is on.
4. `src/battle/actor_rendering.c:2056` `make_flash_palettes`: when `decorations->shockFramesLeft > 0`, write `PAL_MEM((PAL_MEM(*src) & 1) | 0xFFC0)` (RGBA5551 yellow R31 G31 B0, the alpha bit is kept). Otherwise keep the white line. The shock uses its own field, so the white flash from `set_actor_flash_mode(target, 1)` (for example `dmg_item.c:449`, after `dmg_item.c:103`) does not cancel it.
5. Delete the old path:
   - `src/effects/flashing_box_shockwave.c:162`: the condition becomes `type == FX_SHOCK_OVERLAY_LIGHTNING_WORLD && time % 2`. Delete `case FX_SHOCK_OVERLAY_SHOCK_HIT:` at `:60`.
   - `src/battle/standard_events.c:1597-1603` (`EVS_Enemy_ShockHit_Impl`): replace GetActorPos, GetActorSize, SetF, DivF, AddF and PlayEffect with `Call(ShowShockEffect, ACTOR_SELF)`. Delete the 13 `AddEffectOffset, LVarF` lines at `:1609-1646`. They move the deleted box. The `AddActorPos` shake stays.
   - `src/battle/common/actor/bob_omb.inc.c:578-580`: replace GetActorPos, GetActorSize and PlayEffect with `Call(ShowShockEffect, ACTOR_SELF)`. Before the edit, read the lines after `:580` and confirm that LVar0-LVar4 are not read there.
   - `src/battle/area/omo2_5/actor/shy_squad_redux.c:722-724`: replace GetPartOffset, GetActorSize and PlayEffect with `Call(ShowShockEffect, ACTOR_SELF)`. Keep LVar0 (part index), because `:725-726` read it.
   - `3ds/src/pm_fxtest.c:100`: change the type from 0 to 1 so that the fx test still draws something.
   After the change, `grep -rn "FX_SHOCK_OVERLAY_SHOCK_HIT\|fx_flashing_box_shockwave(0" src 3ds/src` must find only the enum at `include/effects.h:1274`.

Lines changed: about 40. Files: 8.

## Scenes to check
- `thunder_box` only. It uses `apply_shock_effect` (from `dmg_item.c:103`) and the palette path on the Shy Guys.
- No byte-equal check. The change does not touch pm_gbi.c, pm_gpu.c, pm_tex.c or src/model.c. The normal flash keeps the white line when `shockFramesLeft == 0`.
- Mario uses the same functions (`update_part_flash` at `:1907`, the player draw at `:2194`). The catalog has no Mario shock scenario. Mario is covered by the code path only. If the user wants a Mario shot, add a scenario first (Mario jumps on an electrified enemy, `dmg_player.c:1044`).

## The proof shot
- Scenario `thunder_box`, unchanged debug args (`shotwin=252:12`). Change only the until and the expected event in the same commit as the catalog row:
  - until: `debug shock tint on: frame 25[2-9]|debug shock tint on: frame 260`
  - expected event: `debug shock tint on`
- Expected frame: 252 (run 16: type 0 drew first at 252, the first shot frame after the hit). Use the frame that the until-line names. The shot of that frame is the proof.
- Pass: the top Shy Guy shows yellow inside its own outline. No hard-edged rectangle is around it. The quantity is the yellow pixels outside the Shy Guy outline. Run 17 showed this area without the box.
- Fail: no tint line in the log, or a rectangle in the shot. Then revert all 8 files.
- Out of scope: the yellow hit burst stays. It is a different effect (run 17).

## Risk
- shy_squad_redux: `ShowShockEffect` tints all squad member parts, not only the hit member. The old box was per member.
- The counter is driven by rendered frames. At a low frame rate the tint lasts longer than 30 game frames. Frame skip on O3DS can change the flicker phase.
- Parts with `ACTOR_PART_FLAG_NO_DECORATIONS` now show no shock effect. Before the change they showed the box.
