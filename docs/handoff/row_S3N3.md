# Rows S3N-3a, 3b, 3c: native model draw state, check, geometry

## Goal
Build `nd_model_draw` (3ds/src/nd_model.c) in 3 small steps and prove each one. Design: docs/handoff/plan_s3n_native_models.md (read sections Design and Fallback only). Decisions B4 to B7: docs/handoff/agent_s3n.md (read its STATE and B-list only).

## Start state
Branch nds1. Last commit d13be1856. Done: nd_gv_params (moved gv_params_calc), nd_native_submit (pm_gpu.c), nd_tex_handle_set (model.c load_texture_impl), nd_mode_compile (render mode lists). The draw still goes through the walker.

## Rows (in order, one commit each)
3a. STATE RESOLVE. `int nd_model_state(const Gfx* rec, NdDraw* d, NdMState* st)` fills, from the record and the compiled tables only (no walker global): NdTmplKey -> nd_tmpl_build -> d->tmpl; TEV colours (nd_tev_build, B7: combine 8 from the R4 words via pm_comb_build); d->u[0] from nd_tex_get(handle desc) + nd_tex_block; uvso, cmul, cadd, zmix from nd_gv_params. Returns 0 (fallback) for the B4 to B7 cases. Host case in the nd_host test: a hand-made record of a plain 1CYC SURFACE_OPA textured model gives the key fields that nd_raw_native would set for that state (write the expected values in the test from the rules in nd_raw_native, grep `static int nd_raw_native(void)` in pm_gpu.c). Gate: build, nd_host PASS.
3b. CHECK. Debug switch `ndmcheck` (PMD("ndmcheck") pattern, as texdiff was). In the G_PM_REC arm (pm_gbi.c grep `case G_PM_REC`), when the switch is on: call nd_model_state into a static NdDraw `ndm_ref` and set `ndm_live = 1`; rec_run runs as today. In nd_raw_native (pm_gpu.c), when ndm_live: compare the NdDraw it built with ndm_ref field by field: tmpl key fields, tev_col, buf_col, u[0], uvso, cmul, cadd, zmix. Clear ndm_live at the end of the record's geometry. Count recs, eq, diff, fb, legacy (B7 class). Every 300 frames log `ndmcheck f<n> recs <r> eq <e> diff <d> fb <f> legacy <l>` plus up to 20 lines `ndmcheck diff <field> rec <ptr> nat <v> walk <v>`. Run: `powershell -File 3ds/tools/azt.ps1 spot_mac01_pic -Row S3N -Debug ndmcheck` (or the bench spot_mac01_ndmcheck if bench.py list shows it), then mac00 and sbk56. Gate: diff 0 (legacy and fb counted apart). A diff names a field: fix nd_model_state for that field (max 2 tries per field), rerun.
3c. GEOMETRY. `nd_model_draw(rec)`: nd_model_state, then the matrix (nd_mtx_load R1 R2, billboard R8, nd_mtx_combine, nd_mtx_pcomp(post, game) into d->pal[0], npal 1), then for the R12 shape list its DlcEnt (dlc_find) groups: per group batch range (DlcGrp sd, ends, bat_q) the index word of the static index pool and nidx; cull from the group geometry ops (gop) as dlc_static_group computes geom_at. Merge adjacent ranges with equal cull and contiguous indices into one draw. pm_gpu_flush() first, then nd_native_submit per draw. Not called yet (S3N-4 switches). Extend ndmcheck: compare d->pal[0] and the idx ranges too. Gate: ndmcheck diff 0 on the 3 spots.

## Touch only
3ds/src/nd_model.c, 3ds/include/nd.h, 3ds/src/pm_gbi.c (G_PM_REC arm, ndm hooks), 3ds/src/pm_gpu.c (nd_raw_native compare), 3ds/tools/nd_host_test.sh and its test .c, 3ds/CMakeLists.txt.

## Rules
Collision rules: `git status --short` before each edit and commit; a file of yours with a change not yours: do not edit, report. Pathspec commits only. Never git add -A, commit -a, stash, reset, checkout, restore (except your own backup of map_restore_table.c). Do not edit azt_scenarios.json, bench_defs.json, runs.jsonl. CLAUDE.md rules 5, 7, 9. No toggles (ndmcheck is a debug switch), no stubs, no git push, no attribution. Never commit 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). Ignore the build field of bench ingest (known bug). A blocker: take the plan default, write it in agent_s3n.md, continue.

## Commands
Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S3N sh 3ds/build.sh; restore map_restore_table.c. Status line in 3ds/logs/perf/status_S3N.txt before each Azahar start. Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget S3N.

## Done
3a, 3b, 3c committed, ndmcheck diff 0 on the 3 spots. Update agent_s3n.md STATE after each commit.
