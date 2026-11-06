# Row S3N-6: native model state cached per record, walker state work skipped (native-first)

## Goal
S3N-4 made map model geometry native, but the frame got slower on the console: mac_01 16.2 fps / 58.8 ms (S3F: 23.5 fps / 39.4 ms). Cause: per record and per frame, rec_apply still runs in full, nd_comb_decide reruns the walker combine work with save and restore, the slot is found again by content key, and the ranges are rebuilt. Plan section 2.9 says this state is compiled ONCE. This row: build the native state of a record once, keep it, and skip rec_apply on a hit.

## Start state
Branch nds1. Last commit c31545799. Read the STATE block of docs/handoff/agent_s3n.md first (B4 to B10, 3c, S3N-4). Code: nd_model.c (nd_model_state, nd_model_draw, nd_comb_native), pm_gpu.c (model_draw, pm_gpu_model_native, ndm_cmp), pm_gbi.c (rec_run, rec_apply, nd_comb_decide, pm_dlc_native_ranges).

## Steps (one commit each)
1. CACHE. `NdMCache` table in nd_model.c (open hash, 1024 entries, 32-byte aligned entries; no division, hw_guidelines.md). Key: record pointer, a hash of the 14 record words, nd_st om_h/om_l/geom base bits that B8/B9 read, pm_dlc_gen, the texture handle load generation. Entry: everything nd_model_state and the range build produce except the per-draw parts: tmpl id, TEV colours, buf colour, uvso/cmul/cadd/zmix, the unit 0 slot id plus its slot generation, the merged ranges (index word, count, cull per range, up to 16; more: fallback). On a miss: run today's code (rec_apply, then nd_model_state and the range build) and store. Reset at map load (pm_dlc_gen) and when the table is full (clear all, count it).
2. HIT PATH (native-first). On a hit: do NOT run rec_apply's state work. Run only the matrix work (load_mtx R1/R2, billboard R8, pop R12 w1, as S3N-4). Per draw: unit 0 words from the cached slot (pm_tex_slot_unit; if the slot generation changed: rebuild the entry), pal[0] = nd_mtx_pcomp(post, combined), then nd_native_submit per cached range. After a hit, mark the walker state stale (sc_ok = 0, sr_ok = 0, tb_last = NULL, the g2_same shadows of the ops rec_apply runs) so the next walker op sets it again.
3. CHECK. With switch ndmcheck on: on a hit ALSO compute fresh (the miss path, but discard) and compare with the cache entry field by field; log `ndmcache f<n> hit <h> miss <m> diff <d>` every 300 frames plus up to 20 diff lines. Gate: `python 3ds/tools/bench.py run spot_mac01_ndmcheck` (and mac00, sbk56): ndm diff 0 and ndmcache diff 0, hit above 90 percent of hit+miss.
4. PICTURES. spot_mac01_pic, spot_mac00_pic, spot_sbk56_pic: right by eye (view each PNG).
Also count the cost line: `ndm f<n> native <k> fb <f> wref <w> hit <h> miss <m>` every 300 frames.

## Touch only
3ds/src/nd_model.c, 3ds/include/nd.h, 3ds/src/pm_gbi.c, 3ds/src/pm_gpu.c, 3ds/src/pm_tex.c (slot generation getter only), docs/handoff/agent_s3n.md.

## Rules
`git status --short` before each edit and commit. Pathspec commits only (`git commit -m ... -- <paths>`). Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore` (except your own backup of map_restore_table.c). Do not run `bench.py build-at`. Do not edit azt_scenarios.json, bench_defs.json, runs.jsonl. No toggles (ndmcheck is a debug switch), no stubs, no git push, no attribution. Never commit 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). A blocker: take the plan default, write it in agent_s3n.md, continue.

## Commands
Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S3N sh 3ds/build.sh; restore map_restore_table.c. Status line in 3ds/logs/perf/status_S3N.txt before each Azahar start. Budget reset first: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget S3N.

## Done
Steps 1 to 4 committed, the gates pass. Update agent_s3n.md STATE after each commit. The orchestrator then pushes to the console (speed is judged there).
