# Row TEXSTALE: native draws bind a stale texture slot (dro_01 paths and fences show Mario's walk frames)

## Facts
User, O3DS, build b09ac893c: in dro_01 the paths and fences sometimes show Mario's walking animation while Mario moves. Read docs/handoff/diag_stale_tex.md (commit 28bd9baa7) in full: the native lookup (pm_tex.c pm_tex_slot_find ~L1693, nd_tex.c nd_tex_unit_hint ~L96) matches a slot by hash, fmt, siz, dim only, with no slot_valid, no dirty, no palette (phash), no img/tlut check that the walker does (pm_tex.c ~L1539); nd_tex_get (nd_tex.c ~L63) never re-reads the source bytes.

## Steps (one commit each)
1. CHECK: debug switch `texstale`: at each native unit bind (the model cache hit path and the native sprite path, where nd_tex_unit / pm_tex_slot_unit gives the words) compare the slot with the draw's desc: slot live, not dirty, same img, same tlut/palette, and the current source hash vs the desc key. Count mismatches per 300 frames: `texstale f<n> binds <b> bad <k> dirty <d> img <i> pal <p> hash <h>`, plus 10 sample lines (slot, desc img, slot img). Run dro_01 with a walk (padscript, warp=dro_01:0, 25 s): bad above 0 proves the cause.
2. FIX (~25 lines): `pm_tex_slot_clean(i, img, tlut)` (live, not dirty, same img and tlut), required in pm_tex_slot_find and nd_tex_unit_hint; on a miss load through pm_tex_load (the walker's load) and refresh the desc key; force = 1 on the unit when the slot generation changed (pm_tex_slot_unit). Gate: the same dro_01 walk: texstale bad 0; top shots during the walk show paths and fences with their own texture; spot_mac01_ndmcheck still diff 0.

## Where
Main folder, branch nds1, build lane tex: `python 3ds/tools/gen_map_restore.py; PM_ROW=TEXSTALE PM_LANE=tex sh 3ds/build.sh`; restore 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e, never commit). Azahar: `powershell -NoProfile -File 3ds/tools/azt.ps1 <scenario> -Row TEXSTALE -Build tex` or run_azahar.ps1 with the build path of lane tex. Touch only 3ds/src/pm_tex.c, 3ds/src/nd_tex.c, 3ds/include/nd.h (one prototype), and the debug hook call sites in pm_gpu.c and nd_sprite.c only if the check needs them (one line each). Other agents work in this folder (S6: entities, MENU3: menu and warp): do not edit their files, keep edits buildable, commit quickly, pathspec commits, own hunks only. If git says index.lock exists, wait and retry.

## Rules
Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. No toggles (debug switch allowed), no stubs, no git push, no console push, no attribution. Budget reset allowed. Status line in 3ds/logs/perf/status_TEXSTALE.txt before each start. Hand back after step 2 with the texstale lines before and after.
