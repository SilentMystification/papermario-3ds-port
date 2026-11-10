# Row AZTSTEREO: merlee_stereoat ends at a subject line; a timed-out run keeps its logged shots

(Model: Haiku. New worktree: `sh 3ds/tools/wt_new.sh AZTSTEREO` from main 019d6ec8c. PM_ROW=PM_LANE=AZTSTEREO. CLAUDE.md rules 1-7. Commit after each passed step. Delete this row file in the completing commit.)

## Cause (code)
1. Tool: at a timeout, run_azahar.ps1 does not pause Azahar. The pause at :445 runs only for `$reason -like "matched*"`. The game runs on through the 30 s shot wait (:466-480) and writes a new shot every 6 frames. A file written at the poll is short. :491 counts every file on disk, logged or not, so the run gets "SHOT COPY FAILED". azt.ps1:724-729 then exits rc 1 before the timeout check at :736, so the record is fail:1, not broken (rc 10). The game logs a shot line only after fclose (pm_gpu.c:1832), so an unlogged short file is a write cut by the stop, not a lost shot. The pause at the until line can cut a shot in the same way.
2. Scenario: merlee_stereoat waits for "merlee: undarken done" (f992). From f500 every frame is stereo (two passes and the replay), so Azahar runs slower. The undarken came near the 45 s cap in the STEREOFX1 gate and after the cap now. The subject (the 3D talk and the price choice with the slider on, then the darken) ends at "merlee: darken done" (f632, npc_merlee.c:152, always logged).
## Touch only
3ds/run_azahar.ps1, 3ds/tools/azt_scenarios.json (merlee_stereoat), 3ds/tools/bench_defs.json (merlee_stereoat_pic).
## Exact edits
1. run_azahar.ps1:445: change the condition `$reason -like "matched*"` to `($reason -like "matched*" -or $reason -like "timeout*" -or $reason -like "stopped by azt*")`. Change the comment: a timeout or a stop also pauses, so no new shot is written in the shot wait.
2. run_azahar.ps1 shot wait loop (:471-475) and `$short` (:491): if `$logged.Count -gt 0`, check only files whose number is in `$logged` (`$logged -contains [int]$f1.Name.Substring(0, 4)`). If `$logged.Count -eq 0` (periodic shots log no line), keep the old check of every file.
3. After the wait (after :486), if `$pausedAtUntil -and $logged.Count -gt 0`: remove files whose number is not in `$logged` and whose length is not `$shotFull` (a write cut by the pause). Comment: the game logs a shot only after fclose (pm_gpu.c dump_shot).
4. merlee_stereoat: until `merlee: darken done`; cap 30; debug shotwin `596:40:6` (7 shots, the darken); expected_events: map, stereo replay, darken done (`merlee: darken done f\\d+`), shot; remove shuffle start and spell done. subject/proves/reason: the 3D talk, the price choice and the darken with the slider at 1 from f500; cap 30 because stereo frames run slower in Azahar (the undarken run passed the 45 s cap).
5. bench_defs.json merlee_stereoat_pic: until `merlee: darken done`, cap 30, purpose to match.
## Done criteria
1. `bench.py run merlee_stereoat_pic --row AZTSTEREO --why "until darken done, shot settle"`: log_pass; the run line has "until at", and the end is 5 s or less after the until line; `SHOT FILES: 7 logged`, 7 full; no SHOT COPY FAILED. One Azahar start.
Stop: no until in 30 s: hand back with the `stopped after` line and the last `shot N frame F` line.
