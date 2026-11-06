# Rows S4-2 to S4-4: sprites draw natively, then the old sprite words go (main folder, main lane)

## Start state
Branch nds1, main folder. S4-1 PASS (24b52e937): sprcheck at mac_01 eq 2024, diff 0 (bench spot_mac01_sprcheck). Read docs/handoff/agent_s4.md (STATE, the S4-2 plan) and docs/handoff/row_S4.md (rows S4-2 to S4-4, gates). S3L open classes wide_edge and code 2 TLUT are named, not in this row.

## Rows (one commit each, in the main folder on nds1: a row that passes its gate is committed here, no branch needed)
S4-2 nd_sprite_draw (nd_sprite.c): vertices of the record quad (record word 5) to the ring with word copies, the matrix (word 4) as pcomp into pal[0], the state from nd_model_state as sprcheck uses it, a cache per sprite (raster, palette, render mode; reset at map load), then nd_native_submit. With sprcheck: also compare pal[0] and the index range. Gate: sprcheck diff 0 at mac_01 and at battle_start.
S4-3 switch: the record arm draws natively, the old sprite words after the record are skipped (word count in the record), a refused sprite runs the old words (fallback, counted in the sprcheck line). Gate: spot_mac01_pic (NPCs and Mario), spot_mac00_pic, battle_start: pictures right by eye; fallback count logged.
S4-4 delete: when fallback is 0 at those scenes, sprite.c writes no G_PM_TEXI header and no old words for native sprites; delete what lost its caller. Else name the left classes in agent_s4.md and delete only the dead parts.

## Budget
The Azahar budget only stops repeats of the same shot. At the cap with gates unrun: reset (`powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget S4b`) and continue. Never stop a row for the budget.

## Collision
Another code agent (row MENU3) works in this folder on pm_warp.c, pm_options*.c, pm_main.c, pm_input.c, the menu functions of pm_gpu.c and its own hunks of azt_scenarios.json. Do not edit those. Keep edits buildable, commit quickly, pathspec commits, only your own hunks of shared files.

## Rules and commands
Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S4b sh 3ds/build.sh; restore 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e, never commit). Status line in 3ds/logs/perf/status_S4b.txt before each Azahar start. Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. No toggles (debug switches allowed), no stubs, no git push, no console push, no attribution. Game source only inside `#ifdef TARGET_3DS`. No vertex shader change. A design question: plan default, write it in agent_s4.md, continue. Update agent_s4.md STATE after each commit. Hand back only after S4-4 (all three rows), with the sprcheck and fallback lines.
