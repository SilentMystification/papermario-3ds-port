# Row BASEFIX: base runs never fail on the BASECACHE budget; a dead base lock never blocks (Haiku)

## Facts (HC4e3 handback, 2026-11-10; tool faults stop the gate path, CLAUDE.md rule 1)
1. `bench.py base mdl5_iwa10_det` ran twice and ended "not stored", azt.ps1 exit 9. bench.py runs base benches under row
   BASE_ROW = "BASECACHE" (3ds/tools/bench.py:1347). That row's Azahar budget (3ds/logs/test_budget.csv, azt.ps1:673) is never
   reset, so after 8 starts every base run hits the cap ("BUDGET: row BASECACHE has no starts left"). With BASEVERIFY's two-run check,
   the second run of the pair hit the cap and the first good run was dropped.
2. A third `bench.py base` call printed nothing for 600 s. build3ds/basecache/6d6cfb9ea0dd1478d4e3094a530fdfb2e7f76925.lock still exists:
   a base run killed by TaskStop skipped its `finally`, and the lock wait (BASE_LOCK_STALE_S = 2 h, bench.py:1504) waits silently.

## Touch only
3ds/tools/bench.py (base run, base lock), 3ds/tools/azt.ps1 (budget check) only if the exemption must live there.

## Exact edits
1. Budget: base runs do not count against any row budget. Either azt.ps1 skips the budget check for row BASECACHE, or bench.py
   passes the same no-budget switch that the tool already has (read azt.ps1 and run_azahar.ps1 budget code first; use an existing
   switch if there is one). A base pair is 2 starts per bench per merge-base and is a tool cost, not a row cost.
2. Lock: write the pid and the time into the lock (as now). A waiting run prints "base lock <commit> held by pid <p> since <t>, waiting"
   once, then every 60 s. If that pid is not alive (Windows: OpenProcess or `tasklist /FI "PID eq <p>"`), delete the lock at once and
   go on. Keep the 2 h rule as a last guard.
3. Delete the stale lock file above in the same commit's test, not by hand.

## Gate
1. Unit test with a fake lock of a dead pid: the base call removes it and goes on at once; with a live pid it prints the wait line.
2. One real `bench.py base mdl5_iwa10_det` (2 starts) at the current main: stored from two equal runs, no exit 9.
Stop: any other failure: hand back the exact output. No workaround.

## Rules
`sh 3ds/tools/wt_new.sh BASEFIX`. PM_ROW=PM_LANE=BASEFIX. Output to build3ds/out/ of the worktree. Foreground waits. Delete this row
file in the completing commit.
