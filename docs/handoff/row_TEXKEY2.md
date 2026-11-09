# Row TEXKEY2: cutout key colour fixed at build time (user, 2026-11-09)

## User facts
"Some textures have lines on them where they shouldn't" (arn_02 wall-barrier rocks; likely foliage, fences, frames). User chose the
build-time table after the runtime fix cost 3.6x per conversion (branch TEXKEY: 254 vs 71 us).

## Known (row TEXKEY, branch TEXKEY 4b8e3e6ed, not merged)
Proven: arn_kabe10 (CI4 64x16) key colour 248,248,248 (texkey log). Bilinear mixes the alpha-0 key colour into cutout edges.
The palette rule: each alpha-0 palette index takes the average RGB of the opaque texels that touch texels of that index (alpha stays
0). Code for the rule exists on branch TEXKEY (pm_tex.c texkey functions): reuse it in a host tool.

## Goal
Apply the palette rule at build time to every CI map texture with its own palette (the map texture archives *_tex: image + palette
pairs), so the game loads already-fixed palettes. Zero runtime cost. Textures whose palette is chosen at run time (sprites, effects,
entity TLUT swaps) stay unchanged (list how many are skipped).

## Start state
`sh 3ds/tools/wt_new.sh TEXKEY2`, branch TEXKEY2. PM_ROW=TEXKEY2, PM_LANE=TEXKEY2. One coder agent.

## Steps
1. Host tool (C or Python in 3ds/tools, run by build.sh on the asset copy, never on assets/us itself): for each map tex archive,
   each CI4/CI8 texture with alpha-0 palette entries: apply the rule; write the fixed archive into the build input (romfs / packed ROM
   path the game reads). Print a summary line (textures fixed, entries changed). Delete the runtime texkey dilation; keep the texkey
   log line (debug).
2. Gates: nd_host PASS; arn_02 shot of the wall-barrier rocks (texkey_arn02 bench): no light outline at the rock edges (compare with
   base; crop the rock edge); kmr_11 windows shot (kmr11_cut); spot_mac01_det diff limited to cutout edges (report count); conversion
   cost of arn_kabe10 back to about 71 us; memcheck OK.
3. Commit; delete this row file and row_TEXKEY.md; delete branch TEXKEY after its useful code is taken. Budget: 6 Azahar starts.

## Touch only
3ds/tools (new tool), 3ds/build.sh (one call), 3ds/src/pm_tex.c (remove the runtime dilation only), azt_scenarios.json, bench_defs.json.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
