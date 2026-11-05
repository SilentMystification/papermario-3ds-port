# Epic S3G: every map model from records, then sort and merge

## Goal
appendGfx_model (src/model.c) writes only producer records for every model class. Then records sort and merge (plan S3-5 item 2). This finishes slice S3 of the walker replacement. User priority: reach the replacement. Speed per row is not a gate.

## Start state
Branch nds1. Last commit 42b2642ee. Tree: user files only. S3F done: record (11 words, G_PM_REC_WORDS) carries texture list, render mode, combine, fog, tints, matrix, billboard, panner, lights, IGNORE_MATRIX. Walker model path deleted (c856da583). Read the STATE block of docs/handoff/agent_s3f.md first.
Open from S3F:
- custom gfx: the pre and post G_DL words stay plain words. The record must carry them (nested list run from rec_apply).
- floor reflection: nd_range_begin, nd_range_end, nd_range_repeat do not exist. Write them in 3ds/src/nd_core.c (plan_native_slices.md S3-5 item 1).
- local vertex copy, specialTex, untextured models: plain words through the walker.
- the mdldiff and texdiff switches are gone. The proof is now host test plus pictures.

## Rows (in order, commit after each part)
1. S3G-1 custom gfx in the record (plan_native_slices.md S3-4 item 2). Anchor: appendGfx_model grep `gCurrentCustomModelGfxPtr)[customGfxIndex]`.
2. S3G-2 local vertex copy, specialTex, untextured models in the record (S3-4 item 3). Anchor: grep `MODEL_FLAG_HAS_LOCAL_VERTEX_COPY`.
3. S3G-3 floor reflection with nd_range_* (S3-5 item 1). Anchor: execute_render_tasks grep `savedGfxPos`.
4. S3G-4 sort unordered runs by (tmpl, tex0, tex1) and merge adjacent draws with an equal key (S3-5 item 2). Only opaque depth-tested draws move (plan_native_system.md section 13 rule 6).

## Blockers are work
Pick the plan default, write it in docs/handoff/agent_s3g.md, continue. Split a large row, commit each piece. A plan anchor that is gone: grep the symbol, use its current line.

## Touch only
src/model.c (TARGET_3DS blocks), 3ds/src/pm_gbi.c, 3ds/src/nd_core.c, 3ds/include/nd.h, include/gbi_custom.h, 3ds/CMakeLists.txt. Read: plan_native_slices.md rows S3-4 and S3-5, plan_native_system.md section 13.

## Tests (small)
Per part: `python 3ds/tools/bench.py run nd_host --row S3G --why "<part> host"` passes, and one picture viewed by eye: `bench.py run spot_mac01_pic` (Toad Town). S3G-3 and S3G-4 also spot_mac00_pic and spot_sbk56_pic. Right by eye is a pass. A small diff in one frame is a pass: note it, do not chase it. No console push, no A/B runs. Do not edit azt_scenarios.json or bench_defs.json.

## Done criteria
All 4 rows committed. In appendGfx_model no gSP or gDP macro runs on the 3DS path except the record write (grep the TARGET_3DS path, list any left in agent_s3g.md with the reason). Spot pictures right by eye.

## Rules
CLAUDE.md rules 5, 7, 9. No toggles, no stubs, no git push, no attribution. Pathspec commits only (`git commit -m ... -- <paths>`): another session commits in this folder. Never commit 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). CRLF source files.

## Commands
Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S3G sh 3ds/build.sh; restore 3ds/src/map_restore_table.c. Status line in 3ds/logs/perf/status_S3G.txt before each Azahar start. Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget S3G.

## At the end
Write docs/handoff/agent_s3g.md with a STATE block (10 lines or fewer). Update it after each commit.
