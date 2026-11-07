# Row AZT4: GPU memory faults with kept Azahars

## Goal
Azahar starts fail with "Failed to allocate Stream buffer of preferred size 8192 KiB" (exit -1073741795, 0xC000001D) on about half the starts while several kept Azahars hold GPU memory (GPU 6.5 to 6.9 of 8 GB). AZT3 (9b5e3d20b) keeps Azahar open 15 minutes per slot, so more instances hold VRAM. Fix: a start never fails because idle kept Azahars hold the GPU.

## Rows (one commit each)
AZT4-1 SLOT CAP: one cap on live Azahar processes (all slots, all agents): default 2, constant in azslot.ps1 (PM_AZ_SLOTS above the cap is clamped, with one log line). A new start that finds the cap reached reuses a kept idle Azahar (warm, AZT3-1 ROM copy) instead of starting another.
AZT4-2 FAULT RETRY: on exit -1073741795 or the stream-buffer line, close every kept Azahar that is idle (no run holds its slot lock), wait 2 s, retry once in the same slot. Log `azslot: gpu fault, closed <n> idle, retry`.
Done: a list of 4 runs from 2 lanes started while 2 kept Azahars are idle: no GPU fault exit, at most 2 Azahar processes at any time (tasklist sample each 2 s into the status file), warm starts reported.

## Rules
Files: 3ds/tools/azslot.ps1, 3ds/run_azahar.ps1 (slot and start parts), 3ds/tools/azreap.ps1. Work in the shared folder; commit only your hunks through your own temporary index (GIT_INDEX_FILE copied from HEAD). No git add -A, commit -a, reset, stash, checkout, restore. No attribution lines. Status line in 3ds/logs/perf/status_AZT4.txt before each Azahar start. Budget: reset and continue. Delete this file in the last commit.
