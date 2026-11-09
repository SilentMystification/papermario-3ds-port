# Row PMU2: cache and cycle counters for the logic thread on core 1

## Goal
The prof `pmu` line counts core 0 only (pm_phase.c:580 reads the counters on the game thread). The `core2` line (pm_phase.c:568) has
wall time only: lgms 25 ms on the O3DS XL in mac_00 for a step of about 3 ms (core 1 also runs audio, the log writer and the net send).
Add the core 1 counters: I-cache misses, D-cache misses and cycles per logic step, so the console shows the real logic work on core 1.

## Start state
Worktree build3ds/wt/PMU2, branch PMU2 from main 346c929d5. Copy assets/us and ver/us/build into it.

## Premise and its check
Premise: svcControlPerformanceCounter(PERFCOUNTEROP_GET_VALUE, CORE_COUNT_REG_0/1, CORE_CYCLE_COUNTER) returns the counters of the core
the calling thread runs on, and pf_start (pm_phase.c:647) set the events on that core too. Check first (code read plus the first console
data): read libctru svc.h and 3dbrew notes on the CORE registers. If the counters are global or core-0 only, report that in the status
file and use the cycle counter only if it is per core. Azahar may return 0 for all PMU reads: that is not a fail on Azahar.

## Touch only
Files: 3ds/src/pm_logic.c (logic thread loop, near line 61: `lg_pf_t += svcGetSystemTick() - t0`; pm_lg_pf at line 156),
3ds/src/pm_phase.c (pf_report core2 line, lines 560-570; pf_start 647). No other files.

## Plan
1. In the logic thread loop: read the 3 counters (one svc per register) before and after each non-idle command only when pm_pf_on
   is set (prof mode). Add the u32 deltas to 3 u64 sums next to lg_pf_t. No reads when prof is off (zero cost).
2. Extend pm_lg_pf to return the 3 sums. In pf_report print per logic step: `core2 ... ic1 %u dc1 %u cyc1 %u` (append to the
   existing core2 line, keep its old fields and order). Integer math only, no double, no division in the hot loop (divide once per window).
3. If the per-step svc cost matters: the prof cal line (pm_phase.c:667) gives pmu us per read. 6 reads per step at about that cost is
   allowed in prof mode only. Note the cost in the commit message.

## Done criteria
1. nd_host PASS: python 3ds/tools/bench.py run nd_host --row PMU2 --why "core2 line fields".
2. One Azahar run of a bench that prints the prof core2 line (bench.py list; spot_mac01_prof or its Azahar twin): the core2 line
   prints ic1, dc1 and cyc1 (values may be 0 on Azahar). memcheck OK.
3. No draw change: no byte gates. Console values come from the user's next push.
Affected scenarios (no others): the one prof scenario above.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=PMU2 on every build.sh and bench call. No git stash. Commit after each passed step.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=PMU2 PM_LANE=PMU2 sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=PMU2 PM_LANE=PMU2 python 3ds/tools/bench.py run <bench> --row PMU2 --lane PMU2 --why "<text>"

## At the end
Delete this row file in the commit that completes the row. SendMessage main "PMU2 ready <hash>".
