# Epic MENU: the Select menu changes of plan_menu (pause, confirm style, folders, warp, cheats, small rows)

## Goal
The Select menu epic was planned on 2026-11-03 (docs/handoff/archive/plan_menu.md, commit 8fe9ec4ee) and only parts landed (for example the Warp area submenu, PT01c in 3ds/src/pm_options.c). Find what is in and what is not, then build the rest. The plan's sections 2 (design) and 3 (rows) are the spec. Read sections 1, 2 and 3 of plan_menu.md in full.

## Start state and where you work
Your own git worktree, branch `menu`: first `git switch -c menu nds1`. Build lane: `PM_LANE=menu sh 3ds/build.sh` (run `python 3ds/tools/gen_map_restore.py` first; never commit 3ds/src/map_restore_table.c). Azahar: `powershell -NoProfile -File 3ds/tools/azt.ps1 <scenario> -Row MENU -Build menu`. If the permission check refuses an Azahar run in the worktree, do not bypass it: code and build, commit the row as "code, gate not run", continue to the next row, and list the unrun gates in docs/handoff/agent_menu.md. Never edit files in the main folder C:\Users\Andrew\Documents\GitHub\papermario-3ds-port: two other code agents work in this repo (S3L in the main folder, S4 in worktree s4).

## User answers to the plan questions (2026-11-05)
- Q1 Close game: end the app and return to the 3DS HOME Menu (chainload; row MN3b).
- Q2 Spawn item: ONE item, spawned once over Mario (no count row, no grid; row CH5 is smaller).
- Q3 Breadcrumb: plan default, the tab line shows the path (`Debug > Boss`).
- Q4 File select mirror: the same pages, without the Cheats tab and the live Boss and Battles rows (row MN5).
- Q5 Fullscreen: plan default, X factor 10/9. NOT in this epic (see Not now).
- Q6 Cheats: ALL 12 of plan section 2.8, Frame advance and Save/Load position included (rows CH7, CH8).
- Q7 Parallel worktrees: yes (this epic runs in its own worktree).

## Order
1. AUDIT first (no code): for each row of plan_menu.md section 3 (MN0 to MN9, CH0 to CH8, BT1 to BT3) grep its key symbol or log line (for example `menu: game paused`, `pm_gpu_menu_grey`, `Press A again`, `page push`, `poslog`, `cheat:`, `dbggame: spawn`, `pm_battle_table.h`, smdh title in 3ds/CMakeLists.txt) in 3ds/src, src, include, 3ds/CMakeLists.txt and in `git log --all --oneline -i --grep=<row id>`. Write a table row id | in / partial / not in | evidence in docs/handoff/agent_menu.md. Commit it.
2. Then the epics in the plan order: MENU-A (MN0, MN1, MN2), MENU-B (MN3, MN3b, MN4, MN5, MN6, MN7), MENU-D (CH0 to CH8), MENU-E (MN8, MN9, BT1, BT2, BT3). Skip rows the audit shows as in. For a partial row do only the missing part.

## Not now
MENU-C (RES1 to RES3, resolution modes): it edits src/model.c, src/cam_main.c and the shared draw code that the S3 epic changes now. Leave it for later; note it in agent_menu.md.
pm_gpu.c: only the rows that need it (MN0 menu shot, MN2 grey), with the smallest change (new functions next to pm_gpu_menu_draw, no edit of the draw path).

## Rules
`git status --short` before each edit and commit. Pathspec commits only (`git commit -m ... -- <paths>`). Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. New scenarios go into 3ds/tools/azt_scenarios.json in the same commit as the row (on your branch; the merge is the orchestrator's). No toggles (debug switches and launch values allowed), no stubs, no git push, no console push, no attribution. Game source edits only inside `#ifdef TARGET_3DS` (filemenu, pause code). hw_guidelines.md. A plan detail that no longer fits the code: take the plan default, write it in agent_menu.md, continue.

## Done
All rows of MENU-A, MENU-B, MENU-D, MENU-E in or committed (gates run where the permission check allows), docs/handoff/agent_menu.md with the audit table and a STATE block (10 lines or fewer), updated after each commit. Hand back after each epic (A, B, D, E) with the commit list. The orchestrator merges branch menu into nds1.
