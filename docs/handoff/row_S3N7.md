# Row S3N-7: measure the native model path on the console, then cut its top cost

## Goal
S3N-6 on the O3DS (192.168.1.170, mac_01, user walking): 17.5 fps, 52.6 ms, walker part `lg` 52.1 ms. On the O3DS XL before S3N (S3F): 23.5 fps, 39.4 ms. The native model draws run inside the walker time, so the prof line cannot split their cost. This row adds tick counters inside the model path, the orchestrator pushes once, then the row cuts the largest cost.

## Start state
Branch nds1. Last commit 250f60788. Read the STATE block of docs/handoff/agent_s3n.md first (S3N-6: cache, hit path, rec_matrix). Console counter line today: `ndm f<n> native <k> fb <f> wref <w> hit <h> miss <m> clears <c>` (cumulative). Console hit rate 73 percent (404214 hit, 152453 miss, 9 clears) while walking.

## Step 1 (one commit): tick counters
In prof mode (pm_phase.c sample frames, the svcGetSystemTick pattern of pm_pf_nd_add), sum ticks per 300 frames for: key build and lookup (pm_gpu_model_key, pm_gpu_model_hit), the hit-path range check (pm_dlc_native_ranges on a hit), the miss path (rec_apply + nd_model_state + range build + store), fallback rec_run, nd_native_submit (split: C3Di_UpdateContext, nd_emit_draw, GPUCMD_AddRawCommands), the matrix work (rec_matrix + pcomp). Print one line every 300 frames through pm_log_prof_line: `ndmt f<n> key <ms> rng <ms> miss <ms> fb <ms> upd <ms> emit <ms> add <ms> mtx <ms> draws <n>` (ms per frame, %.2f). Make the counters per window, not cumulative. Also make the `ndm` line per window. Gate: build, nd_host, one spot_mac01_pic run shows the ndmt line, picture right by eye.
Then STOP and hand back with the commit: the orchestrator pushes to the console and sends you the ndmt line.

## Step 2 (after the console numbers): cut the top cost
Work only on the largest ndmt field, with the plan rules (built once per map, no per-draw rebuild). Known suspects: the hit path rebuilds ranges every hit; the key compares 92 words; C3Di_UpdateContext runs per native draw; wref ranges (walker-refused, about 30 percent of native draws) cost draws the old build did not make. One commit, picture right by eye, then hand back for the next push.

## Touch only
3ds/src/pm_gpu.c, 3ds/src/pm_gbi.c, 3ds/src/nd_model.c, 3ds/src/pm_phase.c, 3ds/include/nd.h, docs/handoff/agent_s3n.md.

## Rules
`git status --short` before each edit and commit. Pathspec commits only (`git commit -m ... -- <paths>`). Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. Do not edit azt_scenarios.json, bench_defs.json, runs.jsonl. No toggles, no stubs, no git push, no console push (the orchestrator pushes), no attribution. Never commit 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). hw_guidelines.md: no division in hot code (scale ticks at print time only).

## Commands
Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S3N sh 3ds/build.sh; restore map_restore_table.c. Status line in 3ds/logs/perf/status_S3N.txt before each Azahar start. Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget S3N.
