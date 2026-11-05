# AZFLAKY: Azahar runs end early (diagnosis, no fix applied)

Scope: why yard_rec (reccheck scenario, until `rec f34\d `) and other scenarios end with `stopped after N s: Azahar exited`.
Tools read: 3ds/run_azahar.ps1, 3ds/tools/azt.ps1, docs/handoff/test_catalog.md, all 13 worktree test_budget.csv ledgers, %APPDATA%\Azahar\log\azahar_log.txt, the sdmc log.txt, Application event log (Get-WinEvent, 30 days).

## Findings

1. CONFIRMED. The early end is `$p.HasExited` (run_azahar.ps1:254). Result text "Azahar exited" appears in 21 end lines of 557 end lines across all ledgers (3.8 %).
   - 2026-10-29: 1 line. 2026-10-31: 1 line. 2026-11-03: 19 lines. Between 2026-11-03 01:15 and 02:48, 6 worktrees ran at the same time (main, uianchor, poshud, poswarp, btlstar, texgarble).
   - Stop times: 1 s (UC3base 02:47:48), 6 s (UC2 01:42:27, UA7, POSHUD), 11 s (UC3b 02:34:07), 12 s, 21 s (most rows).
2. CONFIRMED. yard_rec rows (ledger args contain `reccheck`, main ledger only): 7 of 22 end lines are "Azahar exited" (32 %). The other scenarios are not free of it. Other scenarios with the same result text: uianchor (UA7, 3), poshud (2), btlstar (yard, 2), poswarp (1), texgarble (1), main adhoc runs (5). The symptom is global during 2026-11-03, not yard_rec only.
3. CONFIRMED. Base build UC3base (b0a85588, base b728404af^): 3 runs, 1 "Azahar exited" at 21 s, 1 "Azahar exited" at 1 s, 1 match (23 s). That is 2 of 3 failed, as reported.
4. CONFIRMED. No azahar.exe fault in the Windows Application log for 2026-10-04 to 2026-11-03. The 60 azahar.exe fault records (IDs 1000 and 1001) end on 2026-10-27. The GPU driver crash is not the cause of the 11-03 exits.
5. CONFIRMED. Shared-file contention at boot. The emulator log of the latest session (about 02:48) has two `Common.Filesystem Rename:439: failed ... sdmc/3ds/PaperMario/log.txt --> log.old` lines at 14.12 s. The game renames log.txt at boot (src/pm_log.c:237-238). The rename failed because another handle held log.txt open. run_azahar.ps1:195 deletes log.txt under the lock with `-ErrorAction SilentlyContinue`. The delete failed silently, so the file from the earlier session survived. The same silent delete applies to the emulator log at run_azahar.ps1:199.
6. CONFIRMED. The emulator log (%APPDATA%\Azahar\log\azahar_log.txt) is 0 bytes in the failing runs (task statement). Only the last session is kept in that file. The earlier failing sessions cannot be read again.
7. CONFIRMED. run_azahar.ps1 never records the Azahar exit code. `$p.ExitCode` is not printed or written anywhere.
8. CONFIRMED. The exit rules do not match the symptom in one case. A stale match on `rec f34\d ` would end the run as "matched", not "Azahar exited". No ledger row with the 1 s exit shows a match.
9. INFERRED. The early exit comes from a second Azahar process on this PC, not from the game. No script in the repo starts azahar.exe outside run_azahar.ps1. go.ps1 calls run_azahar.ps1 (under the lock). A manual window of Azahar (not under the lock) can exist, and the Azahar binary contains Qt local socket strings (QLocalServer, QLocalSocket). A single-instance hand-off to a running window would end the new process at once, which fits the 1 s and 6 s exits. Not tested.
10. INFERRED. The game-side causes do not fit. The game ends its own process only through svcExitProcess in pm_wdog.c (hang watchdog after 10 s, or crash handler). The watchdog writes "watchdog: no progress" lines to log.txt first (pm_wdog.c:60-67). The failing runs have no game log, so the watchdog did not run.
11. NOT CHECKED. The Start-Process ID check (run_azahar.ps1:230-231) and azt.ps1 Get-AzaharProc (azt.ps1:174) use the PID of the script's own Azahar. A stale PID could not kill another run's Azahar, so this path is not a cause.

## Causes ranked

- A: a second Azahar instance outside the lock (interactive window or manual start). Not confirmed. Fits the early exits and the 0-byte emulator log.
- B: silent delete of log.txt and azahar_log.txt (run_azahar.ps1:195, :199). Confirmed as a fault path. It can give stale game log lines to the until match and the crash check.
- C: an Azahar exit that is not logged, because the exit code is not kept (item 7).

## Fix (cheapest first)

1. run_azahar.ps1:195 and :199. Change to `Remove-Item ... -ErrorAction SilentlyContinue`, then `if (Test-Path $log) { Release-Lock; ...; exit 5 }` with the message "LOG LOCKED: log.txt in use by another process. Nothing was run." This ends the run before the start. Same for $emuLog. Cost: none. Effect: a locked file becomes a visible error, not a silent stale match.
2. run_azahar.ps1:254. Before `break`, read `$p.ExitCode` and print it, and print `(Get-Process azahar -ErrorAction SilentlyContinue | Select Id,StartTime)`. Add the exit code to $reason, for example "Azahar exited code $($p.ExitCode)". This is a logging change only. The next early exit then names its code.
3. azt.ps1 before the End-Run result check (around azt.ps1:283). If the reason is "Azahar exited", the game log is absent, and azahar_log.txt is 0 bytes, retry once. This costs one more Azahar start in the budget. The start counts, so this needs the user decision on the budget rule (CLAUDE.md rule 5 says the budget never blocks work).
4. Game side (not changed here): pm_log.c:237-238 renames log.txt at boot. A per-run log name would need a game change (`pm_log_init`), and the user must approve that source change. Not recommended until items 1 to 2 show the cause.
5. Operator rule (no code): close any manual Azahar window before a test set. This is the only fix for cause A that the repo cannot enforce.

## Commands used (no Azahar start, no edits to source)

- Ledger tally: awk over 3ds/logs/test_budget.csv in each worktree, end lines, result text.
- Event log: Get-WinEvent -FilterHashtable (Application, IDs 1000/1001/1002, 30 days) from a .ps1 file in the scratchpad.
- Logs: %APPDATA%\Azahar\log\azahar_log.txt (grep for Rename:439), sdmc log.txt (grep for watchdog).

## Open items

- Confirm cause A with item 2 (exit code and running Azahar list) at the next early exit. One start, with a justification line in 3ds/logs/perf/status_AZFLAKY.txt.
- The 0-byte emulator log cannot be recovered for the 11-03 runs. Only the next run shows whether it repeats.
