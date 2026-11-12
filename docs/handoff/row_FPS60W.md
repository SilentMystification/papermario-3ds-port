# Row FPS60W: no unbounded GPU wait; a stuck GPU writes its dump and reboots the console (the 60 fps hang)

(Model: Haiku. Exact edits at named lines; host-checkable. User fault 2026-11-12: turning on 60 fps froze the O3DS .170, power cycle.)

## Cause (ORACLE7 code read, main 5be532ef9)
- Every GPU wait in the frame path has no timeout: pm_gpu.c:2789 and :2941 `C3D_FrameBegin(0)` (citro3d waits for the GX queue
  with timeout -1 and the result is not checked), :2461 pm_gx_room and :2479 pm_gx_drain `gxCmdQueueWait(q, -1)`, :1655
  `gspWaitForP3D()` (jumptest debug) and :1842 `gspWaitForPPF()` (menu present). A GPU that stops on a command list blocks the
  game thread forever.
- pm_wdog.c:218-222: after 10 s with the GPU stuck the watchdog writes the dump to crash.txt and then keeps the process
  ("the process stays; hold START+SELECT 1 s to reboot"): the frozen screen the user saw. The reset exists only in wd_escape
  (:135 APT_HardwareResetAsync).
- 60 fps doubles the GPU frames per step and adds the key frame copy (pm_gpu_key_show :2936-2963), which never ran on hardware.
  The read finds no out-of-range value in it (row FPS60VAL proves its command words on the host); this row makes any hang end in a
  dump and a reboot instead of a frozen console, so the next occurrence names its GX entry by value.

## Start
`sh 3ds/tools/wt_new.sh FPS60W`. PM_ROW=FPS60W PM_LANE=FPS60W.

## Touch only
3ds/src/pm_gpu.c (lines :1655, :1842, :2461, :2479, :2789, :2941 and one new function above pm_gx_room), 3ds/src/pm_wdog.c
(:218-222 only).

## Exact edits
1. pm_gpu.c, new `static void gx_wait_or_reset(gxCmdQueue_s* q, const char* where)`: `if (gxCmdQueueWait(q, 500000000LL)) return;`
   then build one line `gpu hang at <where> f<frame> ` + pm_gpu_hang_dump(...) and pass it to pm_crash_line, then
   `APT_HardwareResetAsync(); svcSleepThread(5000000000LL); svcExitProcess();` (the wd_escape tail, pm_wdog.c:135-137).
2. :2461 and :2479: `gxCmdQueueWait(q, -1);` becomes `gx_wait_or_reset(q, "gx_room");` / `gx_wait_or_reset(q, "gx_drain");`.
3. :2789 and :2941: before `C3D_FrameBegin(...)` call `gx_wait_or_reset((gxCmdQueue_s*)&__C3D_Context, "frame_begin")` (the queue is then
   done, so citro3d's own wait returns at once); then `if (!C3D_FrameBegin(fb_flags))` (and `(0)` at :2941) writes one crash line
   `gpu: frame begin refused f<frame> where <frame_begin|key_show>` and returns from the function without queueing anything.
4. :1655 and :1842: `gspWaitForP3D()` / `gspWaitForPPF()` become `gspWaitForEvent(GSPGPU_EVENT_P3D, false)` / `(GSPGPU_EVENT_PPF,
   false)` only if libctru offers no timed form; otherwise the timed form with 500 ms and the same reset path (check
   libctru gspgpu.h in the build image and write which one in the commit).
5. pm_wdog.c :218-222: after the "GPU stuck" crash line, do the reset of wd_escape (:135-137) instead of `continue`; the text
   becomes "watchdog: GPU stuck, rebooting (dump above)".

## Gate (expected values; no Azahar-only proof: Azahar never hangs here)
- Build, memcheck OK; nd_host PASS.
- `grep -n "gxCmdQueueWait(q, -1)\|gspWaitForP3D()\|gspWaitForPPF()" 3ds/src/*.c`: no line left.
- Test seam (exists): the switch `wdhang=<frame>` parks the walker (pm_gpu.c:2975-2979); one Azahar run of a copy of spot_mac01_det
  with `wdhang=200` (new scenario `wdhang_mac01`, until `watchdog: no progress`): the crash lines appear in the log (budget 2
  starts). The GPU-stuck branch is proven by code read only (Azahar cannot stick the GPU): name its lines in the commit.

## Stop
A build error or a wait left: stop, commit, hand back. Delete this row file in the completing commit.
