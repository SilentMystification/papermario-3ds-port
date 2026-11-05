# Trace: shock-hit yellow box (type 0)

Scope: read-only trace. No source edited.

## 1. Setup (apply_shock_effect)

- `src/battle/popup_messages.c:2144-2166`: loops over the actor parts.
  - Skips invisible parts, parts without idleAnimations, and parts with ACTOR_PART_FLAG_SKIP_SHOCK_EFFECT (2148-2150).
  - Position: x = curPos.x (2152), y = curPos.y + size.y/10 (2153), z = curPos.z (2154). Y minus size.y/2 if ACTOR_FLAG_HALF_HEIGHT (2158-2159).
  - Size: f1 = (size.x + size.x/4) * scalingFactor (2155), f2 = (size.y - 2) * scalingFactor (2156).
  - Call: `fx_flashing_box_shockwave(0, x, y, z, f1, f2)` (2162). Type 0 = FX_SHOCK_OVERLAY_SHOCK_HIT (`include/effects.h:1274`).
- `src/effects/flashing_box_shockwave.c:21-81` (init):
  - type = 0 (39). timeLeft = 30 (40). lifetime = 0 (41).
  - unk_10, unk_14, unk_18 = 255 (43-45). unk_24 = 0.0 (54, type is not MEGA).
  - scaleX = sizeX * 0.5 = f1/2 (56). scaleY = sizeY * 0.5 = f2/2 (57).
  - primCol = (255, 255, 0) (63-65). envCol = (255, 255, 255) (66-68).
- Wrapper: `src/effect_loads.c:421-423` passes args through.

## 2. Per frame

- Update (`flashing_box_shockwave.c:86-127`):
  - timeLeft-- while timeLeft < 100 (92-93). lifetime++ every update (96).
  - Remove when timeLeft < 0 (98-99). Lifetime is 0..30, so the effect lives 31 updates.
  - unk_14 *= 0.9 (115). unk_18 *= 0.8 when timeLeft < 5 (117-118), so the primitive alpha fades at the end.
  - unk_10 -= 13 from lifetime 7 (121-122). unk_10 is not read by the type 0 draw.
- Render queue (`:129-139`): appendGfx, dist 10, RENDER_MODE_SURFACE_XLU_LAYER3 (136).
- appendGfx (`:141-190`):
  - Camera-yaw rotation (149). Segment 0x09 (152). Translate to pos and multiply by the rotation (154-156), load (158).
  - Prim color (159): (255, 255, 0, alpha = unk_18). Env color (160): (255, 255, 255, 0).
  - **Gate (162):** `if ((type == 0 || type == 1) && time % 2)`, where time = lifetime (144). Odd lifetime draws. Even lifetime skips.
  - Inside the gate:
    - Scale (163): (scaleX, scaleY, 15/14). Matrix push and multiply (164-166).
    - `D_09000600_3936A0` (167): state setup. 32x32 I8 texture D_09000200 (`assets/us/effects/gfx/D_09000600_3936A0.gfx.inc.c:15`). 2-cycle (6). Combine (12). Render mode G_RM_PASS / G_RM_ZB_OVL_SURF2 (13).
    - Tile scroll (168): uls = time*8, lrs = time*8+128 (10.2 format, so the sample window moves 2 texels per lifetime step).
    - `D_090008E8_393988` (169): 20 verts from `D_090006A8_393748` and 16 verts from `D_090007E8_393888`. 18 triangles in 9 quads.
  - Types 1, 2 and 3 only (173-187): D_09000950 and D_09000FF8. Not used for type 0.
  - Pop matrix (189).

## 3. Why a hard rectangle

- Geometry: each quad is an axis-aligned rectangle, x in [-1, 1] and y in [0, 2] in local units (`D_090006A8_393748.vtx.inc.c:4-23`, `D_090007E8_393888.vtx.inc.c:4-19`). Scaled by scaleX and scaleY, so the box is the full rectangle of f1 by f2. The 9 quads are stacked in z (-8..0 and 2..8).
- Texture coords: S 0..1024 (0..32 texels) and T 512..1536 (8..24 texels) (vtx files). Texture wraps (`D_09000600` line 15). The quad edges sit on the texture edges.
- Blend (`include/PR/gbi.h:857-859, 980`): Z_CMP on, no Z write, FORCE_BL, cycle 2 = input color x input alpha + memory x (1 - alpha).
- Alpha (cycle 1 alpha = TEXEL0 x SHADE, cycle 2 alpha = COMBINED x PRIM): alpha = texel intensity x unk_18/255.
- Color (cycle 1 color = (ENV - PRIM) x TEXEL0 + PRIM): R = 255, G = 255, B = 255 x texel. The computed color is white where the texel is bright and yellow only at low texel values. The observed yellow depends on the texel values, which were not read.
- Result: the silhouette is the whole rectangle. Only the texel alpha shapes it. Nothing in this draw samples the part outline.
- Not checked: the texel values at columns 0 and 31 (the edges). The PNG thumbnail is too small to read the pattern.

## 4. Why it flickers

- Deciding line: `src/effects/flashing_box_shockwave.c:162`.
- Value used: `time % 2`, with time = data->lifetime (144). lifetime starts at 0 (41) and adds 1 each update (96).
- Odd lifetime draws the box. Even lifetime draws nothing. The box is therefore on alternate updates. If one update and one draw run per frame, the box shows on every other frame.
- The hit burst is not this draw. It comes from other effects, as the shots show.

## Mechanism summary

The box is a fixed rectangle (f1 by f2, from popup_messages.c:2155-2156), drawn with an alpha-blended I8 texture. The texture alpha, not the part outline, sets the shape. Line 162 draws it only on odd lifetime.

## What would make it follow the sprite outline

- Draw the part's own mesh (its display list and vertices) instead of the fixed quad set in D_090008E8, or
- Use a texture whose alpha is zero except on the outline, and map that texture onto the part's own vertices.

Both options are not tested. The code change is not made.

## Commands used

Read and grep only. No Azahar run. No push.
