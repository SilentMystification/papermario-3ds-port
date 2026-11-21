# ws_KICK3 (Haiku): run the GX queue at the kick split, so the GPU draws part 1 of the L frame while core 0 walks part 2
Designer: oracle NATIVE19 (plan_critpath.md 0, 5 row 1). Worktree build3ds/wt/KICK3, branch KICK3. PM_ROW=KICK3 PM_LANE=KICK3. Start: main 400d2ad3ab or later.
Why Haiku: one call after one line, given below.
FACT: C3D_FrameBegin stops the GX queue (citro3d renderqueue.c:97, :170). kick_split (3ds/src/pm_gpu.c:851-865) calls only C3D_FrameSplit; libctru
gxCmdQueueAdd does not run a stopped queue (source/gpu/gxqueue.c:62-75). So the GPU starts at C3D_FrameEnd (renderqueue.c:240) and KICK1 does nothing.
gxCmdQueueRun (gxqueue.c:77-91) starts it. Later entries then run at once or at the next interrupt (gxqueue.c:27-50, :69-73). pm_gx_room, pm_gx_drain
(pm_gpu.c:2641-2668) and C3D_FrameBegin already wait for a running queue. The depth read waits after a kick (pm_gpu.c:4800-4805 area: `if (kick_done || grab_split) pm_gx_drain();`).
Objective: on the console the GPU tail of the L frame (c0k gpul, ws_CPT0) falls to about 45 percent of gpuk; ft -3 to -4.5 ms (GUESS). Azahar: no change.
Counter: console ft. Rule (hw_guidelines.md 8): the kick already flushes the written span once (gpu_flush_written in kick_split).
## Edit (pm_gpu.c kick_split; CRLF: use the Edit tool)
Before (pm_gpu.c:858-859):
        { unsigned k0 = pm_ph_cen ? pm_ticks() : 0; /* PI4: the split is timed on its own (phase mode only) */
          ndunit_scan(); rd_sync(); C3D_FrameSplit(GX_CMDLIST_FLUSH); sc_spl++;
After:
        { unsigned k0 = pm_ph_cen ? pm_ticks() : 0; /* PI4: the split is timed on its own (phase mode only) */
          ndunit_scan(); rd_sync(); C3D_FrameSplit(GX_CMDLIST_FLUSH); sc_spl++;
          gxCmdQueueRun((gxCmdQueue_s*)&__C3D_Context); /* KICK3: C3D_FrameBegin stopped the queue (citro3d renderqueue.c:97): run it, so the GPU draws part 1 now */
Do not change the KICK1 comment or kick_at.
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=KICK3 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base spot_mac01_fps60_stereo_det --commit $(git merge-base HEAD main)`; run and cmp with `--row KICK3`: byte-equal
   (fps60 K copy, stereo replay, the kick). Then the same 3 commands for spot_mac01_drec: byte-equal (drec and gst).
   Azahar runs a command list when it is submitted, so a word that the CPU writes after the kick into part 1 shows as a DIFF here.
Fix rights: compile errors (gxCmdQueue_s and gxCmdQueueRun come from <3ds.h>; pm_gpu.c:2646 calls it already). A DIFF or a base fault: stop, send the output
path to oracle NATIVE19 (the first DIFF frame and draw name the late write).
Stop: commit `KICK3 1: gate. <host> <c0gate> <2 cmp lines>` deletes this page. Hand-back: one SubagentHandback: commit hash, the results.
Risk (console only): a missed swap when the GPU ends the frame before C3D_FrameEnd sets needSwapTop (renderqueue.c:227-230): one late picture, no hang.
Fallback: revert the line (the GPU starts at the frame end, as now).
