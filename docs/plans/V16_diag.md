# V16 diagnosis: box around flickering sprites

Status: cause NOT confirmed. Two Azahar starts (row V16d) did not give usable evidence. Read this file for the ranked leads and the exact next test.

## What the decomp does (read)
- A damaged actor part with opacity < 255 sets `actor->renderMode = RENDER_MODE_SURFACE_XLU_LAYER3` (src/battle/actor_rendering.c:779).
- `spr_appendGfx_component` (src/sprite.c:363) then picks `TranslucentSpriteGfx` (1 cycle, `G_RM_ZB_CLD_SURF`, combine `PM_CC_02`, prim alpha = opacity)
  or `TranslucentShadedSpriteGfx` (2 cycle, `G_RM_PASS`, `G_RM_ZB_CLD_SURF2`).
- A flat quad goes through `spr_appendGfx_component_flat`. With opacity < 255 it sets `G_RM_ZB_CLD_SURF` (no `CVG_X_ALPHA`, no alpha compare) and the combiner alpha is PRIM x TEXEL0 (`PM_CC_3D`) or ENV x TEXEL0 (`PM_CC_0A`).
- So on the N64 the quad is hidden only because the final alpha is texel alpha x opacity. A transparent texel has alpha 0 and writes nothing.
- The opaque path (opacity 255) uses `CVG_X_ALPHA` and hides the quad by the alpha cut.

## What the port does (read)
- `sync_render` (3ds/src/pm_gbi.c:~470): for the translucent modes blend = on, alpha test = -1 (off), depth write off (no `Z_UPD`). The opaque modes: blend off, alpha test 0x30.
- Therefore the translucent sprite has NO alpha cut. The box is hidden only if the PICA TEV alpha output is texel alpha x opacity.
  If any path outputs only the opacity (or only the vertex alpha), every texel of the quad blends in and a box shows.
- Legacy combine (3ds/src/pm_gbi.c `sync_legacy`, pm_gpu.c flush): mode 3 = texture colour, alpha from the vertex only. Mode 2 = texel alpha x vertex alpha, and the sprite opacity is PRIM, not vertex.
  The plan path (combine 8, pm_comb.c) is exact. A fallback to legacy 2 or 3 for a PRIM-alpha sprite mode would give a box.
- Precedent: S26 (docs/HISTORY.md:570 to 575) fixed I4 and I8 alpha, mode 3 and mode 4 for the same symptom (black squares around sparkles). V16 is a case that S26 did not cover.

## Evidence
- Start 1 (preset=toad battle autoa shots rmlog): the rm log lines show the shadow mode `oml 00504f50` (XLU decal, prim alpha 0x80) and sprite modes `00551238` (opaque, `CVG_X_ALPHA`). The log tail did not reach a flicker frame. The run reached frame 14221 in 133 s.
- Start 2 (same without rmlog, shots): the output folder `build3ds/shots` and `build3ds/last_log.txt` are shared with the other running agent (row V17d). The copied shots are a mix of the two runs. They are not usable for a battle frame.
- Seen by chance in the mixed shots (shots 0212 and 0148): small black rectangles at intro sparkle and star positions. This matches the V16 note "some intro stars have a bounding box". It proves the box exists in the current build.
  It does not name the render mode.

## Ranked leads
1. A sprite or effect draw whose combiner alpha does not reach the TEV as texel alpha x constant (legacy mode 3, or the plan building a wrong alpha stage). Test: log `oml`, comb a-fields and the chosen `combine`/plan for the box draw.
2. Translucent mode without alpha cut and a texture whose transparent texels have alpha > 0 after conversion in 3ds/src/pm_tex.c (CI4 with palette alpha, LA8, I4). Test: dump the texture of the box draw.
3. 2-cycle PASS mode (`TranslucentShadedSpriteGfx`) read as a mix or as nocol in `sync_render` (first block p, a, m, b decode). Test: rmlog line of the box draw, decode by hand.

## Fix plan (apply after the test below names the draw)
- If lead 1: file 3ds/src/pm_gbi.c, function `sync_legacy`. Do not return mode 3 when the alpha combiner reads PRIM or ENV times TEXEL0. Route that form to `const_rgb_tex_alpha` style (mode 4) or to the plan.
- If lead 2: file 3ds/src/pm_tex.c, the texture conversion for the format in the log. Force alpha 0 for a texel whose RDP alpha is 0.
- Safe general guard that matches libultraship: in `sync_render`, a translucent mode with blend on and a texture should still discard alpha < 0x01 texels (`atest = 0` with GPU_GREATER 0). Use only if the root cause is the texture alpha. Do not use it to hide a combiner fault.

## Next test (1 Azahar start, row V16d has 2 of 4 left)
Run when no other agent writes `build3ds/shots`. Use a fixed frame window so the shot is small.
1. Find the hit frame: `PM_ROW=V16d powershell -File 3ds/run_azahar.ps1 -Turbo -Timeout 400 -Until "NEVERMATCH" -Debug "fast preset=toad battle autoa shots shotwin=<f>:30 drawlog=<f>:2 rmlog"`.
   Pick `<f>` from a hit frame: add a log line when `actor->opacity` first drops below 255 (one `pm_log` line in a temporary local build) or use the intro: `fast shots shotwin=3140:60 drawlog=3140:2` (stars with boxes around frame 3140 to 3200 of the V17d run).
2. Copy `build3ds/shots` and `build3ds/last_log.txt` at once.
3. In the log find the draw with `oml`, `comb`, `tex` fmt of the box quad. Match it to a lead above.
4. After the fix: the same command, one shot of Thunder Rage and one shot of a damage flicker, no box.
