# Row WCEN: what still runs on the walker after S3, S4, S6

## Goal
Models, sprites and entities draw natively. Measure what the walker still runs, per producer, so the next slices (S5 HUD, S7 world and battle custom gfx, S8 effects, S9 screen) go in order of walker cost. No code change.

## Start state
Branch nds1, HEAD 150cd3408 or later. Read docs/handoff/plan_native_slices.md sections S5, S7, S8, S9 (file lists). The producer census exists: pm_gbi.c, grep `P1: producer census` (phase mode only, op counts and sampled time per producer mark from pm_interp_open). The opsplit line gives ops per class.

## Steps
1. Find the switch that prints the producer census and opsplit lines (grep `pk_ops` and `opsplit` in pm_gbi.c for the print and its switch). Write the switch words in the status line.
2. Run it at 5 scenes with phase mode: spot mac_01, mac_00, sbk_56, iwa_10 (bench defs spot_*), and battle_start. One Azahar start each, cap 25 s.
3. Map each producer mark address to its function (the mark is the caller of pm_interp_open: use the 3dsx map file build3ds/pm_3ds.map or the logged symbol if printed).
4. Write a table in docs/handoff/agent_wcen.md (new subject, add its INDEX.md line): per scene the top 15 walker producers by sampled time and ops, with function, source file, and the slice that owns the file (S5, S7, S8, S9, or "fallback of S3/S4/S6" with the fallback name).
5. Add a total line per scene: walker ops, native draws, walker share of gbi time.

## Done criteria
The table has 5 scenes. Commit agent_wcen.md and INDEX.md. Numbers stay in the status file and the table (counts only, no claims).

## Rules
Main folder, other agents edit pm_gbi.c (FX1-2b), menu files (MENU5). Do not edit code. If the census needs a code change to print the function, hand back and name it. Build: python 3ds/tools/gen_map_restore.py; PM_ROW=WCEN PM_LANE=wcen sh 3ds/build.sh; restore 3ds/src/map_restore_table.c (never commit). Azahar: powershell -NoProfile -File 3ds/tools/azt.ps1 <scenario> -Row WCEN -Build wcen (add debug words with the scenario debug override if azt.ps1 has one, else a new scenario in azt_scenarios.json, own hunk). Status line in 3ds/logs/perf/status_WCEN.txt before each start. Pathspec commits. No git add -A, reset, stash, checkout, restore. No push, no attribution.
