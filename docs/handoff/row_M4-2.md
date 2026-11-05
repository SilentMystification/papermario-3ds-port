# Row M4-2: producer record for plain opaque map models

## Goal
appendGfx_model writes one native record per plain opaque model (texture handle index, render mode index, lights group, matrix, shape list) instead of the texture bind words, render mode list words and light words. A consumer in the walker sets the walker state directly from the record. This removes 5 to 8 texture words, the combine sync and the light words per model (the walker spends 211 us per plain model on the console, the close only 37 us).

## Start state
Branch: nds1. Last commit: d01f8ea62 (docs). Tree: user files 3ds/src/map_restore_table.c and 3ds/logs/perf/pushed_builds.txt (never commit the first).
Console build 2099 (b729b930b): textures mostly OK while moving, no hang. M4 step 1 (fused bind lists, tb_ops, rm_apply_impl) is in.

## Open before you start (blockers are work)
- "Plain opaque" is not defined in code yet: define it in the row from appendGfx_model (SURFACE_OPA render mode, no local vertex copy, no custom gfx, no fog/tint/shroud, one texture unit). Every other model keeps the old words.
- The consumer needs the end state of a bind list: route: call the existing fused op functions (tb_ops) with the handle list, not a memo. A memo of the bind result was tried and reverted (no gain, hang). Do not build a memo.

## Touch only
- Files: src/model.c (TARGET_3DS blocks only), 3ds/src/pm_gbi.c, 3ds/include/gbi_custom.h, 3ds/tools/bench_defs.json, 3ds/tools/azt_scenarios.json.
- Symbols (grep, a line shift is not a stop): appendGfx_model, MODEL_TEX_OP, MODEL_RM_OP, mgHdr in src/model.c. tb_run, tb_ops, tb_diff, rm_apply, rm_apply_impl, ts_save, ts_load, TS_FIELDS, `case G_PM_MODEL` in 3ds/src/pm_gbi.c.
- Read: docs/handoff/handoff_native_epic_orchestrator.md sections 3 (item 1), 4, 5. docs/handoff/plan_native_system.md section 13 (seam rules) only.

## Steps
1. Proof first: a debug switch `mdldiff` (pattern of texdiff: tb_diff, ts_save, ts_load). For each plain opaque model it runs the old words and the record consumer from the same start state and compares TS_FIELDS plus geometry mode, othermode, combine, lights and the matrix stack top. Log `mdldiff f<n> runs <r> diffs <d>` every 500 runs. Add scenario mac01_mdldiff (cap 45 s or less, until-line the mdldiff log line) and bench `mdldiff` in bench_defs.json in the same commit.
2. Record write in appendGfx_model under TARGET_3DS for plain opaque models. The old texture, render mode and light words are not written for these models (no switch selects the old path).
3. Consumer in pm_gbi.c (a new G_PM op or an extension of the G_PM_MODEL header; pick one, state why in the commit).

## Done criteria (all must pass; handback only then)
1. Host tests: `python 3ds/tools/bench.py run nd_host --row M4-2 --why "host tests after the record"`: all pass.
2. `python 3ds/tools/bench.py run mdldiff --row M4-2 --why "record consumer state equals old words"`: runs 6000 or more, diffs 0.
3. `python 3ds/tools/bench.py run seam_pic --row M4-2 --why "picture of Toad Town with records"`: view the PNG and region_diff against the shot of d01f8ea62. Only scattered 1-pixel diffs. The log shows the record count per frame above 0.
4. `python 3ds/tools/bench.py run texdiff --row M4-2 --why "bind lists still equal"`: diffs 0.
Affected scenarios (no others): mac01_mdldiff, seam_pic, texdiff.
The console prof run (bench toad_standing) is done by the orchestrator after handback. Do not push.

## Rules that apply
CLAUDE.md rules 5, 7, 9. No toggles (a debug diff switch is allowed), no stubs, no git push, no attribution. Commit after each passed step. Never commit 3ds/src/map_restore_table.c. Source files are CRLF. hw_guidelines.md: no division, no double in the consumer.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=M4-2 sh 3ds/build.sh; then restore 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e).
- Status line before each Azahar start: 3ds/logs/perf/status_M4-2.txt.
- Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget M4-2

## At the end
Write docs/handoff/agent_m4.md with a STATE block (10 lines or fewer): branch, last commit, done steps, open items, next command.
