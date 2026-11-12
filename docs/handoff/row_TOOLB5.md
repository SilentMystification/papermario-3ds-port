# Row TOOLB5: a run that asks for one slot (list child, base retry, GPU retry) is granted that slot first when it frees

(Model: Haiku. One edit in azslot.ps1 and one host test. No blocker.)

## Cause (ORACLE6 code read, main after TOOLB4 and TOOLDEDUP)
- The same-slot rule stays: per-slot sdmc state (options.ini, saves, cheats) can change bytes (row_TOOLB4.md).
- Invoke-QGrant (3ds/tools/azslot.ps1:399-423) grants a free slot k to, in order: an entry skipped twice, else the oldest entry
  whose lane and speed match the slot's last lane (Get-SlotLane), else the oldest. A want-k entry (TOOLB3 list child, TOOLB4
  base retry, GPU retry) can use only slot k but gets no priority on it. With 6 lanes and 3 slots, other lanes keep taking slot
  k (lane match), and the want entry waits until it has been skipped twice, run after run (S7EFX2: 741 s for run 2). The 900 s
  queue limit (azt.ps1:550) then ends it with LOCK TIMEOUT.
- The 45-minute hang of `bench.py base fxpic_16` (pid 11984, 3.9 GB of RAM) is a second cause, not the queue: the worktree
  copies of runs.jsonl are still 1.8-3.6 GB (S7EFX2 3,550,223,026 B, S7ENT 3,550,238,250, S5MENU0 1,778,793,028, S6E3 203 MB).
  Each ingest parses the tree's own file and every tree's file under runs.lock (bench.py append_runs, all_tree_runs_files).
  Main is 7.5 MB after TOOLDEDUP. That is the orchestrator step below, not this row.

## Start
`sh 3ds/tools/wt_new.sh TOOLB5`. PM_ROW=TOOLB5.

## Touch only
3ds/tools/azslot.ps1 (Invoke-QGrant only), new 3ds/tests/qgrant/run.ps1.

## Exact edits
1. Invoke-QGrant, right after `$key = Get-SlotLane $k`: the first pick is the oldest waiting entry with `[int]$_.want -eq $k`:
   `$pick = @($waiting | Where-Object { [int]$_.want -eq $k } | Select-Object -First 1)`, and the two existing pick lines run
   only `if (-not $pick.Count)`. Comment "TOOLB5: a run that can use only this slot goes first; its wait is at most one run".
   The skipped counting below stays.
2. 3ds/tests/qgrant/run.ps1: set `$env:PM_AZQUEUE_DIR` to a new temp folder and `$env:PM_AZ_SLOTS = 3`, dot-source
   3ds/tools/azslot.ps1, and pick a slot k with `Test-SlotFree k` true (none free: print "qgrant: no free slot, rerun" and exit
   2). Write three wait entries with New-QEntry and Save-QEntry: A (oldest, want 0, lane = Get-SlotLane k, so it matches the
   lane), B (want 0), C (newest, want k). Call Invoke-QGrant once. Expected: C is granted slot k. Print `qgrant PASS` or
   `qgrant FAIL <state of A, B, C>`, then remove the temp folder.

## Gate (expected values)
- `powershell -NoProfile -File 3ds/tests/qgrant/run.ps1`: `qgrant PASS` (on main without edit 1: FAIL, A granted. Run it once
  before the edit and write both lines in the commit).
- No Azahar start is needed. Budget 0.

## Stop
A FAIL after the edit, or a parse error: stop, commit, hand back. Delete this row file in the completing commit.
