# Row SGEN: any static display list draws through the entity list path

## Goal
WCEN3 (docs/handoff/agent_wcen.md): models, sprites and entities draw natively. Walker draws per frame that remain are effects (walking_dust, stars_shimmer, sun), worker back UI, the camera and background lists (S2), and the front UI. Breadth first: one generic path instead of one record per producer. The S6 path (pm_gbi.c ent_draw: compile a list once per (list, vertex segment base) with nd_ent_segments, nd_ent_strip, nd_dlc_compile; state ops run in place; triangles from the static pools; refused segments run their old words; content hash, dynamic after 4 changes) works for any list. Apply it to every G_DL the walker meets, not only after a G_PM_ENT_DRAW record.

## Rows (one commit each)
SGEN-1 GENERIC G_DL: in the walker G_DL arm, a list not already handled (no record) goes to ent_draw with the current vertex segment base. Keep the refusal and dynamic rules. Lists built in the frame heap (gMainGfx, display context) are dynamic by address: skip them (address range check), they stay walker. Counter line `gdl f<n> lists <n> native <n> dyn <n> refuse <n> draws native <n> walker <n>` every 300 frames.
Gate: wd (WCEN3 census, walker draws) of walking_dust_appendGfx, stars_shimmer_appendGfx, render_workers_backUI drops toward 0 at mac_01 and iwa_10. Pictures right by eye at mac_01, iwa_10, sbk_56, and battle_start (small diffs are a pass).
SGEN-2 the S3 and S6 fallback draws at iwa_10 (model wd 1790, entity wd 928 per 300 frames): name the fallback reasons (ndm and entnat why counters), fix the biggest one.
SGEN-3 named fallbacks left for S10, written in agent_s6.md.

## Rules
Main folder, branch nds1. No other code agent in pm_gbi.c, pm_gpu.c, nd_*.c now. CLAUDE.md rules 5, 7, 9. Build: python 3ds/tools/gen_map_restore.py; PM_ROW=SGEN PM_LANE=s6 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c (never commit). Azahar: azt.ps1 <scenario> -Row SGEN -Build s6. Status line in 3ds/logs/perf/status_SGEN.txt before each start. Pathspec commits. No git add -A, reset, stash, checkout, restore. No toggles (debug switches allowed), no stubs, no push, no attribution. If an edit does not change the shot, change a different code point (the try limit is 2 per named cause), not the row. Hand back after SGEN-3.
