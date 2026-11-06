# Row POND: the mac_00 pond water does not draw

## Goal
User report 2026-11-06 on the O3DS XL, build a5cd61e1a (SGEN): in mac_00 at -178 0 88 the pond water does not draw at all. The user sees through the pond into the background layer. Before SGEN the pond drew, but wrong (an older bug: archive/diag_primz.md, PRIMZFIX 9c0387c33, toad_pond scenario). Make the water draw as on the N64.

## Start state
Branch nds1, HEAD 2e863845f or later. Suspect first: SGEN commit 47c53a895 (two-texture combines now draw natively in nd_raw_native, pm_gpu.c, plan.uses_tex1; pm_tex_native_unit1 in pm_tex.c; uvx uniform in nd_emit.c; ent_gdl generic list path in pm_gbi.c). The water is likely a two-tile scrolled texture (tex1) or a list in the frame heap.

## Steps
1. Scenario: copy toad_pond to pond_mac00 with pos=-178:0:88 (warp=mac_00 entry, story as toad_pond) and shots at the pond. One run on a HEAD build: confirm the water is missing in the shot.
2. Bisect by debug switch, not by build: run once with the generic list path off for the water draw (an existing skip switch, grep `entskip` or a new debug switch `notex1nat` that sends uses_tex1 draws back to the walker). If the water returns, the cause is that path: name the value (unit 1 texture address, format, uvx, combine stage for TEXEL1, alpha).
3. Fix that value (2 tries per named cause, then try the next code point; do not hand back while a cause is named). Then check the old bug (PRIMZFIX notes) in the same shot.
4. Gate: one shot shows the water at the pond (not the background). mac_01 picture unchanged by eye. Commit the fix and the scenario.

## Touch only
3ds/src/pm_gpu.c, pm_tex.c, nd_emit.c, pm_gbi.c (water path lines), 3ds/tools/azt_scenarios.json (own hunk). No other code agent runs now.

## Rules and commands
CLAUDE.md rules 5, 7, 9. Build: python 3ds/tools/gen_map_restore.py; PM_ROW=POND PM_LANE=pond sh 3ds/build.sh; restore 3ds/src/map_restore_table.c (never commit). Azahar: powershell -NoProfile -File 3ds/tools/azt.ps1 pond_mac00 -Row POND -Build pond -Try. Status line in 3ds/logs/perf/status_POND.txt before each start. Budget: reset and continue. Pathspec commits. Never git add -A, git commit -a, reset, stash, checkout, restore. No toggles (debug switches allowed), no stubs, no push, no attribution. Hand back with the cause (file:line and value), the commit, the shot path.
