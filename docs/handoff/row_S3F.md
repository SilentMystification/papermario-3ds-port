# Epic S3F: map models draw from records only, walker model path deleted

## Goal
Every map model (src/model.c appendGfx_model) writes producer records, no N64 words. Then the walker model path (G_PM_MODEL header, mg_run, G_PM_TEX, pm_dlc.inc.c) is deleted. This is slice S3 of the plan to replace the display list walker. User priority: reach the replacement. Per-row speed gains are not a gate.

## Start state
Branch nds1. Last commit 7a429f463. Tree: user files only. S3F-1 is DONE (eeba51266 matrix, dfbf5d131 lights). Start at S3F-2.
STATE from the previous agent:
- done: S3F-1a, S3F-1b (host PASS, mdldiff 0 diffs, seam frame 200 4 px; frames 201, 202 about 1 percent, largest region 519 px, world right by eye)
- not checked: matrix stack top is not in TS_FIELDS. Add it to the mdldiff compare in S3F-2 (small, a debug check).
- the record cannot carry yet: panner, billboard, custom gfx, local vertex copy, transform group member, 2CYC, fog, shroud, depth, remap tints (S3F-2 scope)
- pitfall: gSPMatrix stores params XOR G_MTX_PUSH; record words must equal what the macro writes.
- next: S3F-2 at `recOn = plain &&` in src/model.c, then mdldiff, seam_pic.

## Rows (in order, commit after each)
1. S3F-1 matrix and lights in the record. The record carries model->finalMtx (or the billboard matrix) and the lights group; the consumer does what the G_MTX and gSPSetLights* words did (pm_gbi.c G_MTX handler and gbi_movemem light path, same functions, no copy of their logic). Anchors: appendGfx_model grep `gSPMatrix((*gfxPos)++, model->finalMtx` and `gSPSetLights0((*gfxPos)++`. Extend mdldiff to compare the matrix stack top and the light block.
2. S3F-2 all render classes and modes. Widen `recOn` (grep `recOn = plain &&`) to 2CYC, FOG, SHROUD, DEPTH, REMAP tints and all render modes (alpha test, decal, translucent: ordered). Colours and fog go in the record as values. Panner, billboard, custom gfx, local vertex copy: plan_native_slices.md rows S3-3 and S3-4 items.
3. S3F-3 floor reflection and custom gfx ranges: plan_native_slices.md S3-5 item 1 (nd_range_* in execute_render_tasks) and S3-4 item 2.
4. S3F-4 delete: plan_native_slices.md S3-5 item 3 (G_PM_MODEL, mg_run, G_PM_TEX arms, tb_list, pm_texbind_set if no caller, pm_dlc.inc.c, model census, ld names). Keep G_PM_RM (sprites and entities still write it).

## Open before you start (blockers are work)
- Plan anchors may be stale: grep the symbol, continue at its line. Stop only if the symbol is gone.
- A model class the record cannot carry: write it in agent_s3f.md with the class and why, and keep going with the rest. Do not stop the epic on one class.

## Touch only
- Files: src/model.c (TARGET_3DS blocks), 3ds/src/pm_gbi.c, 3ds/src/pm_dlc.inc.c (delete), 3ds/src/nd_core.c, include/gbi_custom.h, 3ds/ld/3dsx.ld.in, 3ds/CMakeLists.txt.
- Read only: docs/handoff/plan_native_slices.md rows S3-3, S3-4, S3-5. docs/handoff/plan_native_system.md section 13 (seam rules). docs/handoff/agent_m4.md.

## Tests (small; user rule: no regression over-testing)
Per row: (a) `python 3ds/tools/bench.py run nd_host --row S3F --why "<row> host"` passes. (b) `python 3ds/tools/bench.py run mdldiff --row S3F --why "<row> record state"`: diffs 0 (rows 1 and 2). (c) one picture: seam_pic (Toad Town). View it. The world must draw right by eye. A small diff in one frame with a right picture is a pass: note it, do not chase it.
Row 4 only: also one picture at mac_00 (warp=mac_00:0 pos=-135:0:85) and sbk_56 (warp=sbk_56:0 pos=20:0:440) if a bench for these spots exists (bench.py list), else one adhoc azt.ps1 scenario per spot (cap 25 s, until-line a frame line the build prints, add to azt_scenarios.json, pathspec commit).
No console push (orchestrator does it at the epic end). No A/B runs.

## Done criteria
All 4 rows committed. `git grep -n "G_PM_MODEL\|mg_run\|pm_dlc.inc.c" 3ds/src include src` returns nothing. Pictures of the 3 spots draw right by eye.

## Rules
CLAUDE.md rules 5, 7, 9. No toggles (mdldiff debug switch allowed), no stubs, no git push, no attribution. Pathspec commits only (`git commit -m ... -- <paths>`): another session commits in this folder. Never commit 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). CRLF source files.

## Commands
Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S3F sh 3ds/build.sh; restore 3ds/src/map_restore_table.c. Status line in 3ds/logs/perf/status_S3F.txt before each Azahar start. Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget S3F.

## At the end
Write docs/handoff/agent_s3f.md with a STATE block (10 lines or fewer).
