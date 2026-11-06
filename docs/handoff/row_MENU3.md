# Row MENU3: iwa_10 warp entry fix, then MN2 and epic B (main folder, lane menu)

## Start state
Branch nds1, main folder. Read docs/handoff/agent_menu.md (STATE, sections 1 and 5), docs/handoff/row_MENU2.md (rules, commands) and docs/handoff/archive/plan_menu.md sections 2.2 to 2.7 and 3. Done: MN0, MN1, MN3 (exit_armed passes with the menu shot, c6db3c5b1), floorscan debug switch (448c8d1fb).
Fact (MENU2): at iwa_10 x -662 z 587 the floor is fine (collider 28, y 0, walkable, normal y 1.0). The shot shows rock, sky and a text box, no Mario; the probe at frame 30 (before the warp) shows Mario at 0 -1071 19.6. So entry 0 of iwa_10 runs a scripted scene that hides Mario or holds the camera. The position is not the fault.

## Rows (one commit each)
1. ENTRY TEST: list the entries of iwa_10 (src/world/area_iwa/iwa_10, the entry list and the entry scripts). Test each entry with pos -662:0:587 (one run with a tour of entries if the tour supports it, else one start per entry, max 4): per entry log Mario visible (model flag), curFloor, the camera target vs Mario, and whether a text box or a script runs. Table in agent_menu.md section 6. Report the first normal walk-in entry.
2. WARP WAIT: in pm_warp.c, a warp-armed position waits until the entry is done before it places Mario: no text box open, player input enabled (the game's own flag for "player can move"), max 300 frames, then place (with the WARPY floor snap). Log `poswarp: entry done after <n> frames`. Gate: iwa_10 run with the best entry: the top shot 60 frames after `poswarp: placed` shows Mario standing on the ground; mac_01 pos 0:0:200 unchanged. Tell me the entry and the frame count so I update the test spot.
3. MN2 grey top screen, then epic B (MN3b Close game to HOME Menu, MN4 folders and breadcrumb, MN5 file select mirror without cheats, MN6 Warp submenu with the warp memory in RAM and log `menu: warp sub restore area <a> row <r>`, MN7 position log combo) as in row_MENU.md. Menu shots: use shotmenu (the menu freezes the frame counter).

## Collision
Another code agent (row DRAW2) works in this folder on pm_gpu.c, pm_gbi.c, nd_model.c, nd_sprite.c, src/sprite.c, and edits azt_scenarios.json and bench_defs.json in the working tree. Do not edit its files except the menu functions of pm_gpu.c (MN2). For azt_scenarios.json commit only your own hunks (stage with `git add -p` or the index method you used before), never its work in progress.

## Rules and commands
As row_MENU2.md: lane menu (`PM_ROW=MENU3 PM_LANE=menu sh 3ds/build.sh`, azt `-Build menu`), status line in 3ds/logs/perf/status_MENU3.txt before each start, budget reset allowed, pathspec commits only, never git add -N, git add -A, git commit -a, git reset, git stash, git checkout, git restore, bench.py build-at. No toggles, no stubs, no git push, no console push, no attribution. Never commit 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). Update agent_menu.md STATE after each commit. Hand back after row 2 and after epic B.
