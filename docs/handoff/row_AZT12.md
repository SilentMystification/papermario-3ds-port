# Row AZT12: Azahar test throughput (user, 2026-11-08)

## User facts
User: "Is there still a bottleneck for azahar testing? I feel like we might not be using the testing harness quickly to prevent collisions and test frequently and quickly."
Orchestrator data, 2026-11-08 (runs.jsonl, build3ds/azslots/pool.log):
- 105 Azahar runs: wall median 23.0 s, p90 42 s, max 76 s; until-line median 21.9 s (so most wall time is the game reaching its until-line).
- pool.log, 911 events: 312 "start cold: option", 15 cold turbo, 11 cold no kept, 5 cold switch mismatch, 5 cold restart failed; 320 "kill orphan of a killed run"; 11 "stale run booted". Warm restarts are rare.
- "option" = run_azahar.ps1:223 forces cold when -NoWarm, -Interactive, -Gdb or -FaultStop is set. Find who sets it for normal bench/azt runs.
- Slots: Get-SlotCount (3ds/tools/azslot.ps1:24) = PM_AZ_SLOTS or 2, AzCap = 2 (:21). build3ds/azslots has slot1..3. 6 to 7 agents run tests at the same time.
- Queue wait per run is not recorded (runs.jsonl has wall_s, until_s, warm, no wait_s).
- Agents report collisions: PERF1 "slot contention made one ndmcheck run fail with no end line"; shots overwritten (fixed by SHOTLANE).

## Goal
Find and fix what makes tests slow or collide, with numbers. In order:
1. Record queue wait: wait_s (enqueue to grant) and boot_s (start to first game line) per run in runs.jsonl, and a `bench.py azstat [--since]` summary (runs, cold/warm, wait, boot, until, wall, kills by reason).
2. Cold by "option": trace who passes -FaultStop/-NoWarm/-Interactive for normal runs; if a det cold boot is the reason, check if a warm ROM reload (the kept Azahar loads the ROM again from boot) gives the same det bytes; if yes, use warm for det runs too. Gate: one det byte gate warm vs cold, byte-equal.
3. The 320 orphan kills: trace why each killed run leaves orphans and stop the churn.
4. Slot count: measure CPU and RAM with 2 and 3 live Azahars on this PC (turbo det run); set the cap to the largest count with no slowdown of a single run over 10%.
5. The failure "slot contention -> no end line": find the cause (input focus, hotkey to the wrong window, shared folder) and fix it.

## Start state
Worktree build3ds/wt/azt12, branch azt12 from nds1 (current). Tools only. Other agents use the slots: do not kill their runs; use the queue like them.

## Touch only
3ds/tools/azt.ps1, azslot.ps1, azreap*, 3ds/run_azahar.ps1, 3ds/tools/bench.py, docs/handoff/test_catalog.md (text).

## Done criteria
1. azstat prints the numbers above for today; wait_s and boot_s recorded by new runs.
2. Each of steps 2-5: the cause named (file:line), the change, and the before/after number from azstat.
3. azt.ps1 -SelfTest PASS. One det byte gate (spot_mac01_det) equal after the changes.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=azt12 PM_ROW=azt12. No git stash. No stubs. Commit after each passed step.

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "azt12 ready <hash>".
