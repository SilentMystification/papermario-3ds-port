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
