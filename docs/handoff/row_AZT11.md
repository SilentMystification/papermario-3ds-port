# Row AZT11: one Azahar run queue, sensible order, stale runs booted (user, 2026-11-07)

User: "azahar runs should be sequenced in a way that makes sense and stale runs should be booted."

## Facts
6+ agents run azt.ps1 / bench.py at once on 2 slots (cap AzCap = 2, azslot.ps1). Today each run waits for a slot on its own; order is arbitrary, a lane change forces a ROM reload, and orphan runs keep slots: the VERIFY chain kept running 74 min after its agent ended (stopped by the orchestrator). pool.log (build3ds/azslots/pool.log) records every start, warm restart, kill and exit.

## Rows (one commit each)
AZT11-1 QUEUE: one machine-wide FIFO run queue (a lock-protected file under build3ds/azslots/queue/, one entry per waiting run: time, tree, lane, scenario, row, owner pid, heartbeat time). azt.ps1 enqueues and waits for its turn instead of polling slots. Order: FIFO, with one rule: when a slot frees, prefer the oldest waiting entry whose lane matches the ROM loaded in that slot (saves a reload), but never let an entry wait more than 2 turns behind a younger one. Print the queue position and wait time in the run result line.
AZT11-2 STALE BOOT: an entry or a running run is stale when (a) its owner pid (the azt.ps1 / bench.py parent chain) is gone, or (b) its heartbeat is older than 60 s, or (c) a running run is past its scenario cap plus 30 s. The queue keeper (azt.ps1 on every enqueue and every poll, and azreap) removes stale entries and stops a stale running run (kill its run_azahar child, keep the kept Azahar if it is healthy, pool.log reason "stale run booted: <a|b|c>"), so the slot frees at once.
AZT11-3 LIST: `azt.ps1 -Queue` prints the queue and the running runs (slot, lane, scenario, row, age, owner alive). bench.py run/list prints the same summary line on start.
Done: 6 concurrent runs from 3 worktrees, 2 lanes: all complete, order logged, at most 1 reload per lane change pair; an orphan test (start a run, kill its parent) is booted within 60 s and the next queued run starts; a run past cap+30 s is booted.

## Rules
Files: 3ds/tools/azt.ps1, azslot.ps1, azreap.ps1, 3ds/run_azahar.ps1, 3ds/tools/bench.py (queue line only). Own worktree build3ds/wt/azt11, branch azt11 from nds1. Code first (CLAUDE.md rule 5). No attribution lines, never commit 3ds/src/map_restore_table.c. Keep Azahar open. Delete this file in the last commit. Hand back once with the head; the orchestrator merges at once and tells all agents to rebase.

## State (2026-11-07, after merge 99ccf421d)
- Merged: AZT11-1..3 and AZT11-4 code (501cf83a1). All worktrees told to take the harness files from nds1.
- AZT11-4 gate FAIL by environment: 5 cold of 18 after the first 2, causes "cold: turbo" and "cold: no kept" from other worktrees' old harness code. Rerun the gate when every worktree runs the merged harness.
- AZT11-5: many SCENARIO BROKEN results (battle_start, battle_start_r13off, mdlw_battle, wcen_mac01) are likely the prof-mode shot line drop (row_SIZE "Oracle decision SIZE-1 shots", pm_log_evt). Rerun after LOGEVT merges.
- Uncommitted in worktree azt11: bench_defs.json turbo default, test_catalog.md slot text. yard_hud / yard_wide_hud exist only on hud1b, fb1, enccrash.

## AZT12 stale boot "c" fires early (2026-11-07)
Fact: pool.log shows "stale run booted: c (ended on stop request)" for log5_u1_prof (cap 45 s, grace 30 s) after 8-14 s of run time, 4 times (02:44 to 02:54), and for spot_iwa10_pic once. Rule "c" is azslot.ps1:408 (`now - runT > (cap + QCapGraceSec) * 1000`). All worktrees run the same azslot.ps1 (md5 72fb2d02). So runT or cap in the entry the sweeper reads is wrong at that time.
Route (code first): trace every write of runT and cap (azt.ps1:170 enqueue: cap 30, runT = enqueue time; azt.ps1:561 running: runT, cap; Set-QFields; Save-QEntry JSON round trip of [long] values; any path where state = running is written before runT, or a warm-restart retry or cold fallback that keeps the old entry with the old runT, e.g. a second round). Name the file, line and value. Fix there. Gate: log5_u1_prof runs to its until-line 3 times with no stale boot; a selftest case for the found path; azt.ps1 -SelfTest passes.
Then AZT11-4 gate rerun (20 runs, 0 cold after the first 2, now that every worktree has the merged harness) and AZT11-5 (battle_start family, median wall over 10 picture benches) on nds1 with logevt (5680d32b8). Port yard_hud and yard_wide_hud only if they are on nds1; else drop them from the list.
