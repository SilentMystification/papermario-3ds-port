# Row AZT13: reuse Azahar instances (warm restart that does not fail)

## User facts (2026-11-08)
"azahar instances are still closing, we should be reusing instances." After AZT12 (merged 2026-11-08 ~16:40):
- pool.log since 17:00: warm restart "nonce found" 13, "nonce none" 19. Kills: 8 "restart failed" (both warm tries missed, then kill + cold), 7 cold "no kept", 4 cold "turbo" + 4 "kept replaced, cold: turbo", 1 "switch mismatch".
- The warm restart posts hotkey F6 to the Azahar window (pool.log "posted hotkey F6 sent=True foreground title 'Azahar 2126.2'"), then waits for the boot nonce. With 3 live Azahars and several agents, the key press is lost often (focus, timing). AZT12 found try 1 lost about 14% of the time; now both tries miss in about 40% of rounds.
- Speed change (turbo vs normal) and a switch change force a cold start ("cold: turbo", "switch mismatch").
- "no kept": the slot had no kept Azahar (closed by azreap after idle, or killed).

## Goal
A run reuses the kept Azahar of its slot almost always (target: >= 95% warm when a kept instance exists), without depending on window focus or a key press:
1. Trace the warm path (3ds/run_azahar.ps1 Get-WarmState and the F6 post, 3ds/tools/azslot.ps1) and name why the key is lost (focus, a modal, the window not ready, the key posted before the ROM copy finished, or the wrong window when several Azahars share one title).
2. Replace the key press with a restart that does not need focus. Options to check (pick by evidence, one path only): a message posted to the exact window handle of that pid (PostMessage WM_KEYDOWN to the hwnd, no foreground needed); Azahar's own command line / IPC to load a ROM into a running instance; or a game-side request (the game reads a restart request over its live log socket 17492 or a file in the slot sdmc and restarts itself through the 3DS app restart path). Determinism: a warm det run must stay byte-equal to a cold run (spot_mac01_det).
3. Speed change without a restart if Azahar can change the speed limit at run time (setting/hotkey to the pid's window); else keep one slot per speed when the queue allows.
4. Switch change: the switches come from the launch args/debug file; check if a warm restart can take new switches (the game reads them at boot from the sdmc file, so a restart may be enough).
5. azreap idle close: keep instances for the session (close only at the user's request or after a long idle, e.g. 60 min).
Report azstat warm/cold before and after.

## Start state
Worktree build3ds/wt/azt13, branch azt13 from main (current). Tools only. Other agents use the slots: never kill their runs.

## Touch only
3ds/run_azahar.ps1, 3ds/tools/azslot.ps1, 3ds/tools/azt.ps1, azreap scripts, 3ds/tools/bench.py (azstat), the game-side restart request only if option 2c is chosen (3ds/src, under the existing live log code), docs/handoff/test_catalog.md text.

## Done criteria
1. Cause named (file:line) for the lost key.
2. 10 runs in a row on one slot (mixed scenarios, same speed): >= 9 warm (azstat).
3. A speed change and a switch change: warm, or the reason they cannot be is named.
4. spot_mac01_det warm byte-equal to cold. azt.ps1 -SelfTest PASS.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=azt13 PM_ROW=azt13. No git stash. No stubs. Commit after each passed step.

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "azt13 ready <hash>".
