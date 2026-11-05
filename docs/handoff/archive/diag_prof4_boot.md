# diag_prof4_boot: why prof_boot gives no game log (commit 8b573558e, prof4)

Scope: read-only. Source read with `git show prof4:<file>`. Prof-only diff = `git diff 8b573558e^ 8b573558e` (4 files, 84 insertions). The three-dot diff `native...prof4` also shows nds1 work that is not part of PROF4.

## Evidence

- E-a. Both runs ended the same way. `3ds/logs/test_budget.csv` (PROF4): start 14:31:54, `retry` 14:32:09 "stopped after 11s: Azahar exited (Azahar exit code -1)", retry-start 14:32:13 (not counted), `end` 14:32:28 same text. Counted starts for PROF4: 1.
- E-b. No game log. `%APPDATA%\Azahar\sdmc\3ds\PaperMario\` has no `log.txt` and no `log.old`. `run_azahar.ps1:204` deletes `log.txt` before each start. So the game never created it. `log.txt` is created only by `open(...O_CREAT)` at `3ds/src/pm_log.c:240` in `pm_log_init`.
- E-c. Azahar emulator log is 0 bytes (`%APPDATA%\Azahar\log\azahar_log.txt`). No new `crash.txt` (last write Nov 3 01:36). The crash handler writes `crash.txt` (`pm_log.c:828`, `pm_log.c:977`). It did not run, or it ran before its file path existed.
- E-d. The test ran the PROF4 code. `build3ds/lane-nds1/pm_3ds.3dsx` (14:31) and the copy `pm_3ds_nds1.3dsx` both contain the string `prof=1 log=off`.
- E-e. `build3ds/lane-nds1/last_log.txt` does not exist. `run_azahar.ps1:347` printed "no log.txt written". `azt.ps1` then reports NO LOG in Show-Checks.

Reading: the game dies before `pm_log_init` opens `log.txt`, or the Azahar process dies with no game output. Both runs fail at the same point, about 11 s after the Azahar start.

## Question 1: is `pm_log_prof_apply(0)` before `pm_log_init` the cause?

Not as an ordering problem. The call is `3ds/src/pm_main.c:646`. In prof mode it changes only in-memory state: it rewrites `switches` (`pm_log.c:416-445`), clears the debug cache, sets `log_level = -1` and `con_lv = 4`, and calls `pass_recalc`. It calls `pm_log_set_file(1)` only when `log_file_on` is 0 (`pm_log.c:440`). In prof_boot `log=off` is not set, so that branch does not run. Nothing in it opens, creates, or renames a file. `pm_log_init` at `pm_log.c:234` runs next and does the rename and open at `pm_log.c:238-240` with or without the call.

So the order is not what removes `log.txt`. The call is still a problem for a different reason: it runs before the log exists and it changes the level state for `pm_log_init`. It also adds the only new stack buffer on the boot path (edit list item 3).

## Question 2: the new code before the file open

Only these changes run before `pm_log_init` (`pm_main.c:645-647`), in `game_thread` (`pm_main.c:614`):

1. `pm_main.c:637-644`: `fopen("options.ini")` and a 32-byte local `char ln[32]` (line 639), kept in the `game_thread` frame for the whole boot.
2. `pm_main.c:646`: `pm_log_prof_apply(0)`. Its own frame holds `char out[256]` (`pm_log.c:421`).

`game_thread` runs on a 256 KB thread (`pm_main.c:716`). The code comment at `pm_main.c:618` says a bigger local array in this function stopped the boot before (`H06d`, `_svfprintf_r PC 0`). So this thread has a small stack margin, and the new locals are the only stack growth on the path.

Ranked causes (not proven, no run yet isolates them):

- Most likely: the game thread stack overflows or is corrupted at boot because of the new locals (`pm_main.c:639`, `pm_log.c:421`). The crash handler does not run (E-c), which fits a stack fault.
- Possible: a fault in `pm_log_prof_apply(0)` itself. Code read: no fault found in `pm_log.c:416-445`. Its `out` buffer is bounded by `sizeof switches` (256), and the copy loop always moves forward.
- Less likely: a fault in the build (new `Slot.pad` use at `pm_log.c:394`, `pm_log.c:642`). These run only after `pm_log_start`, so they cannot stop the open at `pm_log.c:240`.

Decisive test (one start): run the same binary with no `prof` word. If `log.txt` appears, the cause is in the prof path (items 1 to 3 below). If it still does not appear, the cause is outside the prof path.

## Question 3: the until-line and the scenario

The scenario in `3ds/tools/azt_scenarios.json` (`prof_boot`, around line 4490):

- `until`: `prof f\d+ w\d+ fps`. This line is printed by `pf_report` (`pm_phase.c:392` calls it after 300 frames). It goes through `pm_log_prof_line` (`pm_log.c:407`), which sets `prof_force` and `pad = 1`. `vlog` passes it (`pm_log.c:356`), and the drain writes it to the file only (`pm_log.c:642`). So the until-line is printed at warn level with logging otherwise quiet. The scenario is correct on this point.
- `first_marker` `prof=1 log=off`: printed by `pm_log_prof_apply(1)` (`pm_main.c:664`) through the same path. Correct.
- `cap` 25 and `deadline` 10: too short. `pf_report` fires only after 300 frames (`pm_phase.c:392`). At about 25 fps that is about 12 s after the first frame. Add the boot time on top. Set `deadline` 20 and `cap` 40. The reason field must say: "pf_report prints after 300 frames; cap covers boot plus 300 frames".
- The catalog row is missing. `docs/handoff/test_catalog.md` on prof4 has no `prof_boot` row. Rule 5 requires the scenario and the catalog row in one commit.

How `azt.ps1` reads the log: `azt.ps1:142` sets `$logPath` to the Azahar sdmc `log.txt`. `Read-LogChunk` (defined in azt.ps1 from line 144) tails that file live and matches `first_marker` against each new chunk. `run_azahar.ps1:345` copies it to `build3ds/lane-<lane>/last_log.txt` at the end. `Show-Checks` then reads that copy for `expected_events`. With no `log.txt`, the marker never matches and the copy never happens.

Exit codes: `stopped after 11s: Azahar exited (Azahar exit code -1)` is written by `run_azahar.ps1:406`. The `end` line carries `rc 0`, so `azt.ps1` does not report the rc as a failure. It fails on the missing log instead.

## Design (correct)

- `log.txt` is created in `pm_log_init` as in any boot. Prof mode does not change that.
- In prof mode, the gate passes only `prof_force` lines (`pm_log.c:356`, `pm_log.c:642`). Console and socket stay off (`pm_log.c:642`, `tc`).
- One call site: `pm_log_prof_apply(1)` after `pm_options_init` (`pm_main.c:664`). Remove the call at `pm_main.c:646`.
- The options.ini `prof=1` read stays in `game_thread`, but its buffer is static (edit item 2).
- Text fix: the commit message and `pm_log.c:413` comment say "prof on forces logging off". The code keeps the file open for prof lines (`pm_log.c:440`). Change the comment to match the code.

## Numbered edit list for the code agent

Branch: the checked-out branch (nds1), or a branch from prof4 if the fix is still untested (rule 7). Do not commit `3ds/src/map_restore_table.c`.

1. Remove `pm_main.c:646`: delete the line `pm_log_prof_apply(0);`.
2. `pm_main.c:639`: change `char ln[32];` to `static char ln[32];`.
3. `pm_log.c:421`: change `char out[sizeof switches];` to `static char out[sizeof switches];`.
4. `pm_log.c:413` comment: replace "Forces every log line off" with "keeps log.txt open; only prof lines are written".
5. `pm_log.c:440`: keep `if (!log_file_on) pm_log_set_file(1);` unchanged.
6. `azt_scenarios.json` scenario `prof_boot`: set `"deadline": 20`, `"cap": 40`, and add the cap reason to `"reason"` (text in the design section above).
7. Add the control scenario `boot_noprof_azahar` to `azt_scenarios.json`: `"debug": "phase loglevel=debug"`, `"first_marker": "threads: game on core"`, `"until": "boot_main"`, `"deadline": 20`, `"cap": 40`, `"needs_picture": false`, `"expects": "none"`, one expected event with regex `boot_main`. Status `unverified`. Reason field: "control for PROF4: same binary, no prof word".
8. Add a row for `prof_boot` and a row for `boot_noprof_azahar` to `docs/handoff/test_catalog.md`, with command, first marker, cap, and status `unverified`.
9. Build: `PM_LANE=nds1 PM_ROW=PROF4 sh 3ds/build.sh` after `python 3ds/tools/gen_map_restore.py`. Restore `3ds/src/map_restore_table.c` from the user copy afterwards.
10. Run the control first: `PM_ROW=PROF4 powershell -File 3ds/tools/azt.ps1 boot_noprof_azahar -Row PROF4 -Build nds1 -Try`. Write the justification line to `3ds/logs/perf/status_PROF4.txt` before the start. If `log.txt` is missing again, stop and report: the fault is outside the prof path.
11. If step 10 gives a log: run `PM_ROW=PROF4 powershell -File 3ds/tools/azt.ps1 prof_boot -Row PROF4 -Build nds1 -Try`. Justification line first. Pass means the boot line and the `prof f... fps` line are in `last_log.txt`, and the shot shows the subject (log test, needs_picture false).
12. Commit the source files (items 1 to 4) and the docs (items 6 to 8) in two commits on the branch. No attribution in the message.

## Open items

- The cause is not proven. Step 10 separates the two causes.
- The emulator log is empty (E-c). Azahar may crash without writing it. A stack-fault test needs no new tool. Use step 10.
- `docs/handoff/agent_nds1_e6.md` says `prof_boot` "added" with build and run "see status_PROF4.txt". The catalog row and the status line were missing at the time of this diagnosis.
