# Diagnosis: sun overlay and god rays, Dry Dry Desert (SUNRAYS)

## Status
- Cause not proven. No source edit. No build.
- Azahar: 1 start of 3 used. Scenario `sun_sbk` added (unverified, run with `-Try`).
- Binary: build3ds\pm_3ds.3dsx (16:36). Git HEAD at run: 521642b9e. [M]

## Effect path [C]
- Map call: reference/paperboat/src/world/area_sbk/sbk_00/main.c:24 calls N(SpawnSunEffect). Most sbk maps do the same.
- reference/paperboat/src/world/common/todo/SpawnSunEffect.inc.c:5 calls fx_sun with FX_SUN_FROM_LEFT.
- 3ds/src/effect_loads.c:739 fx_sun: load_effect(0x82), then sun_main.
- reference/paperboat/src/effects/sun.c: sun_render sets RENDER_MODE_CLOUD_NO_ZCMP (dist 10). sun_appendGfx sets a screen ortho (+-1600 by +-1200), prim alpha alpha>>1, and five ray layers.
- Each ray layer sets tile 1 with gDPSetTileSize: uls = offsetS + 44*i, width 63 texels, height 31 texels. offsetS scrolls per frame.
- Ray meshes come from the ROM. The 3DS build runs them through pm_gbi.c.

## Display lists, decoded from ver/us/baserom.z64 [M]
- Segment 9 is ROM 0x40BBA0. Base list 0x898: SETTIMG I8 at 0x09000200. Tile 1 is I4, 256 by 8 texels, line 16. Clamp S (cms=1, masks=8). Wrap T (cmt=0, maskt=3, period 8 texel rows).
- Ray lists 0x7C0 to 0x850: each draws 1 or 2 F3DEX2 quads (G_TRI2 0x06) from 4 or 8 vertices.
- Vertex UV in the ray meshes: s from -17.47 to 16.0, t from 0 to 66.12. Vertex alpha 255, 229 or 126.
- The texture rows 0 to 7 at ROM 0x40BDA0 are identical (128 bytes per row, nibble pattern 0000 cccc cccc 0000 5555...). The data varies in S only.

## Azahar shots, run 1 (frames 200 and 204) [M]
- Files: build3ds\azt\shots\sun_sbk\0000_10_0.ppm and 0001_10_0.ppm. Log checks: 3 of 3 pass.
- Screen is turned 90 degrees in the PPM (240 by 400).
- Sun disc is not clear in either shot.
- Sky strip, x 150 to 240 and y 130 to 340, at 4x: faint translucent cells with hard edges. Cell pitch about 19 screen px, cell width about 10 px. The cells lie along one diagonal.
- Two thin light streaks at the right edge look dashed (gaps).
- Frames 200 and 204 look alike by eye. No pixel compare run.

## Hypotheses (ranked) [E]
1. Tile 1 S clamp at the window edge. The ray vertices use s below 0 (-17.47 to -1.47). The N64 clamps to the window column at uls, and uls changes per layer and per frame. The port keeps the clamp on the GPU (pm_tex.c:120-124 wrap_of; pm_gbi.c:1146-1149 V21 comment, linear map in dual_bind). A clamp edge that sits at the texture edge, not at uls+63, gives hard bands that move with the scroll. Most likely. Untested.
2. T wrap period. Rejected for now: rows 0 to 7 are identical [M], so the 8-row repeat cannot make the seams. Only a T read beyond the loaded rows could, and the mask holds that off.
3. Fold of one draw across the window. pm_gbi.c:1131-1133 states that a wrap inside one draw is not reproduced. The ray draws span 64 texels in T and 16 in S. The fold (pm_gbi.c:1134-1160, t1_fold) moves the start but not the inside wrap. Possible seam source. Untested.

## Smallest fix (not applied)
- If hypothesis 1: in the aux texture build (pm_tex.c:1049-1058 and the wrap copy at pm_tex.c:328-334, 368-371), build the S axis at the tile window width with the clamp edge at the window, and set aux_shr_s only when the tile clamps. Keep the fold off for a clamped axis (already done at pm_gbi.c:1149).
- If hypothesis 3: skip t1_fold for a draw whose extent is more than the mask (pm_gbi.c:1139 test `ex[a] >= wsz`).

## Picture check plan
- Log one line per ray draw (pm_gbi.c, tile 1 path): ox1, oy1, t1_w, t1_h, t1_p[0..3], wrap S and T, draw u0 u1 v0 v1. Phase gate, debug only. One run.
- Compare with the sun_sbk frame 200 shot. Pass if the hard cells map to one of the logged values (clamp edge or fold origin).
- Fix pass: the sky strip has no cells in frame 200 and 204, and the ray streaks have no gaps. Read the shot by eye.

## Next
- One run of sun_sbk with the logging line (budget left: 2 starts). Then one fix run.
- Azahar may show the same cells as the console. Azahar lacks an N64 or console reference, so the console shot is needed for the final judgement.
