# Row S103: S10-3, hot code span after the walker refactor (Sonnet)

## Goal
3ds/ld/3dsx.ld.in places the hot functions first in .text (one span that fits the ARM11 L1I, docs/handoff/hw_guidelines.md). The current list (lines ~32-40) names functions that no longer exist or moved (gbi_run_body, flush_draw_cpu, C3D_DrawElements/DrawArrays, sh_*, .text.unlikely.gv_tri, ...). Rebuild the list from measured hot ops. Cold nd code (census, debug) builds with -Os. Pixels: layout only.

## Start state
Worktree build3ds/wt/s103, branch s103 from nds1 (after the S101 merge 93bd8c864 and this row file). Copy assets/us and ver/us/build into it.

## Premise and its check (oracle plan 2026-11-07)
Premise: the hot list is stale after TRI-4, FB-4, GM-3, LS-3, GLOB, S10-1, S10-2.
Check first (1 build, 0 starts): for every `.text.X` entry in 3dsx.ld.in, test whether symbol X exists (nm on the lane elf, or grep the map file). Report the count of names that resolve to no symbol. If 0 are stale, stop and report.

## Steps
1. Hot set from measurement, not guess: one wcen_mac01 run with prof=4 opsplit (and the gp_opt/sampled top functions if the build prints them) on the start commit. Candidate set from the oracle: nd_walk_run, pm_gbi_run, nd_geom_vtx, draw_tri, gv_tri, gv_epoch_enter, gv_params_calc, gtrip, gappend, nd_native_submit, nd_raw_native, flush_draw, tex_op, ensure_tile, load_mtx, sync_combine, sync_combine_work, dlc_try, pm_tex_bind, g2_same, t1_fold_tri. Keep only names that exist; order by call order in the walk.
2. Total size of the hot span: 16 KB or less (L1I). Report bytes from the map.
3. Delete the stale names. Keep the citro3d entries that still exist and are called per draw (C3Di_UpdateContext, C3D_TexBind, C3D_UpdateUniforms: check).
4. CMake: compile nd_cen.c (and other census/debug-only nd files you name) with -Os (set_source_files_properties). Do not put hot files under -Os.
5. nd_size_test.sh: nd_walk_run 8 KB or less.

## Gate
build; nd_host PASS; 0 stale names in 3dsx.ld.in; hot span bytes reported; byte gates (cold det, cmp against base shots you take first on lane s103): spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015, pause_spin_grab_det_w300; wcen_mac01 prof=4 gbi-phase us no worse than base (3 runs each side if the first pair differs by more than 3%). Azahar shows direction only: the console prof push is a later step that needs the user (orchestrator asks).

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. PM_ROW=S103 PM_LANE=s103 on every build.sh and bench call. Budget 8 starts, reset when needed. No git stash. Never commit runs.jsonl or 3ds/src/map_restore_table.c. Wait for runs in the foreground. One commit; it deletes this row file. 2 tries per named cause.
