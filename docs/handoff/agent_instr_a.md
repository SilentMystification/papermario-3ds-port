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
