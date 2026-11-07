# Row AZT3: one kept Azahar per slot serves every lane (reload the ROM, do not close)

## Goal (user, 2026-11-06)
Azahar instances stay open and are reused: a new run loads its ROM into the kept Azahar instead of closing it and starting a new one. Today only 11 of 57 recent starts were warm (grep "^azslot:" build3ds/azt/logs/*.txt): 22 cold "other lane" (the kept Azahar ran another 3dsx), 24 cold "no kept" (the kept one was closed: not kept after a run that did not match, GPU fault, reaper, or -NoKeep).

## Causes and fixes (in order, one commit each)
AZT3-1 ANY LANE WARM: 3ds/tools/azslot.ps1 (warm decision ~line 90-104) and 3ds/run_azahar.ps1 (~line 217): the slot owns one fixed ROM path (build3ds/azslots/slot<k>/rom/pm_3ds.3dsx). Each run copies its lane 3dsx there, then the kept Azahar restarts (the slot's restart shortcut, F6 / Ctrl+R from qt-config.ini, ~line 138) and loads the new file. "other lane" is no longer a cold reason. Check: the boot build line in the log shows the NEW build number after a warm restart (a stale ROM is a fail).
AZT3-2 KEEP ON FAIL: a run that times out or misses its until line keeps its Azahar too (paused), unless Azahar exited or crashed. Only a crash, a GPU fault exit or -NoKeep closes it.
AZT3-3 WARM BOOT LINES: a warm restart loses boot lines such as "prof=1 log=off" (LOG4 gate A failed on this). Fix the reason: the until/marker check must read the log from the start of THIS boot (truncate or mark the offset at restart; if the game appends, find the last boot banner). Gate: log_storm_prof warm matches its marker.
AZT3-4 REAPER: azreap idle time 15 minutes (comments say 3 and 10: make one value, one constant).
Done: a list of 3 scenarios on 3 different lanes runs in one slot with "azslot: slot k warm" for runs 2 and 3, each log shows its own build number; log_storm_prof passes warm; one failed until keeps the Azahar.

## Rules
Files: 3ds/tools/azslot.ps1, azreap.ps1, 3ds/run_azahar.ps1 (slot, keep, restart and log-offset parts only), 3ds/tools/azt.ps1 (keep flags only). The LOG agent edits the log-matching part of azt.ps1 and run_azahar.ps1 at the same time (live log reader): edit only your parts, commit only your hunks through your own temporary index (GIT_INDEX_FILE copied from HEAD). No git add -A, commit -a, reset, stash, checkout, restore. No attribution lines (no Co-Authored-By) in commits. Status line in 3ds/logs/perf/status_AZT3.txt before each Azahar start. Budget: reset and continue. Delete this file in the commit that completes AZT3-4.
