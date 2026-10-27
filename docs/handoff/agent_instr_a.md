# agent_instr_a: state at stop (plan_perf_instrumentation.md, PI1-PI3)

State: PI1, PI2, PI3 not done. No commits. No Azahar start. Tree clean for these rows (pm_port.h reverted).

PI1 blockers:
- python3 is missing on this machine. `python` runs but the edit script failed on the pm_gbi.c pdl_try anchor (0 matches, expected 1). Nothing from that script was written to pm_gbi.c, pm_nusys.c or pm_phase.c.
- Script: /tmp/pi1.py. Re-check the pdl_try anchor text (line near 2530 of pm_gbi.c) before reuse.
- Design: RS_TK_BG/MAIN/KEY, RS_TUS_*, RS_TOPS, RS_PUSH/NOPUSH/RET/RANGE/SPMAX/PDL/PDLM; gbi_task() helper in pm_nusys.c; pm_gbi_ops_now() in pm_gbi.c; RS_MAX macro; new taskpl line in pm_phase.c.

Next steps:
1. Fix anchor, apply PI1 edits, run gen_map_restore.py, copy .bak over map_restore_table.c, build.
2. Run PM_ROW=PI1 Azahar shot (1 start). Commit on pass.
3. PI2, then PI3, same method.

## Update
- PI1 edits applied and built (sh 3ds/build.sh OK). Uncommitted.
- The one Azahar start for PI1 ran on the old binary (build came after). No taskpl line. Budget used 1/8 with no pass. Next: needs a budget decision from the orchestrator, then one start on the new binary.
- Log of the stale run: 3ds/logs/perf/PI1_run.txt. Build log: 3ds/logs/perf/PI1_build.txt.
- PI2, PI3 not started.

## State (PI4, PI5 done; attract report; checks fixed)
Commits: d8103830f (PI4 gpusplit + PI5 log/hud cost, one binary), 8799c99e6 (check definitions), c0dbca1ad, cf1723071 (reports).
Not committed, not mine: HANDOFF.md, README.md, docs/handoff/memory/MEMORY.md, deleted feedback_workflow.md. Never commit 3ds/src/map_restore_table.c or its .bak.
Azahar: PM_ROW=PI4, 1 start used. Log 3ds/logs/perf/PI4_run.txt (82572 frames, title, intro, demo, demo world, battle, file select reached).
Reports: 3ds/logs/perf/attract_PI4.txt (per scene), attract_PI3.txt (old PI3 log), console_ready.txt (verdict: no).

## Check results (attract, 82572 frames)
- PI1 sum task us / (all exclusive phases): 0.947 over 1446 windows. Narrow miss of 0.95. Earlier check used only gbi+gpu+tex (1.150); it misses rom, yay0, audio, gcache, fbread, gdraw and fine g_* phases that nest in the task span.
- PI2 sum fl us / gpu: 2.617. Definition invalid. Flush time (flush_impl) is charged to whatever phase is current (gpu, tex, gcache, rom, yay0, audio). Fit to gpu+tex+gcache+rom+yay0+audio gave 1.02 on the earlier log, but that is a fit, not an identity.
- PI2 TEX n per frame: 152 (plan says about 65; attract mix differs).
- PI3 th n + tm n vs texcall: 1446 of 1446 windows match when floor(th per frame) equals texcall. texcall is an integer average, so the old test was wrong.
- PI4: gpusplit on every frame, kick n max 1. PASS.
- PI5: log avg 591 us, max 845 us, hud avg 64 us. Frame-wall match to PI4 run within 1 ms not tested (no PI5-only run).
- Phase ms values print at 0.1 ms resolution (phase window line).

## Open items
1. PI2: give flush time its own phase (or subtract it from the host phase) so C has an identity check. Code change, then one Azahar start.
2. PI1: 0.947 is below the 0.95 pass. Check whether the phase span and task span match (gdraw nested in task). Decide whether the check or the instrumentation is wrong.
3. Attract file select: reached once in the run (mode 14).
4. Console push not done. Console verdict stays no until PI1 and PI2 pass.

## Next steps
- Fix PI2 flush attribution in pm_gpu.c flush_impl and pm_phase.c; build; one PM_ROW=PI2b Azahar start on attract.
- Re-run attract2 report; rewrite console_ready.txt; commit.

## Commands
Build: `python 3ds/tools/gen_map_restore.py; cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c; sh 3ds/build.sh`
Azahar: `PM_ROW=<row> powershell -File 3ds/run_azahar.ps1 -Until 'game mode 17 frame 4700' -Timeout 900 -Debug 'loglevel=debug phase'` (the log comes from stdout; build3ds/last_log.txt was stale).
Report: `python <scratchpad>/attract2.py 3ds/logs/perf/PI4_run.txt 3ds/logs/perf/attract_PI4.txt PI4 3ds/logs/perf/console_ready.txt`
Push (not yet, console verdict is no): `PM_ROW=PUSH sh /c/Users/Andrew/Documents/GitHub/papermario-3ds-port/3ds/push_3ds.sh --no-build --ip 192.168.1.170 audiostat texstat pacestat logstat phase phsum loglevel=debug`
