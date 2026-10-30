# agent_cap2 (row CAP2, diagnostic logging, no draw change)

## State
- Code done: pm_pmu_sp0() in pm_phase.c (three GET_VALUE reads on CORE_COUNT_REG_0, REG_1 and CORE_CYCLE_COUNTER, divided by pmu_n). sp0_report() in pm_gbi.c calls it once per sp0c window and prints `pmu_ic`, `pmu_dc`, `pmu_cyc` at the end of the sp0c line.
- Build OK. No warning on the changed lines.
- Test: 1 Azahar start (budget 3, 2 left). Scenario mac01t_cap1, PM_ROW=CAP2. Log line: `sp0c f483 tex 1751 dlc 891 rest 976 flush 394 skip 27773 grp 34513 tl 5948 pmu_ic 0 pmu_dc 0 pmu_cyc 0`. Log checks 6 of 6.

## Commit
- Files: 3ds/src/pm_gbi.c, 3ds/src/pm_phase.c, docs/handoff/agent_cap2.md.
- map_restore_table.c not committed.

## Choices and limits
- The counters are read at the sp0c window end only. No svc call per op or per group. Three svc calls per 300 frames, phase on and pmu on.
- The counters reset at the pmu window switch (pmu_set_window), on a 300-frame cadence that is not aligned to the sp0c window. So each field is the per-frame count since the last pmu reset (0 to 299 frames). It is not an exact sp0c window count.
- The counters cover the whole core, not only G_PM_MODEL. The read is at the window boundary, so no per-group split is made.
- pmu window A (pmu_win 0) counts INST_CACHE_MISS and DATA_CACHE_READ_MISS. Window B (pmu_win 1) counts MAIN_TLB_MISS and STALL_BY_LACK_OF_INST. In window B, `pmu_ic` and `pmu_dc` hold the TLB and stall counts. The field names do not change.
- With pmu off (no `pmu` switch), the fields print 0. The mac01t_cap1 scenario has no `pmu` switch, so this run did not read the counters. The console push with `pmu` reads them.

## Open items
- map_restore_table.c: the generator ran before the user copy was saved. The user copy (git status M at session start) is not on disk now. The file equals HEAD. The stash@{0} copy and 3ds/src/map_restore_table.c.bak also equal HEAD. The user must restore the copy from their own source.
- The picture of 0000_1_2 (frame 500) was not read by the agent. The script does not check the picture.

## Next steps (orchestrator decides)
- Console push with pmu and phase: `PM_ROW=PUSH sh .../push_3ds.sh --no-build --ip 192.168.1.170 phase pmu loglevel=debug`. Read the sp0c pmu fields, with the window A and B split in mind.

## Commands
- python 3ds/tools/gen_map_restore.py; sh 3ds/build.sh (save the user map file first, outside the repo).
- Test: PM_ROW=CAP2 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 mac01t_cap1 -Row CAP2 -Try
