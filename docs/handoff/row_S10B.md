# Row S10B: S10-2 BRIDGE (nd_core.c record path deleted)

## Goal
gbi_task walks the list directly with pm_gbi_run. The one-record ND_LEGACY bridge in nd_core.c is deleted. Pixels: none on the main path.

## Start state
Worktree build3ds/wt/s10b, branch s10b from nds1 (after the RS3 merge fa6406ee7 and this row file). Copy assets/us and ver/us/build into it. Row GLOB runs in parallel (pm_gbi.c, nd_cen.c, nd_walk.c): do not edit those files.

## Oracle plan (2026-11-07)
Premise: nd_core.c makes exactly one ND_LEGACY record per task (pm_nusys.c gbi_task: nd_bridge_cut, nd_emit, nd_emit again for eye 2). nd_order and nd_eye_copy have no callers outside nd_core.c.
Check first: `grep -rn "nd_order\|nd_eye_copy" 3ds/src | grep -v nd_core.c` is empty, and gbi_task (pm_nusys.c ~76-86) matches the premise. Otherwise stop and report.
1. gbi_task: replace the 3 calls with pm_gbi_run(gfxList, n), and under stereo a second pm_gbi_run between pm_gpu_eye(1) and pm_gpu_eye(0) (same eye order as now).
2. Keep the pm_pf_nd_add timing: wrap the walk with the tick read and pass legacy 1, so the phase lines keep their format.
3. nd_core.c: delete nd_rec, nd_n, nd_legacy, nd_bridge_cut, nd_emit, nd_field, nd_pass, nd_order, nd_eye_copy. Keep nd_frame_begin and nd_st_log. Delete ND_LEGACY, NdRec and their declarations in nd.h.
4. Update or delete the host tools that test the deleted code: 3ds/tools/nd_core_test.c, nd_emit_test.c, nd_size_stub.c, nd_size_check.py, and the nd_host_test.sh line that builds them. No stubs.

## Gate
build; nd_host PASS; grep proof (no nd_emit, nd_bridge_cut, ND_LEGACY in 3ds/src, 3ds/include, 3ds/tools); byte gates (cold det, cmp against base shots you take at the start commit on lane s10b): spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015, pause_spin_grab_det_w300. Plus one stereo run (yard_stereo_l or the stereo bench bench.py list shows) equal to base; if that scene is not det, compare by eye and say so.

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. PM_ROW=S10B PM_LANE=s10b on every build.sh and bench call. Budget 8 starts, reset when needed. No git stash. Never commit runs.jsonl or 3ds/src/map_restore_table.c. Wait for runs in the foreground.
Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S10B PM_LANE=s10b sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.

## At the end
One commit; it deletes this row file. Report in 5 lines.
