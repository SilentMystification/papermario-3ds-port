# Row ESCAPE: START + SELECT leaves a hung game (debug)

## User facts (2026-11-10, O3DS XL)
The 3D slider hang needs a hard restart (power button). The user asks for a debug toggle: START + SELECT held at the same time
returns to the HOME Menu, usable while the game is hung.

## Facts (orchestrator)
- The watchdog (3ds/src/pm_wdog.c) already sees a stall after 10 s and calls svcExitProcess (:45). After that the console still
  needs a hard restart, so a plain process exit does not free the system when the GPU or GSP is stuck.
- The game thread is the one that hangs. The escape must run on a thread that keeps running (the watchdog thread) and read the
  buttons without the game's input code (the HID shared memory, not hidScanInput of the game thread).

## Design (oracle batch 8)
(Model: Haiku. 2 Azahar starts. `sh 3ds/tools/wt_new.sh ESCAPE`, PM_ROW=PM_LANE=ESCAPE. Files: 3ds/src/pm_wdog.c, 3ds/src/pm_gpu.c, 3ds/tools/azt_scenarios.json, 3ds/tools/bench_defs.json. Output to build3ds/out/. Delete this row file in the completing commit.)

Design:
- **Thread:** the watchdog thread (pm_wdog.c wd_main, core 1, priority 0x3F). It keeps running while the game thread waits on the GPU.
- **Polling:** the loop sleeps 100 ms instead of 1 s. A counter runs the existing per-second stall logic every 10th tick.
- **Pad read:** HID shared memory directly, not hidScanInput (that writes the game thread's kHeld/kOld). `if (hidSharedMem) { u32 id = hidSharedMem[4] & 7u; u32 k = hidSharedMem[10 + id*4]; }`, the same words hidScanInput reads. Fire when `(k & (KEY_START|KEY_SELECT)) == (KEY_START|KEY_SELECT)` holds for 10 polls in a row (1 s).
- **On by default.** Debug switch `noescape` turns it off. SELECT alone opens the port menu (pm_input.c:277); the 1 s hold of both keys is what exits.
- **New in pm_gpu.c (read only, any thread):**
  - `int pm_gx_stuck(void)` returns `q->curEntry > q->lastEntry` (submitted, not completed).
  - `int pm_gpu_hang_dump(char* b, int n)` writes: `gx n %u cur %u last %u` + for i in [lastEntry, curEntry): `e%u id %02x %08x %08x %08x` (data[0]&0xFF, data[1], data[2], data[3]) + ` stereo %d sr_on %d sr_n %u sr_fbn %u f%u`.
  - q = (gxCmdQueue_s*)&__C3D_Context, as in pm_gx_room.
- **wd_escape():**
  1. pm_crash_line("escape: START+SELECT 1 s, phase %u steps %u stuck %d " + dump).
  2. If !pm_gx_stuck(): pm_exit_process() (the existing clean exit: save flush, log close, svcExitProcess; back to the HOME Menu).
  3. Else: APT_HardwareResetAsync() (apt.h:288, APT:U; the console reboots to the HOME Menu, no power button). After it: svcSleepThread 5 s, then svcExitProcess as the last resort.
  4. No GSPGPU_ResetGpuCore: with the PICA hung that call may block the only thread that still runs.
- **Stall path change** (pm_wdog.c:138-150): at the limit, print the 3 existing lines plus the dump line.
  - If pm_gx_stuck(): print once "watchdog: GPU stuck, the process stays; hold START+SELECT 1 s to reboot", do not exit, keep polling. svcExitProcess with a stuck GPU leaves a hung console and makes the escape impossible.
  - Else: exit as now.
- **Debug seams** (debug tools may stay):
  - `wdhang=N`: at frame N the walker loops `for(;;) svcSleepThread(1000000)` in pm_gpu_end_w before flush.
  - `wdesc`: escape_poll treats the combo as held once the stall has lasted 3 s.

Gates:
- nd_host PASS, build and memcheck OK.
- One Azahar scenario wd_escape (warp=random, `wdhang=300 wdesc`, cap 25 s, until-line `escape: START\+SELECT`). Expected: the escape line with `stuck 0`, then the process ends. Azahar cannot show a stuck GPU.
- Console (user, XL): hold START+SELECT 1 s in the field: back to the HOME Menu. After the 3D slider hang (wait 12 s): hold START+SELECT: the console reboots to the HOME Menu, and crash.txt has the dump (this is the STEREOFX evidence).
- Stop: 2 starts, rule 2 STOP RULE.
