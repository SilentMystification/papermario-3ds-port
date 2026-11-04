# Investigation: callers of flashing_box_shockwave and the D_09000950 / D_09000FF8 draws

Status: complete for callers. One correction to docs/handoff/investigate_thunder_box.md (see Correction). No source edited.

## Draw structure (code)
- `src/effects/flashing_box_shockwave.c:162`: types SHOCK_HIT (0) and LIGHTNING_WORLD (1) draw the ring list `D_09000600_3936A0` + `D_090008E8_393988` on odd frames only (`time % 2`), sized by `scaleX`, `scaleY`.
- `src/effects/flashing_box_shockwave.c:173-187`: types LIGHTNING_WORLD (1), MEGA_SHOCK (2), LIGHTNING_BATTLE (3) draw:
  1. `D_09000950_3939F0` (`assets/us/effects/gfx/D_09000950_3939F0.gfx.inc.c`). State only: texture I4 `D_09000000_3930A0`, combine, `gsDPSetRenderMode(G_RM_ZB_CLD_SURF, G_RM_ZB_CLD_SURF2)`, end list. It has no triangles.
  2. `D_09000FF8_394098` (`assets/us/effects/gfx/D_09000FF8_394098.gfx.inc.c`). This is the list that draws triangles.
  - Matrix: translate `(0, scaleY, 0)` times scale `(unk_24*0.25, unk_24*0.25, 1)`. `scaleX` is not used by this branch.
- Render modes inside `D_09000FF8_394098`:
  - Line 5: `G_RM_ZB_CLD_SURF`. Meshes `D_09000CF8_393D98`, `D_09000E18_393EB8`, `D_09000EB8_393F58`.
  - Line 35: `G_RM_AA_ZB_XLU_SURF`. Meshes `D_090009F8_393A98`, `D_09000B18_393BB8`, `D_09000BB8_393C58`.
  - Both modes set `ZMODE_XLU` (0x800). `reference/.../gbi.h:707-709, 826-827`. Neither sets `ZMODE_DEC` (0xc00).
- Bypass: `3ds/src/pm_gbi.c:678` gives `decal = (m & 0xc00) == 0xc00`, which is 0 for both modes. `3ds/src/pm_gpu.c:1107` gives `decal2 = 0`. The V16b stencil path (`pm_gpu.c:1108-1121`) does not run for any draw in this list.
- `D_09000950_3B5C90` (vertex quad, `assets/us/effects/gfx/D_09000950_3B5C90.vtx.inc.c`) is never referenced by a `gsSPVertex` in any list. Its only use is an `#include` in `src/effects/gfx/squirt.c:7`. It is not drawn.

## Correction to investigate_thunder_box.md
The prior report says the battle draw is only the quad `D_09000950_3B5C90` and that it is "the only XLU draw of this effect in battle". Both statements are wrong.
- The battle draw is the four mesh groups in `D_09000FF8_394098` (two `ZMODE_XLU` modes, two sets of meshes each), not a 24 x 16 quad.
- The shape of the visible box comes from those meshes. The meshes were not read in this pass, so their outline is not checked.
- The prior "quad may be the box" cause, and the proposed fix that uses the quad, depend on the wrong draw and need a new check.

## Callers that draw the D_09000950 / D_09000FF8 path (quad callers)
All five call sites reach the same display lists, so all use the same `ZMODE_XLU` render modes and bypass the decal path.

| # | file:line | effect / type | path to shockwave | size source |
|---|---|---|---|---|
| 1 | `src/battle/move/item/thunder_rage.c:34` (`N(SpawnLightningFX)`) | Thunder Rage, type 3 (battle) | `fx_lightning` -> `src/effects/lightning.c:109` | `scaleY` = target `(size.y - 2) * scalingFactor` (`thunder_rage.c:28`). Sets Y offset only. Quad scale from `unk_24`. `scaleX` unused. |
| 2 | `src/battle/move/item/thunder_bolt.c:31` | Thunder Bolt, type 3 (battle) | `fx_lightning` -> `src/effects/lightning.c:109` | Same as row 1 (`thunder_bolt.c:25`). |
| 3 | `src/world/area_kkj/kkj_13/intro_scenes.c:445` | Bowser lightning in world, type 1 (LIGHTNING_WORLD) | `PlayEffect(EFFECT_LIGHTNING, 4, ...)` -> `src/evt/fx_api.c:532` -> `lightning.c:109` | Literal 0, 0. Quad scale from `unk_24` (starts 0, interp to 8.0). |
| 4 | `src/battle/partner/watt.c:1317` | Watt mega shock, type 2 (MEGA_SHOCK) | `PlayEffect(EFFECT_FLASHING_BOX_SHOCKWAVE, ...)` -> `src/evt/fx_api.c:595` | Literal 0, 0. Quad scale from `unk_24` (starts 1.0, `*= 1.1` per frame for 21 frames, `flashing_box_shockwave.c:106-109`). Not from target size. |
| 5 | `src/battle/partner/watt.c:1320` | Same as row 4, second pulse 10 frames later | Same | Same as row 4. |

Row 1 and row 2 both sit inside the battle path, so the same `ZMODE_XLU` bypass applies.

## Callers that reach the effect but not the quad (SHOCK_HIT, type 0)
These call sites draw only the ring list (odd frames). They do not reach `flashing_box_shockwave.c:173`.

| file:line | caller | size source |
|---|---|---|
| `src/battle/standard_events.c:1603` (`EVS_Enemy_ShockHit_Impl`) | script PlayEffect, type 0 | `GetActorSize` of target (LVar4, LVar3) |
| `src/battle/area/omo2_5/actor/shy_squad_redux.c:724` (`N(EVS_MemberShockReaction)`) | script PlayEffect, type 0 | `GetActorSize` of target |
| `src/battle/common/actor/bob_omb.inc.c:580` | script PlayEffect, type 0 | `GetActorSize` of target |
| `src/battle/popup_messages.c:2162` (`apply_shock_effect`) | direct call, type 0 | `part->size` times `actor->scalingFactor` (`popup_messages.c:2155-2156`) |
| `apply_shock_effect` callers (13 call sites): `src/battle/190B20.c:2428`, `src/battle/1A5830.c:325`, `src/battle/1A5830.c:814`, `src/battle/actor_api.c:3176`, `src/battle/dmg_player.c:388`, `:652`, `:1044`, `src/battle/dmg_partner.c:142`, `:344`, `:358`, `:501`, `:903`, `src/battle/dmg_item.c:103` | damage and shock paths | Target part size (as above) |
| `3ds/src/pm_fxtest.c:100` | fxtest table, type 0 | Test value |

## Other references (not callers)
- `src/evt/fx_api.c:594-596`: dispatcher for `EFFECT_FLASHING_BOX_SHOCKWAVE`.
- `src/effects/lightning.c:31, 103-109`: lightning dispatcher. Types 3 or 4 (`arg0 >= 3`) call the shockwave. Context battle gives type 3, else type 1.
- `src/effects/lightning.c:60-64`: `unk_14` only (30 for `arg0 >= 3`).
- `src/effects/lightning.c` `arg0 == 1` or `2` (`hos_05/intro.c:2060, 2178`): no shockwave call.
- `3ds/src/effect_loads.c:414-417`, `3ds/src/rom_remap.h:119`, `3ds/stub_include/effect_ld_addrs.h:127-129, 463-464`: load and address tables.

## Answer to the orchestrator question
- Quad callers: 5 call sites in 4 functions (rows 1 to 5).
- All 5 use the same `ZMODE_XLU` display lists. None goes through the decal path.
- Size source: for the battle quad the size comes from `unk_24` (scale). The target size only moves the quad in Y (`scaleY`). For rows 3 to 5 the sizes are literal 0.

## Open items
- Read the vertices and colours of `D_09000CF8_393D98`, `D_090009F8_393A98` and their group partners to name the outline the user sees.
- The prior test plan (tag draws in `FX_SHOCK_OVERLAY_LIGHTNING_BATTLE`) still applies. Tag the `D_09000FF8` draw groups, not the unused quad.
