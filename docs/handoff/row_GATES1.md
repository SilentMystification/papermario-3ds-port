# Row GATES1: run the pending gates of the merged branches on nds1

## Goal
Branches menu and s4 are merged into nds1 (commits e5c220326 and 6124eb9de) with their code untested, and S3L has a zmark WIP (ee85973f5). The merged tree builds. Run each pending gate once, view the pictures, and report pass or fail with the numbers. Fix only trivial faults (a scenario regex, a typo, a compile warning). Any real fault: report it with the log line and the picture, do not redesign.

## Start state
Branch nds1, main folder. Last commit 6124eb9de. Tree: user files only. Read docs/handoff/agent_menu.md (STATE and section 1), docs/handoff/agent_s4.md (STATE), docs/handoff/agent_s3n.md (STATE top, S3L zmark lines).

## Gates (one Azahar start each, status line in 3ds/logs/perf/status_GATES1.txt before each)
1. MN0 menu_shot: `powershell -NoProfile -File 3ds/tools/azt.ps1 menu_shot -Row GATES1`: until `menu: shot bot_m0000`; the bottom shot shows the General tab.
2. MN1 menu_pause: until `menu: game resumed at frame`; paused and resumed frame numbers equal; bottom shot shows the menu.
3. MN3 exit_armed: the bottom shot shows the blank line under the tab line, the armed row label blue and "Press A again" red.
4. WARPY: one run at iwa_10 (switches of spot_mac01_pic via `bench.py show spot_mac01_pic`, with warp=iwa_10:0 pos=-662:0:587; use bench spot_iwa10_pic if bench.py list shows it): the log has `poswarp: floor y 0 -> <y>` with y above 0, the scene line pos y equals that, the top shot 60 frames after the warp shows Mario standing.
5. S4-1 sprcheck: spot_mac01_pic switches plus `sprcheck` (run_azahar.ps1 direct, 25 s, until `sprcheck f\d+ recs`): eq above 0, diff 0. A diff: write the field and both values.
6. S3L zmark: iwa_10 switches plus `ndmcheck` (or bench spot_iwa10_ndmcheck): ndmcheck diff 0; the ndmr refusal line shows zmark refusals down from 647; the iwa_10 top picture right by eye (no wrong depth, no missing object). If depth looks wrong, the WIP note says the pal row layout may be the other way: report it, do not change it.
Also: `python 3ds/tools/bench.py run nd_host --row GATES1 --why "host tests after merge"`.

## Report
docs/handoff/gates1.md: one line per gate (pass or fail, the numbers, the shot path). Commit it by pathspec. Hand back with the table.

## Rules
Collision: the other agents are paused for this row; still use `git status --short` before each commit and pathspec commits only. Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. Do not edit game code beyond a trivial fix (then commit it by itself). No toggles, no stubs, no git push, no console push, no attribution. Build: python 3ds/tools/gen_map_restore.py; PM_ROW=GATES1 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e, never commit it). Budget reset allowed: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget GATES1.
