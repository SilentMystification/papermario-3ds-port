
## Trace of the later freeze (ORACLE7, after the user's 17:50 fact: f73457b88 with FPS60W2 still froze in mac_01 after some time)
No cause by value found in this pass. Paths read and why each cannot freeze, with values:
- Stream chains: two chains by gCurrentDisplayContextIndex & 1 (nd_rec.c:155-163; ch_enter resets a chain when its parity comes
  back). Step N's key walk reads chain N&1; the logic thread posts step N+1's frame only after frame N and key N are walked
  (nd_wq.c:150-155, wq_fr_walked counts keys too: :168), so chain N&1 is reset only after key N is done.
- Idle passes: retrace_call flips D_80073E0A; only the pass that makes it 0 runs step_game_loop, gfx_task_background and
  gfx_draw_frame (src/main.c:131-140), so idle passes add no task to key_list (pm_nusys.c:176).
- kc_buf and the vertex ring: written only after the frame begin waited for the GX queue that read them (pm_gpu.c:2789-2790,
  :2909-2917). VRAM uploads join the same GX queue before the begin (pm_tex.c:695-699): sequential with the key frame.
- GX queue: drained at 24 of 32 (pm_gpu.c:2455-2464) before every add of the key frame (:2953).
- Scene changes: kc_cut = 2 refuses the copy for 2 frames (pm_gpu.c:2869, :2907) and the key frame is walked (pm_nusys.c:229-230):
  the kcen run shows why[3] = 4 per 300 steps.
- Why no log came back: the push used profile prof (prof=4: log=off, pm_log.c:655-661): the live log carries only prof lines,
  one per 300 frames, which stop when frames stop. The watchdog writes its crash lines to crash.txt on the SD card 10 s after the
  game thread stops (pm_wdog.c:202-222); that file is on the card after the power cycle.
Open for the next oracle (this one's context passed 700K): the K frame re-queues the L frame's memory fills and VRAM copies
(pm_gpu.c:2946-2955); what each fill and copy targets at the frames around a scene change and an off-screen pass (gs.offs slots
reset each frame, :2760), and whether any re-queued entry can target memory the GPU no longer owns.
