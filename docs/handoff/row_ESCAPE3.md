# Row ESCAPE3: debug menu Close game closes the app to the HOME Menu (aptClearChainloader)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## User facts (2026-11-10)
- The debug menu Close game and the START+SELECT escape (row ESCAPE2) are two separate exit mechanisms: "Separate it as 2 different exit
  mechanisms, keep both."
## Cause (oracle code read)
- pm_options_ui.c:15-25 pm_home_chainload calls `aptSetChainloader(0x0004001000021000 + (region << 8), MEDIATYPE_NAND)`. Those title ids
  are system applications (00040010-00021000 System Settings, -00021100 Download Play, -00021200 Activity Log), not the HOME Menu (an
  applet, 00040030-00008F02 USA). libctru aptExit then sends DoApplicationJump (0x00320084) to that title.
- libctru aptInit sets chainload-to-self when __system_runflags has RUNFLAG_APTCHAINLOAD (4) (Homebrew Launcher). aptClearChainloader()
  clears aptFlags bit 0x40000000, so aptExit takes its close path: PrepareToCloseApplication (0x00220040), then CloseApplication
  (aptExitProcess, 0x00270044) at the process end, and the HOME Menu runs (libctru.a apt.o disassembly).
- The rest of the Close game path stays: pm_options_ui.c:177-182 sets pm_exit_request, the loop ends, pm_shutdown runs, main returns.
## Start
After ESCAPE2 is merged (both edit 3ds/tools/azt_scenarios.json; never at the same time).
`sh 3ds/tools/wt_new.sh ESCAPE3` (build3ds/wt/ESCAPE3, branch ESCAPE3 from main). PM_ROW=ESCAPE3 PM_LANE=ESCAPE3 on every build and bench call.
## Touch only
3ds/src/pm_options_ui.c (pm_home_chainload and its comment), 3ds/tools/azt_scenarios.json (scenario menu_close only),
3ds/tools/bench_defs.json (one new entry, after the bench menu_texf). No other running row edits these files.
## Status
Edits 1 and 2 committed (b177f4bfe); memcheck OK (static 20704 margin 1282), nd_host PASS (run 247392). Gate 2 could not start:
no bench runs the scenario menu_close ("no bench named menu_close", rc 4, no start).
## Next edit: the bench (test_catalog.md E)
In 3ds/tools/bench_defs.json, after the closing `},` of the bench "menu_texf", insert:
```
    {
      "name": "menu_close",
      "kind": "azahar_stream",
      "speed": "normal",
      "default": false,
      "purpose": "ESCAPE3: Select menu Close game, two A presses: the close path line, the chainload clear (aptClearChainloader), then the process end line. Log only.",
      "scenario": "menu_close",
      "cap": 45,
      "until": "exit: audio job done",
      "tags": ["azahar", "menu", "exit"],
      "gates": []
    },
```
The until text is in the C source: pm_main.c:928 `pm_log("exit: audio job done, saving flushed, ...")` (pm_shutdown). The chainload
event of the scenario matches pm_options_ui.c:19 `menu: chainload cleared, the app closes to the HOME Menu`.
Then `python 3ds/tools/bench.py rebuild` and `python 3ds/tools/bench.py show menu_close`. Amend nothing: commit the bench entry as
"ESCAPE3: bench menu_close (scenario menu_close)" (the scenario edit is in b177f4bfe; the bench joins it before the merge).
## Gate (CLAUDE.md 2: no picture; no draw code changes, so no byte compare)
- Build with memcheck OK and nd_host PASS: done (b177f4bfe, run 247392). The bench entry is no build input: no rebuild.
- `python 3ds/tools/bench.py run menu_close --row ESCAPE3 --why "ESCAPE3 Close game chainload clear"`: LOG CHECKS PASS (all events).
- The HOME Menu after Close game on hardware: the user's console check.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
