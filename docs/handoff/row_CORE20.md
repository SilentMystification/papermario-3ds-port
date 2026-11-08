# Row CORE20: CORE2-0, measure the audio join wait and the core 1 load

## Goal
Two new fields in the prof line so a console prof run shows how long the game thread waits on audio and how busy core 1 is. Plan: docs/handoff/plan_core2.md. No behavior change.

## Start state
Worktree build3ds/wt/core20, branch core20 from nds1 3afca9217. Copy assets/us and ver/us/build into it. The audit agent edits pm_tex.c, nd_rect.c, nd_geom.c, pm_gpu.c in build3ds/wt/audit: do not edit those.

## Check first (0 starts)
Read pm_audio_join_timed (pm_audio.c ~234) and its call sites (pm_main.c ~599, pm_nusys.c ~157/163, pm_phase.c ~754). Grep the switch list for coreprobe and say what it measures. If a join-wait or core-1 figure already exists in the prof line, report it and stop.

## Steps (one commit)
1. aujoin: sum the ticks the game thread spends inside pm_audio_join (all call sites) per prof window; print `aujoin <us/frame> max <us>` in the prof line (pm_log.c / pm_phase.c prof row). Use the existing tick source of the prof code (no new timer API, no division in the hot path: divide once per window).
2. core1 busy: an idle probe thread on core 1 at the lowest priority (0x3F) that counts loop turns while nothing else runs; at startup calibrate turns per ms with the other core-1 threads idle (or use the first window as the base); print `core1 <busy %>` = 100 - idle share per window. Create it only in prof mode (prof on), so play and det runs have no extra thread. If core 1 is refused (time limit), print `core1 n/a`.
3. Keep the prof line format parseable: add the fields at the end of the existing line; update the bench.py ingest parser if it splits on known keys (3ds/tools/bench.py) so the new keys land in runs.jsonl as aujoin_us, aujoin_max, core1_busy.

## Gate
Build; nd_host PASS; spot_mac01_det byte-equal to base (det runs have prof off: no change); one Azahar run of a prof bench on lane core20 (azahar prof is direction only) shows the 2 new fields with plausible values; bench.py ingest of that log stores the 3 keys. Console push: not in this row (the orchestrator asks the user).

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. PM_ROW=CORE20 PM_LANE=core20 on every build.sh and bench call. Budget 8 starts. No git stash. Never commit runs.jsonl or 3ds/src/map_restore_table.c. The commit deletes this row file.
