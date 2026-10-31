# Plan: console test harness (O3DS, unattended, log checks only)

Status: PLAN. No source change is made by this plan.
User decision 2026-10-31: the console harness has no shots and no picture check. It runs the same log checks as azt.ps1 (until line, expected events, numeric gates, fps lines, jump test lines, crash, watchdog and stall rules).

## 0. What exists now (read on 2026-10-31)

| Piece | File | State |
|---|---|---|
| Scenario list | 3ds/tools/azt_scenarios.json | 46 scenarios. Fields: name, status, subject, proves, debug, until, cap, deadline, first_marker, expects, needs_picture, expected_events (name, regex, max, min, pick), extra, reason. |
| Log checks | 3ds/tools/azt.ps1, function Show-Checks | Runs on any log file. `-CheckLog <file>` already checks a log with no start. |
| Until static check | azt.ps1 Get-UntilProblem | Runs before a start. Reusable. |
| Push | 3ds/push_3ds.sh | Sends build3ds/pm_3ds.3dsx with 3dslink. Game args: `--verbose livelog <words>`. 3 connect attempts of 15 s, send limit 60 s. Then runs livelog.py. Log file: build3ds/hw_live_log_<last octet>.txt. |
| Debug string on the console | 3ds/src/pm_main.c game_thread | Switches = sdmc:/3ds/PaperMario/debug3ds.txt + the netload args. 255 bytes total. The log line `switches <...>` shows the result. |
| Live log | 3ds/tools/livelog.py | Receive only. 32 KB replay on connect, `#<seq>` per line, GAP lines on a loss. Stops on an end line (`exit: audio job done`, `boot_main returned`, `cpu limit ... exit (main)`, `watchdog: exiting the process`) or 5 s with no connection. |
| Quit on the console (QUIT1) | 3ds/src/pm_log.c read_cmd, pm_log_poll | In the working tree, not committed (agent QUIT1). The game reads one line on the live socket. `quit` sets pm_exit_request (same path as the menu row Exit to Homebrew). The game logs `quit received`. The PC side does not send it yet. docs/handoff/agent_quit1.md does not exist yet. |
| Hang watchdog | 3ds/src/pm_wdog.c | 10 s with no progress: logs `watchdog: exiting the process`, then svcExitProcess. `wdog=<s>` changes the limit. |
| Exit at a frame | pm_main.c `exitframe=<n>` | Leaves the main loop at frame n. A fallback when no quit arrives. |
| FTP pull | 3ds/tools/ftppull.py | Pulls log.txt and Luma dumps. Needs the FTP app on the console (port 5000). The FTP app and the netloader cannot run at the same time. |

## 1. PC driver

Put the driver in azt.ps1 as `-Console <ip>`. This reuses the scenario load, the until check, the stall, marker and attract rules, and Show-Checks. A second script would copy them.

Steps of `azt.ps1 <scenario> -Console <ip> -Row <row>`:
1. Load the scenario. Refuse unknown or unverified (exit 4) as now. Run Get-UntilProblem (exit 7).
2. Remove the shot words from the debug string: `shots`, `shotwin=*`, `shotbottom=*`, `shotskip=*`. Skip each expected event whose regex starts with `shot`. Ignore needs_picture. (ponytail: name-based filter; add a `console_events` field only if a scenario needs other events.)
3. Start `sh 3ds/push_3ds.sh --no-build --ip <ip> -t <cap x ConsoleScale> <debug words>` in the background. Set PM_LIVELOG_OPTS to `--until <until regex> --stopfile build3ds\hwt\stop.flag`.
4. livelog.py: on the first line that matches the until regex, send `quit\n` on its socket. When the stop file exists, send `quit\n` too. A second connection must not send the quit: the game closes the old client on a new accept (pm_log.c accept_client).
5. The driver reads build3ds/hw_live_log_<octet>.txt with Read-LogChunk (same code, other path). The clock starts at the first log line, not at the push start (the 13 MB send takes up to 60 s).
   - first_marker missing after the deadline: write the stop file. Result exit 2.
   - more than 2 attract lines in a warp or tour scenario: stop file. Exit 3.
   - no new line for 40 s: stop file. Exit 6.
6. Wait for push_3ds.sh to end. It ends on the game end line, or 5 s with no connection, or the cap.
7. Checks on the log copy (build3ds\hwt\<scenario>.txt):
   - Show-Checks (expected events, gates, [CRASH]).
   - The `switches` line holds every word of the sent debug string. A miss means a stale debug3ds.txt on the SD card or a cut at 255 bytes. Fail.
   - No `[livelog] GAP` line. A gap: fail as LOG INCOMPLETE.
   - After the until line: `quit received`, then `exit: audio job done`. A miss: fail as NO EXIT (the console may not be in the Homebrew Launcher).
   - `watchdog: no progress` line: fail as HANG.
8. Append one line to 3ds/logs/perf/console_runs.csv: time, row, scenario, build string from the log, ip, exit code, result text.
9. Exit codes: same as azt.ps1. Add 10 PUSH FAILED (push_3ds.sh printed it) and 11 NO EXIT.

Calibration knobs (the O3DS is slower than Azahar): `-ConsoleScale` (default 2) multiplies cap. The marker deadline is max(deadline, 60) s. Change these after the first runs show the real times.

The Azahar start budget (rule 5) does not count console runs. The record file is the count.

## 2. Console side

Already in place: switches from netload args, the live log server with seq numbers, `loglevel=`, the until lines and expected-event lines (all are log lines), the hang watchdog, the crash handler, `exitframe=`, and QUIT1 (uncommitted).

Missing pieces (each is a row):
- QUIT1 commit and its console test (agent QUIT1 owns it).
- NL1: the next push without a button press. Now each push needs a person to open the Homebrew Launcher and press Y (docs/plans/hw_batch.md, S28_prep.md). After quit, the launcher is open, but its netloader is off. Unknown: whether the game can relaunch itself with new switches (libctru next-load to the Homebrew Launcher, with argv) and where the netloaded 3dsx is stored on the SD card. This is an investigation first (rule 3), then a code row.
- No shot writer work. The user removed shots from this harness.

## 3. What cannot be automated

- The Y press in the Homebrew Launcher before each push, until NL1 works. If NL1 shows that the launcher cannot relaunch the game, one Y press per scenario stays.
- Console power, Wi-Fi, and the start state (the Homebrew Launcher open). A person does this.
- A Luma exception screen (a crash that the game's crash handler does not catch) waits for a button. A full console hang needs a power cycle.
- FTP pull of log.txt and Luma dumps: a person starts the FTP app, and it excludes the netloader. The harness uses the live log only. FTP is for a failed run, by hand.
- Speed judgment: the user judges speed on the console (rule 6). The harness records fps and phase lines as numbers only.

## 4. Rows (each under 60 changed lines)

| Row | Change | Lines | Test that proves it |
|---|---|---|---|
| QUIT1 | Console quit command (in the tree, agent QUIT1). | done in tree | QUIT1's own test. |
| H1 (first) | PC driver. livelog.py: `--until`, `--stopfile`, send `quit\n` on the open socket (about 15 lines). push_3ds.sh: pass `$PM_LIVELOG_OPTS` to livelog.py (1 line). azt.ps1: `-Console <ip>`, steps 1 to 9 above (about 40 lines). No console change. | about 56 | Build with QUIT1. Person opens the launcher and presses Y. Run `azt.ps1 yard -Console 192.168.1.170 -Row H1`. Pass: map and steady-frame events pass, the log has `quit received` then `exit: audio job done`, a console_runs.csv line exists, exit 0. Second run: `tour_no_warp` (a fail demo). Pass: exit 2 or 3, and the log still has `quit received`. |
| NL1-inv | Investigation (fresh agent): can a netloaded 3dsx set the next load in the Homebrew Launcher with argv, and is the 3dsx on the SD card? | 0 (report only) | Report with the libctru and launcher source lines. |
| NL1 | Only if NL1-inv says yes. Live command `next <switches>`: the game sets the next load (itself, with the new switches), then quits. livelog.py sends it in place of `quit` when the driver gives a next scenario. | about 40 | Two scenarios in a row with one Y press: both log checks pass, the second log shows its own `switches` line. |
| H2 | Batch: `azt.ps1 -Console <ip> -Batch a,b,c` runs the list in order. Stops at the first PUSH FAILED or NO EXIT. Only after NL1. | about 20 | A batch of 3 log-only scenarios with one Y press. 3 lines in console_runs.csv. |

## 5. Risks

- A hang: the watchdog ends the process after 10 s. Unknown: whether a process ended by svcExitProcess returns to the Homebrew Launcher. H1 records it (the next push answers or not). A hang that also stops the watchdog thread needs a power cycle.
- LAN: netload fails often on the O3DS Wi-Fi (HANDOFF_2026-10-22). push_3ds.sh already tries 3 times. The driver reports PUSH FAILED (exit 10) and stops. No retry loop.
- A live-log loss: livelog.py reconnects and reports GAP lines. A gap fails the run. The game ring holds 32 KB, so a short loss can be refilled.
- The console must be on, on the LAN, and in the netloader at each start. The driver cannot check this before the push. A connect failure is the signal.
- Stale sdmc:/3ds/PaperMario/debug3ds.txt on the console adds switches. The switches check in step 7 finds it.
- Rule 6 limits pushes for the user to the end of an epic. Unattended console runs by an agent are a new case. The orchestrator must decide the rule before H1 runs on the console.
- Frame-based until lines (for example `warp: scene f300`) arrive later on the O3DS. The cap scale covers this. A scenario that does not reach its until line in the scaled cap fails as a timeout, not as a game fault.

## 6. First row to build: H1

Files: 3ds/tools/livelog.py, 3ds/push_3ds.sh, 3ds/tools/azt.ps1. No console source. It needs a build that has QUIT1. Without QUIT1 the game does not quit, and the run fails as NO EXIT.
