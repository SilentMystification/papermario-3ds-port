# Row CRASHEXIT2: the clean exit after a logic-thread crash hangs (watchdog at 3 s)

(Model: Haiku. Base: main 4db2dfadf. Files pm_logic.c and pm_wdog.c only: disjoint from every running row. Reads only this file and CLAUDE.md.)

## User fact
XL crash, build 4195: every [CRASH] line and "exiting the process" were written, then
`watchdog: the crash handler did not finish in 3 s, ending the process`.

## Cause (oracle code read, main 4db2dfadf)
- Order in crash_handler (pm_main.c:142-202) is already right: registers, memory, stack walk (at most 2048 words), phase line, all
  through pm_crash_line (crash.txt first, pm_log.c:1338-1356), then the crash page (its wait refreshes the crash tick, :92-102), then
  the clean exit: pm_crash_exit = 1, pm_exit_request = 1, the tick is reset for 3 s (:199-201) and the thread ends.
- The 3 s that ran out are the clean exit's: the crashed thread was the logic thread. The game thread waits for logic progress in loops
  that test only the logic counters: pm_lg_quiesce (pm_logic.c:110, lg_done), pm_lg_quiesce_ms (:119, until its deadline only),
  pm_lg_hand ring wait (:138, lg_qtaken), the publish wait (:149, lg_dpub). A dead logic thread never moves them, so the game loop
  never reaches its exit test and the watchdog (pm_wdog.c:162-168) ends the process. Its line names the handler, not the exit.

## Start
`sh 3ds/tools/wt_new.sh CRASHEXIT2`. PM_ROW=CRASHEXIT2 PM_LANE=CRASHEXIT2 on every build and bench call.

## Touch only
3ds/src/pm_logic.c: the four wait loops above and one extern line. 3ds/src/pm_wdog.c: wd_main, the crash tick block.

## Exact edits
1. pm_logic.c: after the includes add `extern volatile int pm_crash_exit; /* pm_main.c CRASHEXIT */`. Add `&& !pm_crash_exit` to the
   condition of: the while at :110 (`(int)(lg_done - lg_handed) < 0`), the while at :119 (same test), the while
   `while (lg_qn - lg_qtaken >= LG_RING)` (:138) and the while `while (lg_dpub != lg_handed)` (:149). Comment once at :110:
   `/* CRASHEXIT2: a crashed logic thread never ends its step: the exit goes on */`.
2. pm_wdog.c wd_main crash block: `extern volatile int pm_crash_exit;` at file scope; the message becomes two: when pm_crash_exit is 1,
   `watchdog: the exit after a crash did not finish in 3 s (game thread phase %u %s), ending the process` with pm_wd_phase and
   pm_wd_phase_name(pm_wd_phase) (snprintf into a 160-byte buffer, then pm_crash_line); else the old line.

## Gate (CLAUDE.md 2; Azahar budget 2 of 8)
- Build, memcheck OK; `bench.py run nd_host --row CRASHEXIT2 --why "pm_logic.c waits"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py run spot_mac01_det --row CRASHEXIT2 --why "no change without a crash"`,
  `bench.py cmp spot_mac01_det --row CRASHEXIT2`: 6/6 byte-equal.
- Console check for the user (the crash test of CRASHEXIT): a push with `crashtest=logic`; expect the crash page, A, then the HOME
  Menu or hbmenu, and no watchdog line in crash.txt.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file
in the completing commit. Hand back the hash, the gates, the memcheck line.
