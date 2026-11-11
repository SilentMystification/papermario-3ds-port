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
b177f4bfe (code, scenario text), fa752b07e (bench menu_close); head ae780001e. memcheck OK, nd_host PASS (run 247392).
Gate 2 run 249036 failed: the pad script selected Return to Title, not Close game.
## Cause of the gate fail (oracle code read)
- pm_options.c:414-416 (tab_ids, General tab, mode 1 = Select menu): row 0 Frame skip, 1 Resolution Mode, 2 60 FPS, 3 Return to Title
  Screen, 4 Exit to Homebrew Menu, 5 Close game. The menu opens at row 0 (sel[] starts at 0); one ddown moves one row (pm_menu_hold).
- Scenario menu_close (azt_scenarios.json) presses ddown 3 times: row 3. The run log shows "tab 0 row 3 Return to Title Screen = Press
  A again", then the title return and the attract demo (azt exit 3, ATTRACT LOOP). The script dates from before MENUGEN added rows 0-2.
- rc 2 "crash symbols" of that run is not the exit path (the exit never ran): build3ds/wt/ESCAPE3/build3ds/lane-ESCAPE3/last_crash.txt
  lists 96 UnmappedAccess reads at 57 s, Read32 @ 0xF51F30DE..0xF51F311A, at PC 0x00100630 = load_mtx (nd_mtx_set.c:104-111, the
  nd_mtx_unpack of a Mtx at a bad w1) in the attract demo. A separate bug (oracle question); not this row.
- bench.py check gave PASS because the bench has no gates (bench.py gate_lines :547-561 checks only listed gates).
## Next edits (one commit: "ESCAPE3: menu_close selects Close game (row 5); bench gates")
1. azt_scenarios.json, scenario menu_close, "debug": the padscript gets two more `wait20,ddown` after the third ddown:
   `padscript=wait80,SELECT,wait20,ddown,wait20,ddown,wait20,ddown,wait20,ddown,wait20,ddown,wait20,A,wait20,A,wait60`.
   Event "close armed" regex: `menu: shot bot_m\d+ tab 0 row 5`. "subject": "row 5 Close game" in place of "row 4 Close game".
2. bench_defs.json, bench menu_close: replace `"gates": []` with
   `"log_metrics": [{"name": "chainload_line", "regex": "#(\d+) menu: chainload cleared", "pick": "last"},`
   `{"name": "exit_line", "regex": "#(\d+) exit: audio job done", "pick": "last"}],`
   `"gates": [{"metric": "chainload_line", "op": ">", "value": 0}, {"metric": "exit_line", "op": ">", "value": 0}]`
   In the JSON files every `\d` above is written `\\d` (as in the other regexes there). (A missing line is "MISSING", a failed gate: bench.py :554-556). The texts are pm_options_ui.c:19 and pm_main.c:928.
3. `python 3ds/tools/bench.py rebuild`, `python 3ds/tools/bench.py show menu_close`.
The exit ends as before: the until line "exit: audio job done" prints in pm_shutdown before ndsp, citro3d and gfx shut down; the run
pauses Azahar at that line, so the process end is never needed for a pass.
## Gate (CLAUDE.md 2: no picture; no draw code changes, so no byte compare)
- Build with memcheck OK and nd_host PASS: done (b177f4bfe, run 247392). The bench entry is no build input: no rebuild.
- `python 3ds/tools/bench.py run menu_close --row ESCAPE3 --why "ESCAPE3 Close game chainload clear"`: LOG CHECKS PASS (all events),
  then `python 3ds/tools/bench.py check <run id>`: PASS chainload_line and PASS exit_line (no MISSING).
- The HOME Menu after Close game on hardware: the user's console check.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
