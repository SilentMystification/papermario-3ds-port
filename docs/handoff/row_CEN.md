# Row CEN: walker census with why codes, refusal split

## Goal
Name why each remaining walker GPU draw (wd) is not native, per scene, so the streams TRI, OFF, PM5, MDL and RECT-2 each own their cases. Plan: docs/handoff/plan_walker_end.md (read sections Inventory, Decisions, Streams and ownership, Rows CEN-1a, CEN-1b, CEN-2).

## Start state
Branch nds1, HEAD 3f6ce7a01. Census tool: pm_gbi.c os_report, pm_gpu.c cen_add / cen_frame_end (WCEN3, agent_wcen.md).

## Rows (in order, one commit each)
CEN-1a WHY: plan row CEN-1a. Missing scenarios for the 8 scenes: add them (switch for the census, until-line the wcen4 line of the last census frame). Output: the wd x why table per scene, replacing the old table in docs/handoff/agent_wcen.md, plus the R13 aux forms and the R11 cases seen.
CEN-1b SPLIT: plan row CEN-1b. Commit it fast: TRI, OFF and PM5 wait for it.
CEN-2: not now. The orchestrator relaunches CEN-2 after each deletion row.

## Touch only
3ds/src/pm_gpu.c cen_add, cen_frame_end, the refusal line(s) of nd_raw_native; 3ds/src/pm_gbi.c os_report; docs/handoff/agent_wcen.md; own hunks of azt_scenarios.json, bench_defs.json.

## Done criteria
1. CEN-1a: per scene, sum of why counts = wd; no picture change; table in agent_wcen.md.
2. CEN-1b: spot_mac01_pic byte-equal to HEAD before the split; nraw_why counts unchanged.
Hand back after CEN-1b, with the table.

## Rules
CLAUDE.md rules 5, 7, 9. Decisions D1 to D10 of docs/handoff/plan_walker_end.md are fixed. Other agents edit pm_gpu.c, pm_gbi.c, pm_tex.c, nd_*.c at the same time: edit only the functions your stream owns (plan table "Streams and ownership"); commit only your hunks through your own temporary index (GIT_INDEX_FILE copied from HEAD), never the shared index; no git add -A, commit -a, reset, stash, checkout, restore. Build in a scratch tree from HEAD plus your diff (git archive HEAD, apply your diff, copy assets/us and ver/us/build, gen_map_restore.py, copy 3ds/src/map_restore_table.c), lane named after your row; azt.ps1 <scenario> -Row <row> -Build <lane>. After each code commit build the committed tree clean. Status line in 3ds/logs/perf/status_<row>.txt before each Azahar start. Budget: reset (powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget <row>) and continue. No toggles (debug switches allowed, removed in the deletion row), no stubs, no push, no attribution lines, never commit 3ds/src/map_restore_table.c. Two tries per named cause, then the next code point. A new scenario goes into azt_scenarios.json and bench_defs.json in the same commit. Delete this file in the commit that completes the last row.
