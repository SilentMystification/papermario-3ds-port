# Investigation 6: THUNDER-BOX sparkle square (blend and alpha path)

Scope: read only. No source edit, stage, commit or revert. Question: where can a texel with alpha 0 still write colour in the port?

## Result

No code path found where an alpha 0 texel writes colour through the blender. The square needs a non-zero alpha over the quad. The cause of that alpha is not found by reading.

## Setup state (sparkle draw)

- Setup DL: assets/us/effects/gfx/D_09000F20_338EE0.gfx.inc.c:9-17.
  - Render mode `G_RM_CLD_SURF, G_RM_CLD_SURF2`. Blender: `GBL_c1(CLR_IN, A_IN, CLR_MEM, 1MA)`. Colour = IN * A_IN + MEM * (1 - A_IN). Z_CMP and Z_UPD are both off (include/PR/gbi.h:880-882).
  - Combine (1 cycle): colour = (1 - PRIM) * TEXEL0 + PRIM. Alpha = PRIM_A * TEXEL0_A.
  - Texture: IA 8 bit, tile 176 x 22, CLAMP on S and T, bilerp.
- Quad: D_09001210_3391D0 (one gSP2Triangles, 4 vertices), table D_E0022CD0 in src/effects/sparkles.c:14-18.

## Port blend and alpha path

- Blend flag: `blend` at 3ds/src/pm_gbi.c:661-662. CLD_SURF gives M1 = CLR_MEM and B1 = 1MA, so blend = 1.
- Alpha test: `atest` at 3ds/src/pm_gbi.c:663-668. CLD_SURF has no CVG_X_ALPHA (0x1000) and no threshold, so atest = -1 (alpha test off).
- Nocolour: `nocol` at 3ds/src/pm_gbi.c:670-678. P1 is CLR_IN, so nocol = 0.
- Blend factors used when blend is on: `sh_blend(ADD, ADD, SRC_ALPHA, ONE_MINUS_SRC_ALPHA, SRC_ALPHA, ONE_MINUS_SRC_ALPHA)` at 3ds/src/pm_gpu.c:761-763 (flush path). Rect path at 3ds/src/pm_gpu.c:1055-1058.
- With src alpha 0: colour = dst. So a texel alpha 0 writes no colour here. The blend cannot produce the square if the combined alpha is 0.
- Alpha test is off in this state (pm_gpu.c:755-756), so no alpha compare value applies.

## Alpha source

- Alpha slots: Aa = PRIMITIVE, Ac = TEXEL0. Alpha = PRIM_A * TEXEL0_A.
- Mode choice: `alpha_tex` true, so the legacy mode is 2 (pm_gbi.c:544-571). Mode 2 cannot express the (1 - PRIM) colour, so the sparkle needs the plan path (combine 8). The plan alpha reads TEXEL0 alpha (cc_v, pm_gbi.c:883-900: sel 1 = t0[3], sel 3 = pr[3]).
- `comb_alpha_shade` is set only from SHADE in the alpha slots (pm_gbi.c:3391-3392). It is 0 for this combine. Gates that use it:
  - gv_params_calc `a255` at pm_gbi.c:1279, applied at pm_gbi.c:1306 (cm[3] = 0, ca[3] = 1, alpha forced to 1 on the raw Vtx path).
  - CPU draw vertex alpha at pm_gbi.c:1976-1977 (poly.a = 255).
  - Both gates are off when plan_active is set. Whether plan_active is set for the sparkle draw is not verified.
- Analysis: if alpha is forced to 1 but the TEV alpha stage still multiplies by TEXEL0 alpha, the texel alpha is kept. So a forced vertex alpha alone does not remove the alpha 0 background. The override matters only if the TEV alpha stage for this draw does not read TEXEL0 alpha. That was not checked (pm_gpu.c TEV builder, plan stages).

## Texture sample path (checked, not the cause)

- IA 8 bit goes native (pm_tex.c:1167). Raw byte copy to LA4 (pm_tex_tx1.h:108-127 path, else branch at 122-124). Convention: L high nibble, A low nibble, same as the IA4 branch. Alpha 0 texel stays alpha 0.
- Clamp: pm_tex_tx1.h:30-36 clamps to n - 1 when mask is 0 (mask >= w gives mw = 0, pm_tex.c:1161). So texels beyond x = 175 are not junk. The earlier suspicion of junk texels is not supported.
- Wrap: wrap_of at pm_tex.c:120-124 gives CLAMP_TO_EDGE for mode 2.

## Confidence

- That the blend path does not write colour for alpha 0 texels: high (direct code read).
- That the square comes from the alpha path: not established. Source of the non-zero alpha over the quad is unknown.

## Next check (no start used yet for this question)

1. Log, for the sparkle draw only: plan_active, plan vtxname, a255, and the TEV alpha stage source. One line per draw, frame 260.
2. If the log shows a255 = 1 with plan_active = 0 and the alpha stage not reading TEXEL0, the override at pm_gbi.c:1306 is the writer. Fix the gate (alpha_tex) at pm_gbi.c:1279 only after that is shown.
3. If plan_active = 1, check the plan alpha stage builder for PRIMITIVE alpha sourcing.
