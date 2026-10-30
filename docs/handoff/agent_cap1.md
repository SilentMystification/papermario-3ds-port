# agent_cap1 (row CAP1, diagnostic logging, no draw change)

## State
- Code done: pos line, window reset at map load, nested words in opsplit ops. Build passed (only the usual 32-bit enum link warnings, none in changed lines).
- Test: 1 Azahar start (budget 3 for CAP1, 2 left, not reset). Log: build3ds/last_log.txt (run of mac01t_cap1).
- Run result: load map area 1 map 2 at frame 209. First opsplit and pos line at f483, then f783 and f1083 (300 frame spacing). No window line at f299 or f599 (pre-reset windows gone). Top shot 0000 (frame 500) shows Mario, HUD on.

## Commit
- Files: 3ds/src/pm_gbi.c, 3ds/src/pm_phase.c, docs/handoff/agent_cap1.md, scenario mac01t_cap1 in 3ds/tools/azt_scenarios.json, catalog row in docs/handoff/test_catalog.md (staged as HEAD plus this row only; other working-tree hunks are not in the commit).
- pm_route.c not touched. map_restore_table.c not committed.

## Choices
- The `|| pm_ph_on` gate in mg_run was NOT added. With that gate, mg_run returns before the new os_nn count, so the count is dead under phase. Per-op time for nested words (audit section 1, second option) needs a separate decision.
- os_svc_last moved to file scope, so the pt field of the first window after a reset is correct.
- os_report tail memsets moved to os_win_clear(), also used by pm_gbi_win_reset().
- fs_win_reset() in pm_phase.c is shared by pm_ph_scene_map and the fsplit window end.

## Open items
- mops_report and mdc_report (mdlsum) are not reset at map load. Their first window after a load still mixes earlier maps.
- The fsplit reset runs on every pm_ph_scene_map call, including without phase.
- The mac01t_cap1 run went to the 120 s cap: the first until regex (pos f5\d\d) was wrong. Fixed to pos f\d+ M x. The log was checked with -CheckLog (6 of 6). A later run is not needed for CAP1.

## Commands
- python 3ds/tools/gen_map_restore.py; sh 3ds/build.sh; then put back the user copy of 3ds/src/map_restore_table.c (never committed).
- Test: powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 mac01t_cap1 -Row CAP1 -Try
- Check only: ... azt.ps1 mac01t_cap1 -Row CAP1 -Try -CheckLog build3ds/last_log.txt
