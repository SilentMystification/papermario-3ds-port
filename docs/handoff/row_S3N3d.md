# Row S3N-3d: combine decide as one function with explicit inputs and outputs (B10)

## Goal
The native model draw needs the walker's combine choice (legacy mode or plan, TEV colours). Make it one call with explicit inputs and outputs: `nd_comb_decide(const NdCombIn*, NdCombOut*)`. The native path calls it with record values. Then the ndmcheck gate (3b) can compare real draws. Decision B10 in docs/handoff/agent_s3n.md (read its STATE and B-list only).

## Start state
Branch nds1. Last commit 25f2a5679. Code of 3a, 3b, B8, B9 is in. ndmcheck at the 3 spots: eq 0 (all legacy class). Budget S3N: reset first.

## What exists (pm_gbi.c, grep the symbols)
- `sync_combine` (grep `static void sync_combine(void) {`): its signature lists ALL inputs of the work: comb_w0, comb_w1, omode_h, prim, envc, aud_prim_lod, keyc, keys, conv_k4, leg_mode, tex_on, use_t1, td[1].valid, dual_on, box_prod, box_pat, box_folded, comb_set, comb_alpha_shade.
- `sync_combine_work` (sync_legacy, const_rgb_tex_alpha, decide, pm_comb_colours) does the work.
- `ScMemo`, `scm_store`, `scm_replay` (grep `scm_store`): the memo already captures ALL outputs of the work and replays them.

## Steps (one commit each)
1. NdCombIn = the signature inputs above. NdCombOut = the ScMemo content. In pm_gbi.c (it owns the globals) write `void nd_comb_decide(const NdCombIn* in, NdCombOut* out)`: save the input globals and the current outputs (ScMemo form), set the inputs from `in`, run sync_combine_work, capture the outputs into `out` (as scm_store does), then restore the saved inputs and outputs (scm_replay of the saved memo). Prototype in 3ds/include/nd.h. The walker path is unchanged.
   Check (no Azahar): a debug switch `combchk` in sync_combine: after the work, call nd_comb_decide with the current inputs and compare its out with the live outputs; log `combchk f<n> runs <r> diff <d>` every 300 frames. Gate: one run of spot_mac01_pic with combchk on (run_azahar.ps1 with the bench switches plus combchk, 25 s), diff 0.
2. nd_model_state uses nd_comb_decide: inputs from the record and the compiled tables: comb w0 w1 (list combine when set, else R4 entry, B9), omode_h (B8 merge), prim and env (R6), tex_on and use_t1 from the handle desc (B4), td[1].valid from the desc (second tile), box_prod = box_pat = box_folded = 0, dual_on = 0, comb_set = 1, leg_mode and comb_alpha_shade and aud_prim_lod, keyc, keys, conv_k4 from nd_st or the record, else fallback class `nost`. Remove the B7 legacy class: ndmcheck now compares legacy draws too.
3. Gate 3b: `python 3ds/tools/bench.py run spot_mac01_ndmcheck --row S3N --why "..."`, then spot_mac00_ndmcheck, spot_sbk56_ndmcheck: eq above 0, diff 0. A diff names a field: fix the input that feeds it (max 2 tries per field). If a run fails only on the prof scene-line event, read the ndmcheck line from the log and report it; do not edit the scenario.

## Touch only
3ds/src/pm_gbi.c, 3ds/src/nd_model.c, 3ds/include/nd.h, 3ds/src/pm_gpu.c (ndmcheck compare only), docs/handoff/agent_s3n.md.

## Rules
`git status --short` before each edit and commit. Pathspec commits only (`git commit -m ... -- <paths>`). Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore` (except your own backup of map_restore_table.c). Do not edit azt_scenarios.json, bench_defs.json, runs.jsonl. No toggles (combchk and ndmcheck are debug switches), no stubs, no git push, no attribution. Never commit 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e). A blocker: take the plan default, write it in agent_s3n.md, continue.

## Commands
Build: python 3ds/tools/gen_map_restore.py; PM_ROW=S3N sh 3ds/build.sh; restore map_restore_table.c. Status line in 3ds/logs/perf/status_S3N.txt before each Azahar start. Budget reset: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget S3N.

## Done
Steps 1 to 3 committed, ndmcheck eq above 0 and diff 0 on the 3 spots. Update agent_s3n.md STATE after each commit. Next row after this: 3c (row_S3N3.md).
