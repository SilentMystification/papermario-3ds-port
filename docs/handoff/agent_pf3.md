# agent_pf3: row PF3 (PMU counters at the sample edges, scenario mac01t_prof)

## State
- PF3 done. Build OK (sh 3ds/build.sh), no warning on pm_phase.c lines.
- Azahar: 1 start (budget 3, 2 left). PM_ROW=PF3, `mac01t_prof -Try`. Log checks 5 of 5. Shot 0000_1_2 (frame 250) read by eye: Mario and a Toad on the Toad Town street.
- map_restore_table.c: saved to the scratchpad, generator run, file restored (same SHA1). Not committed.

## What the code does
- pf_init (pm_phase.c): pm_pmu_on = 0 when prof is on. ENABLE, SET_EVENT (set m: PMN0 CORE_INST_CACHE_MISS; set s: PMN0 CORE_STALL_BY_LACK_OF_INST; PMN1 CORE_DATA_CACHE_READ_MISS), SET_VALUE 0 on PMN0, PMN1, CCNT. Cal line adds pmu us (50 GET_VALUE reads of PMN0) and base us = 6 x pmu / N.
- pf_pmu_read: one svc per register (3 per call). Results in pf_base (at the sample end) and in the clean sums (at arm).
- Clean interval = [sample end, next arm]. At arm, u32 deltas go into pf_pe[0..2] (u64 sums), and pf_run (clean frames since the last sample end) goes into pf_pfr.
- Window line: pmu fields = pf_pe / pf_pfr (per clean frame), cyc = CCNT per clean frame, pm = e x 1000 / cyc (permille). Window sums reset at the report. pf_run and pf_base carry over.
- Counters are never reset after init. Per sample: 2 edges, 6 svc calls. The sampled frame's own counts are not summed.

## Test result
- prof cal: `prof cal every 16 set m tick 1.35 us pmu 0.11 us base 0.04 us rc 00000000`.
- prof line: `prof f300 w300 fps 28.6 | ft 35.44 max 1121.8 o34 7 wk 10.56 | s18 fts 29.91 wks 12.08 oh 1.52 amort 94 | gbi 7.66 dlc 1.38 tex 0.02 gpu 0.30 | pmu m 0 0 cyc 0 pm 1000 1000 | st 293 vb 629 iv 46 248 0 6`.
- Azahar PMU: the counts are tiny (cyc 0 per clean frame). pm 1000 1000 is a ratio of small numbers. Azahar does not give PMU data. Console numbers are needed (push with prof).

## Open items
- Console check after the epic: `PM_ROW=PUSH sh .../push_3ds.sh --no-build --ip 192.168.1.170 prof` (rule 6, after PF3 is committed). Read cal, fps, ft, oh, amort, pmu m.
- Second push with `prof profstall` for the stall share (set s).
- max 1121.8 ms in the window is one frame (boot or load). Not checked.
- Edge: a skipped step between arm and the sampled draw extends the sample wall.

## Commands
- python 3ds/tools/gen_map_restore.py; sh 3ds/build.sh; restore map_restore_table.c from a saved copy.
- Test: PM_ROW=PF3 powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 mac01t_prof -Row PF3 -Try
