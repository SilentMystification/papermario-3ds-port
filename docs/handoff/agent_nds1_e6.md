# agent_nds1_e6: handoff (branch nds1)

## State
- nds1 = native merged in (commit 774f64e27). Native is not merged back.
- E5-3 retry FAILED (valid gate). census_mac01 run 1 with -Try: 71 batches / 810 tris / 53 distinct. Gate 64 / 810 / 52.
  The merge of adjacent learn draws in dlc_learn_end did not lower the batch count. Code reverted.
  Patch kept: docs/handoff/nds1_e53_merge_wip.patch. Not applied.
- Earlier E5-3 patch (docs/handoff/nds1_e53_wip.patch): see diag_e53_batches.md. Not applied.
- PROF4 (profiler toggle): code in pm_log.c, pm_main.c, pm_phase.c. Scenario prof_boot added to azt_scenarios.json. Build and Azahar run: see status_PROF4.txt.
- Not started: slice 1 (static models on Toad Town), slice 2.

## Open items
- E5-3 batch count. Diagnosis for a fresh agent: find which learned draws do not merge (log idx_off and nidx of ln_tmp after merge, versus the live grange count for the same group).
- Options.ini prof key is read in pm_main.c (prof=1 line). pm_options.c has no prof key, so options panel and options.ini write do not carry prof.
- Runtime button toggle for prof: not built.

## Commands
- Build: `/c/Python314/python 3ds/tools/gen_map_restore.py`, `PM_LANE=nds1 PM_ROW=<row> sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from the user copy. Never git checkout that file.
- Census: `PM_ROW=NDS1 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 census_mac01 -Row NDS1 -Build nds1 -Try`.
- Prof: `PM_ROW=PROF4 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 prof_boot -Row PROF4 -Build nds1 -Try`.
