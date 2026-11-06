# Row MENU4: Select menu gaps of epic B, then cheats (MENU-D) and small rows (MENU-E)

## Start state
Branch nds1, main folder. Epic A and B done (commits f0d537096 MN2, 7f41d6e1c MN3b, d6c750d4b MN4, 24e488671 MN5, 0da22de10 MN6 part, 85604c04c MN7 log). Read docs/handoff/agent_menu.md (STATE, section 10: results and gaps), docs/handoff/row_MENU.md (user answers Q1 to Q7) and docs/handoff/archive/plan_menu.md sections 2.6 to 2.12 and 3 (rows MN6, MN7, MN8, MN9, CH0 to CH8, BT1 to BT3).

## Rows (one commit each)
1. MN6 rest: Warp folder rows per area (area name table), one row per map with its entry shown, left/right moves the entry, A warps to the chosen map and entry (live warp, the WARPY floor snap and camera re-init apply). Keep the warp memory (area, map row, entry) in RAM.
2. MN7 rest: a Debug row "Position log combo" (On/Off, saved with the other options) next to the poslog switch.
3. MN3 check: one shotmenu shot that shows the blank row under the tab line.
4. MENU-D cheats CH0 to CH8 (user Q2: Spawn item = ONE item, once over Mario; Q6: ALL 12 further cheats incl. Frame advance and Save/Load position). Cheats are off the file select panel (Q4).
5. MENU-E: MN8 log console colours, MN9 3dsx metadata, BT1 battle table generator, BT2 Battles folder under Boss, BT3 HUD encounter line.
Each row: the plan's gate, menu shots with shotmenu (the menu freezes the frame counter). A hard part becomes a named gap in agent_menu.md; it does not stop the row list.

## Collision
Other code agents work in this folder: S6 (src/entity_model.c, src/entity/*, src/item_entity.c, src/animator.c, pm_gbi.c, pm_gpu.c, nd_model.c) and TEXSTALE (pm_tex.c, nd_tex.c). Do not edit their files. Keep edits buildable, commit quickly, pathspec commits, only your own hunks of azt_scenarios.json and bench_defs.json (check that both files stay valid JSON before you commit: `python -c "import json;json.load(open('<file>',encoding='utf-8'))"`). If git says index.lock exists, wait 10 s and retry.

## Rules and commands
Lane menu: `python 3ds/tools/gen_map_restore.py; PM_ROW=MENU4 PM_LANE=menu sh 3ds/build.sh`; restore 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e, never commit). Azahar: `powershell -NoProfile -File 3ds/tools/azt.ps1 <scenario> -Row MENU4 -Build menu`. Status line in 3ds/logs/perf/status_MENU4.txt before each start. The budget never stops a row: reset and continue. Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. No toggles, no stubs, no git push, no console push, no attribution. Game source only inside `#ifdef TARGET_3DS`. Update agent_menu.md STATE after each commit. Hand back after row 3 and after MENU-E.
