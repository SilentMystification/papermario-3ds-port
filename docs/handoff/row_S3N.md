# Epic S3N: map model records draw natively, not through the walker

## Goal
Today a model record (G_PM_REC, 14 words) is run by the walker (pm_gbi.c rec_apply, rec_run) into walker state, and flush_draw (pm_gpu.c) closes the batch, then nd_raw_native writes words. Goal: a record goes straight to native draws: template (nd_tmpl), texture unit words (nd_tex), matrix (nd_mtx), shape ranges (nd_dlc NdObj in the static pools), written by nd_emit_draw (nd_emit.c). The walker state and flush_draw are not used for map models. This is the real walker replacement for models (plan_native_system.md sections 2 and 6).

## Start state
Branch nds1. Last commit 5fc6fb083. Tree: user files only. Read the STATE blocks of docs/handoff/agent_s3g.md and agent_s3f.md first (record layout R0 to R13).
Facts: nd_raw_native (pm_gpu.c grep `static int nd_raw_native(void)`) already builds a template key and calls nd_tmpl_build and nd_emit_draw for about 90 percent of Toad Town batches. nd_dlc compiles shape lists at map load (pm_map.c). nd_emit has no native record kind (nd_core.c nd_emit: LEGACY only).

## Rows (in order, commit after each part)
0. S3N-0 delete the dead old emission under recSkip and REC_SKIP in appendGfx_model (list in agent_s3g.md). No toggles rule.
1. S3N-1 compile once per map: each ModelRenderModes list to a state delta (nd_st fields: othermode, geometry, combine), each texture handle bind list to an nd_tex descriptor (plan nd_tex_handle_set, model.c load_texture_impl). Cold code, new file 3ds/src/nd_model.c.
2. S3N-2 nd_model_draw(rec): build the template key from the record and the compiled deltas, the tex block, the matrix (nd_mtx_pcomp(post, game), seam rule 1), the light block. One NdDraw per NdObj segment. Emit MERGED ranges: adjacent segments with an equal key make one draw (the grange rule, pm_gpu.c grep `grange_batch_open`). nds1_status_2026-11-05.md finding 3: per-batch replay doubled draws.
3. S3N-3 the record arm calls nd_model_draw instead of rec_apply. Cases nd_model_draw cannot draw (state it does not know yet): keep rec_apply for that record and count it (`ndm fallback <n>` in the prof line or a 300-frame log line). Target: fallback 0 in the 3 spots.
4. S3N-4 delete rec_apply paths that the count shows unused.

## Proof (small)
- Host: nd_host passes. Add a host case: for the ndref kind 6 raw draws of Toad Town (3ds/tests/ndref), nd_model_draw words equal nd_raw_native words (pattern: nd_emit_test).
- Pictures: bench.py run spot_mac01_pic, spot_mac00_pic, spot_sbk56_pic after S3N-3. Right by eye is a pass. A small one-frame diff is a pass: note it, do not chase it.
- The prof `dr` field (draws) at mac_01 not above 98 (build 2146 value) is noted, not a gate.

## Blockers are work
Pick the plan default, write it in docs/handoff/agent_s3n.md, continue. Split a large row, commit each piece. A record class nd_model_draw cannot draw yet: fallback plus count, continue.

## Touch only
src/model.c (TARGET_3DS), 3ds/src/nd_model.c (new), 3ds/src/nd_core.c, 3ds/src/nd_emit.c, 3ds/src/nd_tmpl.c, 3ds/src/nd_tex.c, 3ds/src/nd_mtx.c, 3ds/src/nd_dlc.c, 3ds/include/nd.h, 3ds/src/pm_gbi.c, 3ds/src/pm_gpu.c, 3ds/src/pm_map.c, 3ds/CMakeLists.txt, 3ds/tools/nd_host_test.sh and its test .c files. Not: azt_scenarios.json, bench_defs.json, runs.jsonl, other docs. Read: plan_native_system.md sections 2, 3, 4, 13.

## Collision rules
Another session works in its own worktree. Run `git status --short` before each edit and commit. A file of yours with a change that is not yours: do not edit it, report. Pathspec commits only. Never git add -A, commit -a, stash, reset, checkout or restore (except your own backup of map_restore_table.c).

## Done criteria
Rows 0 to 4 committed. Fallback count 0 in the 3 spots, or each left class named in agent_s3n.md with the reason. Spot pictures right by eye.

## Rules and commands
CLAUDE.md rules 5, 7, 9. No toggles, no stubs, no git push, no attribution. Never commit 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). hw_guidelines.md (no division, no double, 32-byte records). Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S3N sh 3ds/build.sh; restore map_restore_table.c. Status line in 3ds/logs/perf/status_S3N.txt before each Azahar start. Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget S3N.

## At the end
docs/handoff/agent_s3n.md STATE block (10 lines or fewer), updated after each commit.
