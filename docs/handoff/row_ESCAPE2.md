# Row ESCAPE2: START+SELECT exits through the normal exit path, Debug row "START+SELECT" picks the target

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## User facts (2026-11-10, O3DS XL, main 86004672a)
- "start+select quit doesnt work as expected, it kicks us out to the homescreen and restarts the system from a fault." XL log:
  `escape: START+SELECT 1 s, phase 2 steps 29470 stuck 0 gx n 1 cur 1 last 1 ... f7974` (GX queue idle, game loop running).
- "from the debug menu we should be able to configure the start+select behavior between homebrew menu, quitting and homescreen if possible."
## Cause (oracle code read)
- pm_wdog.c:110 `if (!stuck) pm_exit_process();` runs on the watchdog thread; pm_exit_process (pm_wdog.c:41-47) calls svcExitProcess
  while the game, logic, walker and audio threads run. It skips every step of pm_shutdown (pm_main.c:919-933) and of libctru exit:
  1. ndspExit (pm_audio.c:36) never runs: the DSP component stays loaded, its DMA points into the linear heap the kernel frees.
  2. C3D_Fini and gfxExit (pm_main.c:930-931) never run: GSP right, framebuffers and GX work of the dead process stay with GSP.
  3. aptExit never runs: libctru aptExit is the only sender of APT PrepareToCloseApplication (0x00220040) / CloseApplication
     (aptExitProcess, 0x00270044) or DoApplicationJump (0x00320084) (libctru.a apt.o disassembly). NS sees the active app die unclosed.
- The clean path exists: pm_exit_request (pm_options_ui.c:26) ends the loop (pm_main.c:534), main runs pm_shutdown and returns.
- libctru aptInit sets chainload-to-self when __system_runflags has RUNFLAG_APTCHAINLOAD (4) (Homebrew Launcher): a plain return goes
  to hbmenu; aptClearChainloader() first makes aptExit close the app to the HOME Menu; aptJumpToHomeMenu() is the HOME button (game
  thread only: it calls GSPGPU_SaveVramSysArea and the suspend hooks; aptMainLoop calls it the same way).
- The debug menu Close game (pm_options_ui.c) is a separate exit mechanism (user, 2026-11-10): row ESCAPE3. This row does not touch it.
## Start
`sh 3ds/tools/wt_new.sh ESCAPE2` (build3ds/wt/ESCAPE2, branch ESCAPE2 from main). PM_ROW=ESCAPE2 PM_LANE=ESCAPE2 on every build and bench call.
Never at the same time as ESCAPE3 (both edit 3ds/tools/azt_scenarios.json); ESCAPE2 runs first.
## Touch only
3ds/include/pm_port.h, 3ds/src/pm_options.c, 3ds/src/pm_wdog.c, 3ds/src/pm_main.c, 3ds/tools/azt_scenarios.json, 3ds/tools/bench_defs.json.
## Exact edits
1. pm_port.h PmOpt: after `poslog` add `int esc_mode; /* ESCAPE2: START+SELECT: 0 Homebrew Menu, 1 Quit (close to the HOME Menu), 2 HOME Menu (suspend). options.ini escmode=hbmenu|quit|home */`.
   Declare `const char* pm_esc_name(int m);` and `void pm_esc_take(void);`.
2. pm_options.c: initializer `.esc_mode = 0` (add "esc_mode hbmenu" to the line 13 comment); line 20 `22` -> `23`. After dpad_label (line 73):
   `static const char* const esc_name[3] = { "hbmenu", "quit", "home" }; static const char* const esc_label[3] = { "Homebrew Menu", "Quit", "HOME Menu" };`
   `const char* pm_esc_name(int m) { return esc_name[m < 0 || m > 2 ? 0 : m]; }`. pm_options_write: `escmode=%s\n` after `poslog=%d\n`, arg
   `esc_name[pm_opt.esc_mode]` after `pm_opt.poslog`. pm_options_init: after the dpad_mode block, the same block for key `escmode` (esc_name,
   3 values, sets pm_opt.esc_mode). Enum line 349: `RW_ESC` after `RW_BOSSFOLDER`. tab_ids tab 2: `ids[n++] = RW_ESC;` after Fixed Seed.
   Label switch: `case RW_ESC: return "START+SELECT";`. Value switch: `case RW_ESC: return esc_label[pm_opt.esc_mode];`. pm_optui_step, before
   the `if (id == RW_FPS ||` line: `if (id == RW_ESC) { pm_opt.esc_mode = (pm_opt.esc_mode + (dir > 0 ? 1 : 2)) % 3; pm_options_touch();
   pm_log("options: escmode %s", esc_name[pm_opt.esc_mode]); return 1; }`. pm_optui_act switch: `case RW_ESC: pm_optui_step(tab, row, 1, mode); return 0;`.
3. pm_wdog.c: `extern volatile int pm_exit_request; volatile int pm_esc_req; int pm_debug_value(const char* key, char* out, unsigned n);`
   pm_esc_take (game thread): `int r = pm_esc_req; if (!r) return; pm_esc_req = 0; pm_log_evt("escape: taken, mode %s", pm_esc_name(r - 1));`
   `if (r == 3) { aptJumpToHomeMenu(); return; } if (r == 2) aptClearChainloader(); pm_exit_request = 1;`.
   wd_escape: replace line 110 with: `if (!stuck) { pm_esc_req = 1 + pm_opt.esc_mode; for (int i = 0; i < 30 && pm_esc_req; i++)
   svcSleepThread(100000000LL); if (!pm_esc_req) return; static const char m[] = "escape: the game loop did not take the request in 3 s,
   hardware reset\n"; pm_crash_line(m, (int)sizeof m - 1); }` (then lines 111-114 run unchanged).
   escape_poll, first line: `if (pm_exit_request || !aptIsActive() || pm_wd_asleep) { esc_n = 0; return; }`. After the wdesc line, seam:
   `static int esc_at = -2; if (esc_at == -2 && pm_frame_count() > 0) { char v[16]; esc_at = pm_debug_value("escat", v, sizeof v) ? atoi(v) : -1; }`
   `if (!held && esc_at > 0 && pm_frame_count() >= (unsigned)esc_at) held = 1; /* ESCAPE2 test seam: the combo held from frame escat */`.
4. pm_main.c line 547: after `pm_log_poll();` add `pm_esc_take(); /* ESCAPE2: the START+SELECT request of the watchdog */`.
5. New scenario `wd_escape_run` (copy wd_escape): debug `warp=random escat=300 loglevel=debug`, until `exit: audio job done`, cap 25,
   expected events `escape: START\\+SELECT 1 s, phase \\d+ steps \\d+ stuck 0`, `escape: taken, mode hbmenu`, `exit: audio job done`.
   New bench `wd_escape_run` in bench_defs.json (copy wd_escape, scenario and until as above). One commit with the scenario.
   Scenario wd_escape "proves": "the escape line prints with stuck 0; the parked game loop does not take the request, so after 3 s the watchdog notes it and calls APT_HardwareResetAsync". Do not run wd_escape.
## Gate (CLAUDE.md 2: no picture; no draw code changes, so no byte compare)
- `PM_ROW=ESCAPE2 PM_LANE=ESCAPE2 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`, memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row ESCAPE2 --why "ESCAPE2 build check"`: PASS.
- `python 3ds/tools/bench.py run wd_escape_run --row ESCAPE2 --why "ESCAPE2 running exit path"`: LOG CHECKS PASS (3 of 3).
- Quit, HOME Menu and the hung-game reset: the user's console check.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
