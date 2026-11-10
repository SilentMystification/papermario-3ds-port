# Row HOMEHANG: pressing HOME on the N3DS hangs the game (user, 2026-11-09)

## User facts
N3DS (192.168.1.224), main de4f4b8c8 build #3742: "pressing the home button on n3ds causes the game to crash".

## Log (orchestrator, build3ds/hw_live_log_224.txt)
Last lines: map sbk_35 loaded, then `watchdog: no progress for 10 s, game thread last phase 10 (hud and test switches), steps 3417`,
`step_game_loop call 15, walker op ed, last flush pm_gpu_end_w:2424`. No "home: suspend" line: the HOME hook did not reach its log.
On the N3DS the logic thread runs on core 2 (prof `core2 ... core 2`).

## Suspect (code read)
pm_log.c cpu_hook (about :1190): on APTHOOK_ONSUSPEND / ONSLEEP / ONEXIT it first calls pm_lg_quiesce() (pm_logic.c:108-112), which
loops until the logic thread finishes its step (`while (lg_done - lg_handed < 0) { pm_wq_pump_blocked(); pm_wq_wait_walker(); }`)
with no time limit, then logs "home: suspend". If the logic thread on core 2 cannot finish while the HOME Menu opens (the system
can take core 2 from the app at suspend, or the logic step waits for the game thread), the hook waits forever and the watchdog ends
the process. The O3DS XL (logic on core 1) is not reported.

## Start state
`sh 3ds/tools/wt_new.sh HOMEHANG`, branch HOMEHANG. PM_ROW=HOMEHANG, PM_LANE=HOMEHANG. One coder agent (the only agent running).

## Steps
1. Code check: confirm the hook thread and the quiesce wait path; find what the logic step can wait on (pm_wq / game thread / GPU).
   Add log lines before and after the quiesce in the hook ("home: quiesce start/end <ms>").
2. Fix: bound the quiesce in the hook (for example 200 ms total); on a timeout log "home: quiesce timeout" and go on with the suspend
   (the save flush still runs after it: keep SAVESAFE intact by flushing only data the game thread owns, or skip the flush on timeout
   and say why). If the cause is core 2 being taken, also stop handing new logic steps while suspended.
3. Gates: nd_host PASS, build OK, memcheck OK. Azahar cannot show the N3DS core 2 case: one Azahar run with the HOME hook forced by a
   debug word if one exists (else none). Console (user): N3DS, press HOME in a field and in a battle, return, the game continues;
   the log has home: quiesce lines.
4. Commit; delete this row file in the completing commit. Budget: 2 Azahar starts.

## Touch only
3ds/src/pm_log.c (cpu_hook), 3ds/src/pm_logic.c (pm_lg_quiesce or a bounded variant).

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
