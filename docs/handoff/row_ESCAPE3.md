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
3ds/src/pm_options_ui.c (pm_home_chainload and its comment), 3ds/tools/azt_scenarios.json (scenario menu_close only).
## Exact edits
1. pm_options_ui.c lines 14-25: replace the comment and the body of pm_home_chainload with:
   `/* ESCAPE3: Close game. Under the Homebrew Launcher aptInit set the chainload to self (RUNFLAG_APTCHAINLOAD). Clearing it makes aptExit`
   ` * close the application (APT PrepareToCloseApplication, CloseApplication), so the HOME Menu runs. The old title id 0004001000021000 +`
   ` * region was System Settings, not the HOME Menu (an applet). */`
   `static void pm_home_chainload(void) {`
   `    aptClearChainloader();`
   `    pm_log("menu: chainload cleared, the app closes to the HOME Menu");`
   `}`
   (cfguInit, CFGU_SecureInfoGetRegion and cfguExit go: no other user in this file.)
2. azt_scenarios.json, scenario menu_close: expected event "chainload" regex becomes `menu: chainload cleared`; "proves" says
   "the second A on Close game logs the close path and the chainload clear (aptClearChainloader), then the process end line"; "reason"
   says "(title id by region)" -> "(chainload cleared)".
## Gate (CLAUDE.md 2: no picture; no draw code changes, so no byte compare)
- `PM_ROW=ESCAPE3 PM_LANE=ESCAPE3 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`, memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row ESCAPE3 --why "ESCAPE3 build check"`: PASS.
- `python 3ds/tools/bench.py run menu_close --row ESCAPE3 --why "ESCAPE3 Close game chainload clear"`: LOG CHECKS PASS (all events).
- The HOME Menu after Close game on hardware: the user's console check.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
