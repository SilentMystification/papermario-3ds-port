# Epic S3L: the fallback map model records draw natively too (fallback to 0)

## Goal
About 25 map model records per frame still fall back to the walker (rec_apply plus walker geometry): console O3DS .170, mac_01, ndmt `apply 2.77 fb 0.79` ms per frame in Azahar, more on the console. Make nd_model_draw draw those classes, so every map model record skips the walker. Then the walker model state path can be deleted. Design: docs/handoff/plan_s3n_native_models.md (section Fallback) and docs/handoff/agent_s3n.md STATE (B4 to B10, the key word list, S3N-7 timer names).

## Start state
Branch nds1. Last commit 7a7886d0f. Console steady state (build 2225, 30 fps mode): 28.8 fps, 32.2 ms. Cache: new 0, ins 0, clr 0. 60 fps mode: 17.2 fps, 54.3 ms. Fallback reason codes exist: pm_gpu.c ndm_why[16] (grep `S3N-3b: fallback draws by reason code`), printed only by ndmcheck today.

## Steps (one commit each)
1. COUNT. Print the fallback reason codes every 300 frames in normal mode too (append ` why <c>:<n> ...` for nonzero codes to the ndm line), with the record count per code. One spot_mac01_pic, spot_mac00_pic and spot_sbk56_pic run. Write the table (code, meaning, records per frame, at which spot) in agent_s3n.md. Known classes: lit (pm5 program, g_use5, nd_raw_native refuses it), textured handle without a desc (why 9), list not static (why 12), two texture units or noise, custom gfx and aux words.
2. LARGEST CLASS FIRST. For the code with the most records: make nd_model_state and nd_native_submit draw it. Lit: pm5 program in nd_emit_draw (the light block uniforms from nd_lights, nd_mtx.c and pm5.v.pica layout, the normal matrix rows; compare with the words citro3d writes for pm5 through ndmcheck). Without desc: extend the B4 desc to the handle form it lacks. Not static: compile the list with nd_dlc at its first use (plan 2.9: once per list pointer and pm_dlc_gen). Gate per class: ndmcheck diff 0 at the 3 spots for that class, ndmcache diff 0, the class count drops, pictures right by eye.
3. Repeat step 2 for the next code until the fallback count per frame is 0 at the 3 spots, or a class is named in agent_s3n.md as not possible with the reason.
4. DELETE. When fallback is 0 at the 3 spots: delete the walker paths that only fallback records used (rec_apply branches, rec_dl for model geometry) if no other caller is left. Keep ndmcheck, combchk, ndmcache as debug tools.

## Touch only
3ds/src/nd_model.c, 3ds/src/nd_emit.c, 3ds/src/nd_tex.c, 3ds/src/nd_dlc.c, 3ds/src/nd_mtx.c, 3ds/include/nd.h, 3ds/src/pm_gbi.c, 3ds/src/pm_gpu.c, 3ds/src/pm_tex.c, 3ds/tools/nd_host_test.sh and its test .c files, docs/handoff/agent_s3n.md. Not: 3ds/shaders (no vertex shader change, seam rule), azt_scenarios.json, bench_defs.json, runs.jsonl.

## Rules
`git status --short` before each edit and commit. Pathspec commits only (`git commit -m ... -- <paths>`). Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. No toggles (debug switches allowed), no stubs, no git push, no console push (the orchestrator pushes), no attribution. Never commit 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). hw_guidelines.md. Static memory: the O3DS boot failed once at 26 MB needed / 24 MB free; log the size of any new table and keep the total static add under 256 KB. A blocker: take the plan default, write it in agent_s3n.md, continue.

## Commands
Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S3L sh 3ds/build.sh; restore map_restore_table.c. Status line in 3ds/logs/perf/status_S3L.txt before each Azahar start. Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget S3L. Host: python 3ds/tools/bench.py run nd_host --row S3L --why "...".

## Done
Fallback 0 per frame at the 3 spots (or each left class named with the reason), step 4 committed, nd_host PASS, pictures right by eye. Update agent_s3n.md STATE after each commit. Hand back after each class commit with the ndm and ndmt lines, so the orchestrator can push.
