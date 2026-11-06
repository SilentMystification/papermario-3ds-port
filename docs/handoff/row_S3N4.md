# Rows S3N-4 and S3N-5: map models draw natively, then the dead walker model state code goes

## Goal
Switch: a model record draws through nd_model_draw (emit 1). The walker runs a record only when nd_model_draw refuses it (fallback, counted). Then delete what no path uses. This is the first content type the walker no longer interprets. Design: docs/handoff/plan_s3n_native_models.md (sections Fallback and Walker state after a native model).

## Start state
Branch nds1. Last commit cca2056fb. ndmcheck at the 3 spots: diff 0 (eq 11532 / 10222 / 8595, wref 0 / 213 / 1170, fb about 3000 to 5200). nd_model_draw(rec, ctx, list, emit) exists in 3ds/src/nd_model.c; its emit 1 path (pm_gpu_flush, then nd_native_submit per merged range) is written, not called. Read the STATE block of docs/handoff/agent_s3n.md first.

## S3N-4 steps (one commit each)
1. Switch in the G_PM_REC arm (pm_gbi.c grep `case G_PM_REC`): call nd_model_draw(..., emit 1). On success the record's walker STATE work (rec_apply: bind, combine, render mode list, fog, lights, prim/env/blend, shroud) does not run. The MATRIX work still runs (load_mtx R1/R2, billboard R8, pop R12 w1), so the nd_mv stack stays right for the walker draws that follow (plan default: the stack is shared). On refusal: rec_run as today. Counter line every 300 frames: `ndm f<n> native <k> fb <f> wref <w>` (pm_log_prof_line).
2. wref ranges: the walker drew them by the ring path or not at all. Plan default: native draws them too (the static range is valid). The picture decides: if the 3 pictures show an object that the old build did not draw, or a wrong one, make wref a fallback (the record runs on the walker) and note it.
3. Walker state after a native model: mark the walker state the record would have set as stale, so the next walker op sets it again (tb_last = NULL; clear the g2_same shadows of the ops rec_apply runs; sc_ok = 0 for the combine memo; nd_leave_native is already called by flush_draw before a citro3d draw).
4. Pictures: `python 3ds/tools/bench.py run spot_mac01_pic --row S3N --why "native models"`, then spot_mac00_pic, spot_sbk56_pic. View each PNG. Right by eye is a pass (one-frame small diffs are a pass, note them). A whole object missing, extra, wrong texture or wrong blend is a fail: name it, fix (max 2 tries), or move its class to fallback.

## S3N-5 (one commit)
Delete code no path reaches after S3N-4 for records that draw natively: only branches of rec_apply/rec_run with no caller left (keep the fallback path whole while fb > 0). Keep ndmcheck and combchk as debug tools. Host nd_host passes, build passes, spot_mac01_pic right by eye.

## Touch only
3ds/src/pm_gbi.c, 3ds/src/pm_gpu.c, 3ds/src/nd_model.c, 3ds/include/nd.h, docs/handoff/agent_s3n.md.

## Rules
`git status --short` before each edit and commit. Pathspec commits only (`git commit -m ... -- <paths>`). Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore` (except your own backup of map_restore_table.c). Do not edit azt_scenarios.json, bench_defs.json, runs.jsonl. No toggles (the fallback is a bounded bridge path, not a switch), no stubs, no git push, no attribution. Never commit 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). A blocker: take the plan default, write it in agent_s3n.md, continue.

## Commands
Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S3N sh 3ds/build.sh; restore map_restore_table.c. Status line in 3ds/logs/perf/status_S3N.txt before each Azahar start. Budget reset first: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget S3N.

## Done
S3N-4 steps 1 to 4 and S3N-5 committed. 3 spot pictures right by eye. `ndm` line shows native above 0 at the 3 spots. Update agent_s3n.md STATE after each commit. The orchestrator then pushes to the console.
