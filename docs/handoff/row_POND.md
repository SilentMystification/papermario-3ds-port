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

## Facts (2026-11-06, Sonnet pass)
- Water model: MODEL_water = 0xD6 (include/mapfs/mac_00_shape.h:9). Script: src/world/area_mac/mac_00/main.c:95-97 (collider SURFACE_TYPE_WATER, Exec EVS_SetupWaterCustomGfx, EnableTexPanning MODEL_water). water_gfx.c: custom gfx pre builder gfx_build_water. Pass 1: G_ZS_PRIM depth 0x7FF0, Z_CMP no Z_UPD, prim colour 0 0 128 alpha 0, draws the model list (the blocker, writes dark blue where the stored depth is farther than the prim depth). Pass 2: 2CYCLE, G_ZS_PIXEL, G_RM_AA_ZB_XLU_SURF2, mdl_draw_hidden_panel_surface(MODEL_water) (model.c:4911: copy of the model with flags VALID|HAS_LOCAL_VERTEX_COPY, so no panner and no custom gfx on that copy).
- modellog=d6 run (scenario pond_mac00_mlog, log build3ds/azt/logs/pond_mac00_mlog.txt): appendGfx_model draws model 0xD6 TWICE per frame from f40 on: flags 0x2811 (VALID|USES_CUSTOM_GFX|HAS_TEX_PANNER|IGNORE_MATRIX), then flags 0x0081 (the hidden panel copy). Both: rm 17, tex 83, var 0, node type 2, cam 0. At f37 the first draw has flags 0x2801 (custom gfx not yet set). So the model is reached; the draw is not missing at the model layer.
- Not yet known: whether those two draws reach nd_native_submit or the walker, what the GPU state (depth func, depth write, colour mask, blend) is for each, and the cull result. The six walker switches did not bring the water back, so look at the GPU state of pass 1 (Z_CMP without Z_UPD: pm_gpu.c depth test GREATER, no depth write) and pass 2 (XLU surf, 2CYCLE, tex 83 with panner).
- Result (2026-11-06, waterlog run, build 2389): the water draws are reached and native (all 17 frame 200 submits; no walker draw of the water; frame 201 has no line). Pass 1 blocker: nat, om 0f0a4014 (ZS_PRIM), fn GREATER, colour-only write, Z_UPD off, prim 0x00008000, prim z 0x7FF0, 36 tris. Surface (blend on, not ZS_PRIM): nat, fn GREATER, colour-only, tex d10fb365, 36 tris. Decal pass B x9: nat, fn GEQUAL, colour-only, blend on, decal pass A stencil mark, tex 2db1398b, 2 tris each. Shot: the pond interior is the orange-brown bed, not blue.
- Ruled out (picture unchanged, switches reverted): zspix (G_ZS_PRIM draws keep vertex depth, pm_gbi.c G_SETPRIMDEPTH arm): pond identical. zeq (plain Z_CMP draws test GEQUAL with decal bias, pm_gpu.c nk.decal and flush_draw): pond identical. So the prim depth 0x7FF0 and the strict GREATER test of plain Z_CMP are not the value that hides it.
- Not named yet. Next: skip decal pass A (the stencil mark, pm_gpu.c flush_draw_cpu and nd_tmpl.c:318) for the 9 pass B draws, one switch, one shot. Budget: 8 of 8 starts used (5 before the waterlog run, then wlog, zspix, zeq), INCOMPLETE; reset is the orchestrator's call (run_azahar.ps1 -ResetBudget POND).
- Scenario pond_mac00_mlog is in 3ds/tools/azt_scenarios.json working tree (uncommitted: the file mixes other lanes' hunks, split by hand).
