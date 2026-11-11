# Row SYNC60: with 60 FPS (interpolation) the frame begin is paced by time, not by the vblank wait (user decision 2026-11-11)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Cause (oracle code read)
- pm_gpu.c pm_gpu_begin_wait :2441 `fb_flags = (pm_interp_on > 0 && !pm_wd_wake_frames) ? C3D_FRAME_SYNCDRAW : 0;` and :2443
  `C3D_FrameBegin(fb_flags)`: with 60 FPS on, every frame begin blocks the walker until the next vblank (citro3d frame sync), on top of
  the wait for the GX queue. At 30 FPS (interpolation off) the begin waits only for the GPU, and the frame loop paces by time
  (pm_main.c:613-623, sleep until sched_next). So with 60 FPS a frame that is ready early still waits the vblank, and a late frame
  waits up to one more vblank.
## Start
`sh 3ds/tools/wt_new.sh SYNC60` (build3ds/wt/SYNC60, branch SYNC60 from main). PM_ROW=SYNC60 PM_LANE=SYNC60 on every call.
Runs beside HC8: HC8 edits pm_gpu.c pm_gpu_spr_build (:3796) and pm_gpu_grange (:2924); this row edits pm_gpu_begin_wait (:2408-2448)
and adds one static function directly above it. Do not edit or move any other line of pm_gpu.c; `git merge main` before the
completing commit. Runs beside LOADMTX (nd_mtx_set.c): no shared file.
## Touch only
3ds/src/pm_gpu.c: the block just above `void pm_gpu_begin_wait(void) {` (new function) and line :2441 inside pm_gpu_begin_wait.
## Exact edits
1. Directly above the comment block that precedes `void pm_gpu_begin_wait(void) {` (the line `static int begin_pend, begin_clear;`
   stays where it is), add:
```
/* SYNC60: with 60 FPS (interpolation) one frame begin per LCD period (1/59.83 s), paced by the tick: a sleep to 0.5 ms before the time,
 * then a short spin. Replaces C3D_FRAME_SYNCDRAW, which blocked the walker until the vblank. No division: 15/4 ns per tick (0.6 % long,
 * covered by the 0.5 ms margin and the spin). */
static u64 s60_next;
static void sync60_pace(void) {
    const u64 period = 4481228ull; /* SYSCLOCK_ARM11 (268111856) / 59.83 */
    const u64 margin = 134056ull;  /* 0.5 ms of ticks */
    u64 t = svcGetSystemTick();
    if (s60_next && t < s60_next) {
        u64 rem = s60_next - t;
        if (rem > 2u * margin) svcSleepThread((s64)(((rem - margin) * 15u) >> 2));
        while (svcGetSystemTick() < s60_next) {}
        t = s60_next;
    }
    s60_next = (s60_next && t < s60_next + period) ? s60_next + period : t + period; /* a late frame starts a new schedule: no catch-up */
}
```
2. Line :2441 becomes two lines:
   `unsigned fb_flags = 0; /* SYNC60: the begin waits only for the GX queue; 60 FPS is paced by time */`
   `if (pm_interp_on > 0 && !pm_wd_wake_frames) sync60_pace();`
   Lines :2442 (`if (pm_wd_wake_frames) pm_wd_wake_frames--;`) and :2443 stay. Update the PT88 comment at :2445 only if it names
   SYNCDRAW for fps60 ("with fps60 the begin waits for the vblank as before" -> "with fps60 the begin is paced by time (SYNC60)").
## Gate (CLAUDE.md 2: nd_host and byte compare)
- Bases: LOADMTX is merged: `bench.py base` stores fresh bases again (test_catalog.md 6a).
- `PM_ROW=SYNC60 PM_LANE=SYNC60 sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row SYNC60 --why "SYNC60 build"`: PASS.
- Byte compare (one Azahar list): spot_mac01_fps60_pic (interpolation on) and spot_mac01_det (off, unchanged path):
  `python 3ds/tools/bench.py base <bench> --row SYNC60`, `python 3ds/tools/bench.py run <bench> --row SYNC60 --lane SYNC60 --why
  "SYNC60 byte gate"`, `python 3ds/tools/bench.py cmp <bench> --row SYNC60`: 6/6 byte-equal each. If spot_mac01_fps60_pic's base
  prints "two runs differ", stop and hand back (its picture is then not deterministic, and the gate needs the orchestrator).
- Console (user): 60 FPS on, mac_01 and a battle: no tearing, no stutter, game speed unchanged. Azahar cannot show the LCD timing.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
