# Row MENU5: Select menu cheats (MENU-D) and small rows (MENU-E)

## Start state
Branch nds1, main folder. Epic A and B done, MN6 rest and MN7 row done (21ba60d65, bc6b263a8, 37d8dd007). Read docs/handoff/agent_menu.md (STATE, sections 10 and 11), docs/handoff/row_MENU.md (user answers: Q2 Spawn item = ONE item once over Mario; Q4 no cheats on the file select panel; Q6 ALL 12 further cheats incl. Frame advance and Save/Load position) and docs/handoff/archive/plan_menu.md sections 2.8 to 2.12 and 3 (rows CH0 to CH8, MN8, MN9, BT1 to BT3).

## Rows (one commit each, plan order)
CH0 item name table generator, CH1 Cheats tab and Set Stats folder, CH2 Badges folder, CH3 Key Items folder, CH4 Partners folder, CH5 Spawn item (one item), CH6 Moon Jump and pm_cheat_tick, CH7 cheats batch 1, CH8 cheats batch 2 (split if large). Then MN8 log console colours, MN9 3dsx metadata, BT1 battle table generator, BT2 Battles folder under Boss, BT3 HUD encounter line.
Each row: the plan's gate, menu shots with shotmenu. Copy 3ds/tools/pad_*.txt to %APPDATA%\Azahar\sdmc\3ds\PaperMario before a start that uses a pad file. A hard part becomes a named gap in agent_menu.md; it does not stop the list.

## Collision
Other code agents work in this folder: S6b (src/entity_model.c, src/entity/*, src/item_entity.c, src/animator.c, pm_gbi.c, pm_gpu.c, nd_model.c, nd_dlc.c) and an effects trace (read-only). Do not edit their files. Keep edits buildable, commit quickly, pathspec commits, own hunks of azt_scenarios.json and bench_defs.json only, both stay valid JSON. Other agents' Azahar runs share the sdmc log: if a run ends with the marker missing, check the log time before you count it. If git says index.lock exists, wait 10 s and retry.

## Rules and commands
Lane menu: `python 3ds/tools/gen_map_restore.py; PM_ROW=MENU5 PM_LANE=menu sh 3ds/build.sh`; restore 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e, never commit). Azahar: `powershell -NoProfile -File 3ds/tools/azt.ps1 <scenario> -Row MENU5 -Build menu`. Status line in 3ds/logs/perf/status_MENU5.txt before each start. The budget never stops a row: reset and continue. Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. No toggles, no stubs, no git push, no console push, no attribution. Game source only inside `#ifdef TARGET_3DS`. Cheats write gPlayerData live, never the save file unless the user saves. Update agent_menu.md STATE after each commit. Hand back after CH8 and after BT3.
