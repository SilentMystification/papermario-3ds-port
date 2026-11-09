# Row TEXKEY: light or dark lines on cutout texture edges (user, 2026-11-09)

## User facts
User: "some textures have lines on them where they shouldn't. arn_02: the flat foreground rocks on the wall barrier." Don't confirm
visually by exploration; the cause is by code (below). Likely the same for many "lines on textures" (foliage, fences, wall frames).

## Cause (shared oracle, code and asset read, main 00a4e1901)
arn_02 model `waku` (tree index 6, arn_02_shape.bin), texture arn_kabe10tif CI4 64x16, wrap, bilinear, render mode ALPHATEST
(G_RM_AA_ZB_TEX_EDGE, src/model.c:680) -> nd_edge_aa blend, alpha test 0x1F (nd_tmpl.c:291). Palette index 2 = transparent key RGB
255,255,255 alpha 0, around every rock outline. Bilinear mixes the key colour into the edge: frame = a^2 rock + a(1-a) white +
(1-a) dst, up to 25% white at a = 0.5. The N64 hides it with coverage + VI filter. General: every cutout whose key colour differs
from its edges (KMR11W census: 158 of 380 CI cutouts have a dark key, the rest light).

## Start state
`sh 3ds/tools/wt_new.sh TEXKEY`, branch TEXKEY. PM_ROW=TEXKEY, PM_LANE=TEXKEY. One Haiku agent.

## Steps
1. Check (log, no shot hunt): debug switch texkey; in pm_tex.c expand() CI branch (:418-427) print once per conversion
   "texkey <img> <w>x<h> n0 <alpha-0 count> keyrgb <avg RGB of alpha-0 texels with an opaque 4-neighbour>". One Azahar run in arn_02
   (new scenario + bench, warp arn_02, cap 15 s): arn_kabe10 keyrgb near 255,255,255. If not -> hand back (premise false).
2. Fix: colour dilation at conversion. In pm_tex.c expand(), after the CI4/CI8 and RGBA16 (and IA if it has alpha-0 texels) loops:
   each alpha-0 texel with an opaque 4-neighbour (indices wrapped by the tile mask, or clamped) gets RGB = average of those
   neighbours; alpha stays 0. One pass per conversion (cached), integer only, sequential. Same rule in 3ds/tools/texbake.c if it
   mirrors expand. Host test in nd_host (small CI4 texture: key texels next to opaque get the neighbour colour, alpha 0 kept).
3. Gates: nd_host PASS; memcheck OK; one shot in arn_02 of the rocks over the wall (no light outline) and one of the kmr_11 windows
   (kmr11_cut scenario; rims match the wall); one XLU scene shot for regressions (pick an existing XLU bench). Byte-equal not expected.
   Report the conversion cost (us per texture, from pm_tex timing) before/after on the arn_02 run.
4. Commit; delete this row file in the completing commit. Budget: 6 Azahar starts. No-repeat: a try with no picture change ends the try.

## Touch only
3ds/src/pm_tex.c (expand), 3ds/tools/texbake.c (if it mirrors), host test, azt_scenarios.json, bench_defs.json.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
