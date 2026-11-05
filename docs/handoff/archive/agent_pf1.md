# agent_pf1: row PF1 (prof switch, clean-frame profile line)

## State
- PF1 done. Commit 6656d5870 (pm_port.h, pm_phase.c, pm_main.c).
- Build: sh 3ds/build.sh OK. No warning line names pm_phase.c, pm_main.c or pm_port.h.
- No Azahar start (plan: PF1 checked by build only).
- map_restore_table.c: regenerated for the build, then restored byte-equal. Not committed.

## What the code does
- Switch `prof` or `prof=<N>` (N 4 to 255, else 30). `profstall` sets set `s` in the cal line only (PMU is PF3).
- `pf_init` runs in `pm_ph_init` after `pmu_init`. `pm_pf_on = want && pm_log_file()`. `pm_ph_log_gate` sets `pm_pf_on = on && pf_want`.
- `pm_pf_frame(step_start, now)` runs in pm_main.c after `frame_ms` when `!last_step_skipped`.
- Window: 300 drawn frames. Line prefix `prof f... w... fps ...`. Sampled fields and pmu fields print 0.
- Cal line once at init: `prof cal every N set c tick X us` (200 pm_ticks reads, cold).

## Open items
- PF2 fills s, fts, wks, oh, amort, gbi, dlc, tex, gpu. PF3 fills pmu.
- The init cal read uses svc (boot only). The frame path has no new svc call.

## Next
- PF2 (agent_pf2 or the same epic agent): pm_ph_on arm and disarm, divert in pm_ph_frame_end.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; sh 3ds/build.sh; restore map_restore_table.c.
